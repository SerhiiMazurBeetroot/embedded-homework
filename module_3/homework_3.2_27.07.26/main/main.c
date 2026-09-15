#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define LDR_ADC_UNIT    ADC_UNIT_1
#define LDR_ADC_CHANNEL ADC_CHANNEL_3 // GPIO4
#define LED_GPIO        GPIO_NUM_5    // GPIO5

#define SMA_WINDOW_SIZE 10

#define THRESHOLD_DARK 1500  // SMA < value - dark
#define THRESHOLD_LIGHT 1900 // SMA > value - light

#define SAMPLE_PERIOD_MS 50

static const char *TAG = "homework_3.2";


// SMA variables
static int sma_buffer[SMA_WINDOW_SIZE];
static int sma_index = 0;
static int sma_count = 0;
static int sma_sum = 0;

static int sma_update(int new_sample) {
	// if the buffer is full - decrease the value
	if(sma_count == SMA_WINDOW_SIZE) {
		sma_sum -= sma_buffer[sma_index];
	} else {
		sma_count++;
	}

	sma_buffer[sma_index] = new_sample;
	sma_sum += new_sample;

	sma_index = (sma_index + 1) % SMA_WINDOW_SIZE;

	return (int)(sma_sum / sma_count);
}

void app_main(void) {
	// LED config
	gpio_config_t led_cfg = {
		.pin_bit_mask = (1ULL << LED_GPIO),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&led_cfg);
	gpio_set_level(LED_GPIO, 0);

	// ADC config
	adc_oneshot_unit_handle_t adc_handle;
	adc_oneshot_unit_init_cfg_t init_cfg = {
		.unit_id = LDR_ADC_UNIT,
		.ulp_mode = ADC_ULP_MODE_DISABLE,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

	adc_oneshot_chan_cfg_t chan_cfg = {
		.atten = ADC_ATTEN_DB_12,		  // full range 0-3.3V
		.bitwidth = ADC_BITWIDTH_DEFAULT, // 12 bit (0...4095)
	};
	ESP_ERROR_CHECK(
		adc_oneshot_config_channel(adc_handle, LDR_ADC_CHANNEL, &chan_cfg));

	bool led_state = false;

	while (true) {
		int raw = 0;
		ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, LDR_ADC_CHANNEL, &raw));

		int filtered = sma_update(raw);

		// switch the state only when threshold is exceeded
		if(filtered < THRESHOLD_DARK && !led_state) {
			led_state = true;
			gpio_set_level(LED_GPIO, 1);
			ESP_LOGI(TAG, "Is dark -> LED ON (raw=%d, sma=%d)", raw, filtered);
		} else if(filtered > THRESHOLD_LIGHT && led_state) {
			led_state = false;
			gpio_set_level(LED_GPIO, 0);
			ESP_LOGI(TAG, "Is light -> LED OFF (raw=%d, sma=%d)", raw, filtered);
		}

		ESP_LOGD(TAG, "raw=%d, sma=%d led=%d", raw, filtered, led_state);

		vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }
}
