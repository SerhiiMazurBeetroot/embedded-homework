#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ldr_sensor.h"
#include "servo_control.h"
#include "common.h"
#include "config.h"

// Light range used to map sensor mv -> servo deg
#define LIGHT_MIN_MV     50
#define LIGHT_MAX_MV     3000

#define SUPERLOOP_DELAY_MS 100

static const char *TAG = "homework_3.7";

void app_main(void) {
    ldr_sensor_config_t ldr_cfg = {
        .unit = ADC_UNIT,
        .channel = ADC_CHANNEL,
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH,
        .sma_window_size = SMA_WINDOW_SIZE,
    };
    ESP_ERROR_CHECK(ldr_sensor_init(&ldr_cfg));

    servo_control_config_t servo_cfg = {
        .gpio_num = SERVO_PIN,
        .freq_hz = SERVO_FREQ_HZ,
        .duty_resolution = SERVO_RESOLUTION,
        .timer_num = SERVO_TIMER,
        .channel = SERVO_CHANNEL,
        .min_us = SERVO_MIN_US,
        .max_us = SERVO_MAX_US,
        .min_deg = SERVO_MIN_DEG,
        .max_deg = SERVO_MAX_DEG,
    };
    ESP_ERROR_CHECK(servo_control_init(&servo_cfg));

    ESP_LOGI(TAG, "Starting loop");

    while (true) {
        int filtered_mv = 0;
        esp_err_t err = ldr_sensor_read_filtered_mv(&filtered_mv);
        if (err == ESP_OK) {
            int angle_deg = range_map(filtered_mv, LIGHT_MIN_MV, LIGHT_MAX_MV, SERVO_MIN_DEG, SERVO_MAX_DEG);

            ESP_LOGI(TAG, "Light: %4d mV | Servo: %3d deg", filtered_mv, angle_deg);
            ESP_ERROR_CHECK(servo_control_set_angle_deg(angle_deg));
        } else {
            ESP_LOGE(TAG, "Failed to read LDR sensor: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(SUPERLOOP_DELAY_MS));
    }
}
