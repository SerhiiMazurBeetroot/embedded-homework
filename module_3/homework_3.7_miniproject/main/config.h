#pragma once

#include "driver/ledc.h"
#include "hal/adc_types.h"

// setup ADC (LDR Sensor)
#define ADC_CHANNEL      ADC_CHANNEL_8   // GPIO 9
#define ADC_UNIT         ADC_UNIT_1
#define ADC_ATTEN        ADC_ATTEN_DB_12
#define ADC_BITWIDTH     ADC_BITWIDTH_12
#define SMA_WINDOW_SIZE  5

// setup Servo (LEDC PWM)
#define SERVO_PIN        4
#define SERVO_FREQ_HZ    50
#define SERVO_RESOLUTION LEDC_TIMER_12_BIT
#define SERVO_TIMER      LEDC_TIMER_0
#define SERVO_CHANNEL    LEDC_CHANNEL_0

// Calibration variables
#define SERVO_MIN_US     500
#define SERVO_MAX_US     2500
#define SERVO_MIN_DEG    10
#define SERVO_MAX_DEG    170
