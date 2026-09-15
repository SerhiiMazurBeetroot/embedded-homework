#pragma once

#include "esp_err.h"
#include "driver/ledc.h"

#ifdef __cplusplus
extern "C" {
#endif

// ser
typedef struct {
    int gpio_num;                    // GPIO servo
    int freq_hz;                     // 50Hz
    ledc_timer_bit_t duty_resolution; // LEDC_TIMER_12_BIT
    ledc_timer_t timer_num;          // LEDC_TIMER_0
    ledc_channel_t channel;          // LEDC_CHANNEL_0

    int min_us;    // 500
    int max_us;    // 2500
    int min_deg;   // 10
    int max_deg;   // 170
} servo_control_config_t;

// PWM (LEDC) init for servo control
esp_err_t servo_control_init(const servo_control_config_t *config);

// set servo angle
esp_err_t servo_control_set_angle_deg(int deg);

#ifdef __cplusplus
}
#endif
