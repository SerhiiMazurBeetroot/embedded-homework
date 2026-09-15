#pragma once

#include "esp_err.h"
#include "hal/adc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// setup ADC (LDR Sensor)
typedef struct {
    adc_unit_t unit;         // ADC_UNIT_1
    adc_channel_t channel;   // ADC_CHANNEL_8 (GPIO 9)
    adc_atten_t atten;       // ADC_ATTEN_DB_12
    adc_bitwidth_t bitwidth; // ADC_BITWIDTH_12
    int sma_window_size;
} ldr_sensor_config_t;

// ADC init and calibration
esp_err_t ldr_sensor_init(const ldr_sensor_config_t *config);

// Read a raw and convert it to mv
esp_err_t ldr_sensor_read_raw_mv(int *out_mv);

// SMA for smoothing ADC readings
esp_err_t ldr_sensor_read_filtered_mv(int *out_mv);

#ifdef __cplusplus
}
#endif
