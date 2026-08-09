#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdckdint.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

// setup ADC (LDR Sensor)
#define ADC_CHANNEL ADC_CHANNEL_8 // GPIO 9
#define ADC_UNIT ADC_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

// setup Servo (LEDC PWM)
#define SERVO_PIN 4
#define SERVO_FREQ_HZ 50
#define SERVO_PERIOD_US (1000000 / SERVO_FREQ_HZ) // 20,000 us (20ms)
#define SERVO_RESOLUTION LEDC_TIMER_12_BIT
#define SERVO_MAX_DUTY ((1 << SERVO_RESOLUTION) - 1) // 4095
#define SERVO_UNIT LEDC_TIMER_0
#define SERVO_CHANNEL LEDC_CHANNEL_0

// Calibration variables
#define SERVO_MIN_DEG 10
#define SERVO_MAX_DEG 170

#define SERVO_MIN_US 500
#define SERVO_MAX_US 2500

#define LIGHT_MIN_MV 50
#define LIGHT_MAX_MV 3000

#define SUPERLOOP_DELAY_MS 100
#define SMA_WINDOW_SIZE 5

static const char *TAG = "homework_3.7";

static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;

// ADC init and calibration
static void ldr_init(void) {
	adc_oneshot_unit_init_cfg_t unit_cfg = {
		.unit_id = ADC_UNIT,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

	adc_oneshot_chan_cfg_t chan_cfg = {
		.atten = ADC_ATTEN,
		.bitwidth = ADC_BITWIDTH,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

	#if CONFIG_ADC_CALI_CURVE_FITTING_SUPPORTED
		adc_cali_curve_fitting_config_t cfg = {
			.unit_id = ADC_UNIT,
			.chan = ADC_CHANNEL,
			.atten = ADC_ATTEN,
			.bitwidth = ADC_BITWIDTH,
		};
		ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cfg, &cali_handle));
	#endif
	ESP_LOGI(TAG, "ADC and Calibration initialized successfully.");
}

// PWM (LEDC) init for servo control
static void servo_init(void) {
	ledc_timer_config_t timer_cfg = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.timer_num = SERVO_UNIT,
		.duty_resolution = SERVO_RESOLUTION,
		.freq_hz = SERVO_FREQ_HZ,
		.clk_cfg = LEDC_AUTO_CLK,
	};
	ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

	ledc_channel_config_t chan_cfg = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = SERVO_CHANNEL,
		.timer_sel = SERVO_UNIT,
		.intr_type = LEDC_INTR_DISABLE,
		.gpio_num = SERVO_PIN,
		.duty = 0,
	};
	ESP_ERROR_CHECK(ledc_channel_config(&chan_cfg));
	ESP_LOGI(TAG, "Servo PWM (LEDC) initialized successfully");
}

// limit value in the range [val_min, val_max]
static inline int val_clamp(int value, int val_min, int val_max) {
	if(value < val_min) return val_min;
	if(value > val_max) return val_max;
	return value;
}

// linear range transform
static int map_range(int value, int in_min, int in_max, int out_min, int out_max) {
	if(in_min == in_max) return out_max;
	value = val_clamp(value, in_min, in_max);
	return out_min + (value -in_min) * (out_max - out_min) / (in_max - in_min);
}

// SMA for smoothing ADC readings
static int apply_sma(int new_val) {
	static int buffer[SMA_WINDOW_SIZE] = {0};
	static int index = 0;
	static int count = 0;
	static int sum = 0;

	sum -= buffer[index];
	buffer[index] = new_val;
	sum += buffer[index];

	index = (index + 1) % SMA_WINDOW_SIZE;
	if(count < SMA_WINDOW_SIZE) count++;

	return sum / count;
}

// set servo angle
static void set_servo_angle_by_voltage(int mv) {
	int filtered_mv = apply_sma(mv);
	int val_deg = map_range(filtered_mv, LIGHT_MIN_MV, LIGHT_MAX_MV,
							SERVO_MIN_DEG, SERVO_MAX_DEG);
	int val_us = map_range(val_deg, SERVO_MIN_DEG, SERVO_MAX_DEG, SERVO_MIN_US,
						   SERVO_MAX_US);

	// calculate
	uint32_t duty = (uint32_t)(((uint64_t)val_us * SERVO_MAX_DUTY) / SERVO_PERIOD_US);
	ESP_LOGD(TAG, "mV: %d | Filtered: %d | Deg: %d | Us: %d | Duty: %lu", mv,
			 filtered_mv, val_deg, val_us, (unsigned long)duty);

	ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL, duty));
	ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, SERVO_CHANNEL));
}

void app_main(void) {
	ldr_init();
	servo_init();

	int raw_adc = 0;
	int voltage_mv = 0;

	ESP_LOGI(TAG, "Starting loop");

	while (true) {
		// read raw values from ADC
		if (adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw_adc) == ESP_OK) {
			// convert into mv
			if(cali_handle != NULL) {
				adc_cali_raw_to_voltage(cali_handle, raw_adc, &voltage_mv);
			} else {
				voltage_mv = (raw_adc * 3300) / 4095;
			}

			ESP_LOGI(TAG, "Raw ADC: %4d | Voltage: %4d mV", raw_adc,
					 voltage_mv);

			// set servo angle
			set_servo_angle_by_voltage(voltage_mv);
		} else {
			ESP_LOGE(TAG, "Failed to read ADC value");
		}

		vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}
