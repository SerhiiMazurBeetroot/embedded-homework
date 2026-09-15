#include "ldr_sensor.h"

#include <stdlib.h>
#include <string.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "ldr_sensor";

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_cali_handle = NULL;
static ldr_sensor_config_t s_cfg;

// Simple moving average state, sized at init time for s_cfg.sma_window_size
static int *s_sma_buffer = NULL;
static int s_sma_index = 0;
static int s_sma_count = 0;
static int s_sma_sum = 0;

static int sma_apply(int new_val) {
    s_sma_sum -= s_sma_buffer[s_sma_index];
    s_sma_buffer[s_sma_index] = new_val;
    s_sma_sum += s_sma_buffer[s_sma_index];

    s_sma_index = (s_sma_index + 1) % s_cfg.sma_window_size;
    if (s_sma_count < s_cfg.sma_window_size) {
        s_sma_count++;
    }

    return s_sma_sum / s_sma_count;
}

esp_err_t ldr_sensor_init(const ldr_sensor_config_t *config) {
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is NULL");
    ESP_RETURN_ON_FALSE(config->sma_window_size > 0, ESP_ERR_INVALID_ARG, TAG,
                         "sma_window_size must be > 0");

    s_cfg = *config;

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = s_cfg.unit,
    };
    ESP_RETURN_ON_ERROR(adc_oneshot_new_unit(&unit_cfg, &s_adc_handle), TAG,
                         "failed to create ADC unit");

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = s_cfg.atten,
        .bitwidth = s_cfg.bitwidth,
    };
    ESP_RETURN_ON_ERROR(
        adc_oneshot_config_channel(s_adc_handle, s_cfg.channel, &chan_cfg), TAG,
        "failed to configure ADC channel");

#if CONFIG_ADC_CALI_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = s_cfg.unit,
        .chan = s_cfg.channel,
        .atten = s_cfg.atten,
        .bitwidth = s_cfg.bitwidth,
    };
    esp_err_t cali_err = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali_handle);
    if (cali_err != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration unavailable (%s), falling back to linear conversion",
                 esp_err_to_name(cali_err));
        s_cali_handle = NULL;
    }
#else
    ESP_LOGW(TAG, "ADC curve-fitting calibration not supported on this target");
#endif

    s_sma_buffer = calloc((size_t)s_cfg.sma_window_size, sizeof(int));
    ESP_RETURN_ON_FALSE(s_sma_buffer != NULL, ESP_ERR_NO_MEM, TAG,
                         "failed to allocate SMA buffer");
    s_sma_index = 0;
    s_sma_count = 0;
    s_sma_sum = 0;

    ESP_LOGI(TAG, "LDR sensor initialized (unit=%d channel=%d window=%d)", s_cfg.unit,
             s_cfg.channel, s_cfg.sma_window_size);
    return ESP_OK;
}

esp_err_t ldr_sensor_read_raw_mv(int *out_mv) {
    ESP_RETURN_ON_FALSE(out_mv != NULL, ESP_ERR_INVALID_ARG, TAG, "out_mv is NULL");
    ESP_RETURN_ON_FALSE(s_adc_handle != NULL, ESP_ERR_INVALID_STATE, TAG,
                         "ldr_sensor_init() not called");

    int raw_adc = 0;
    ESP_RETURN_ON_ERROR(adc_oneshot_read(s_adc_handle, s_cfg.channel, &raw_adc), TAG,
                         "ADC read failed");

    if (s_cali_handle != NULL) {
        ESP_RETURN_ON_ERROR(adc_cali_raw_to_voltage(s_cali_handle, raw_adc, out_mv), TAG,
                             "calibration conversion failed");
    } else {
        int max_raw = (1 << s_cfg.bitwidth) - 1;
        *out_mv = (raw_adc * 3300) / max_raw;
    }

    return ESP_OK;
}

esp_err_t ldr_sensor_read_filtered_mv(int *out_mv) {
    ESP_RETURN_ON_FALSE(out_mv != NULL, ESP_ERR_INVALID_ARG, TAG, "out_mv is NULL");

    int raw_mv = 0;
    esp_err_t err = ldr_sensor_read_raw_mv(&raw_mv);
    if (err != ESP_OK) {
        return err;
    }

    *out_mv = sma_apply(raw_mv);
    return ESP_OK;
}
