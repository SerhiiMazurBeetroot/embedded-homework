#include "servo_control.h"

#include "common.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "servo_control";

static servo_control_config_t s_cfg;
static bool s_initialized = false;

esp_err_t servo_control_init(const servo_control_config_t *config) {
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_FALSE(config->freq_hz > 0, ESP_ERR_INVALID_ARG, TAG, "freq_hz must be > 0");
    ESP_RETURN_ON_FALSE(config->min_deg <= config->max_deg, ESP_ERR_INVALID_ARG, TAG, "min_deg must be <= max_deg");

    s_cfg = *config;

    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = s_cfg.timer_num,
        .duty_resolution = s_cfg.duty_resolution,
        .freq_hz = s_cfg.freq_hz,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_cfg), TAG, "failed to configure LEDC timer");

    ledc_channel_config_t chan_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = s_cfg.channel,
        .timer_sel = s_cfg.timer_num,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = s_cfg.gpio_num,
        .duty = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&chan_cfg), TAG, "failed to configure LEDC channel");

    s_initialized = true;
    ESP_LOGI(TAG, "Servo initialized (gpio=%d freq=%dHz range=[%d,%d]deg)", s_cfg.gpio_num, s_cfg.freq_hz, s_cfg.min_deg, s_cfg.max_deg);
    return ESP_OK;
}

esp_err_t servo_control_set_angle_deg(int deg) {
    ESP_RETURN_ON_FALSE(s_initialized, ESP_ERR_INVALID_STATE, TAG, "servo_control_init() not called");

    int clamped_deg = range_clamp(deg, s_cfg.min_deg, s_cfg.max_deg);
    int pulse_us = range_map(clamped_deg, s_cfg.min_deg, s_cfg.max_deg, s_cfg.min_us, s_cfg.max_us);

    int period_us = 1000000 / s_cfg.freq_hz;
    int max_duty = (1 << s_cfg.duty_resolution) - 1;
    uint32_t duty = (uint32_t)(((uint64_t)pulse_us * max_duty) / period_us);

    ESP_LOGD(TAG, "deg: %d (clamped %d) | pulse: %dus | duty: %lu", deg, clamped_deg, pulse_us, (unsigned long)duty);

    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, s_cfg.channel, duty), TAG, "ledc_set_duty failed");
    ESP_RETURN_ON_ERROR(ledc_update_duty(LEDC_LOW_SPEED_MODE, s_cfg.channel), TAG, "ledc_update_duty failed");
    return ESP_OK;
}
