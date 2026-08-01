#include <stdio.h>
#include <stdbool.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "hal/adc_types.h"
#include "math.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_CHANNEL      ADC_CHANNEL_6      // GPIO7
#define ADC_ATTEN        ADC_ATTEN_DB_12    // for measurement range 0...3.3V
#define ADC_BIT_WIDTH    ADC_BITWIDTH_12    // 12bit -> RAW 0...4095
#define V_REF_MV         3300.0f

#define OVERSAMPLE_N 8       // how many raw readings do we average per measurement
#define SAMPLE_PERIOD_MS 100

static const char *TAG = "homework_3.1";

static int read_raw_avg(adc_oneshot_unit_handle_t handle, adc_channel_t channel, int n) {
	long sum = 0;
	int val = 0;
	for(int i = 0; i < n; i++) {
		ESP_ERROR_CHECK(adc_oneshot_read(handle, channel, &val));
		sum += val;
	}
	return (int)(sum / n);
}

void app_main(void) {
	// init ADC1
	adc_oneshot_unit_handle_t adc_handle;
	adc_oneshot_unit_init_cfg_t init_cfg = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

	// channel config
	adc_oneshot_chan_cfg_t chan_cfg = {
		.bitwidth = ADC_BIT_WIDTH,
		.atten = ADC_ATTEN,
	};
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

	// init calibration
	adc_cali_handle_t cali_handle = NULL;
	bool do_calibration = false;

	adc_cali_curve_fitting_config_t cali_cfg = {
		.unit_id = ADC_UNIT_1,
		.chan = ADC_CHANNEL,
		.atten = ADC_ATTEN,
		.bitwidth = ADC_BIT_WIDTH,
	};

	esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle);

	if (ret == ESP_OK) {
		do_calibration = true;
		ESP_LOGI(TAG, "Curve Fitting calibration successfully initialized");
	} else {
		ESP_LOGI(TAG, "Not available on this chip, we use an approximate formula");
	}

	printf("\n=======================================================\n");
    printf("ESP32-S3 | Vref(nominal) = %.0f mV | Atten = 12dB | 12-bit | oversample x%d\n",
           V_REF_MV, OVERSAMPLE_N);
    printf("=======================================================\n");
    printf(" RAW  | U_manual(mV) | U_cali(mV) | Error(%%)\n");
    printf("-------------------------------------------------------\n");
	
	int raw_min = 4095, raw_max = 0;
   	float err_min = 1e9f, err_max = -1e9f;
   	int raw_at_err_min = 0, raw_at_err_max = 0;

	while (true) {
		// read avg RAW
		int raw_value =
			read_raw_avg(adc_handle, ADC_CHANNEL, OVERSAMPLE_N);
		
		// U_manual = RAW * (Vref / (2^12 - 1))
		float u_manual = (float)raw_value * (V_REF_MV / 4095.0f);

		// calibrated voltage
		int cali_mv = 0;
		if(do_calibration) {
			ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_value, &cali_mv));
		} else {
			cali_mv = (int)u_manual;
		}

		// relative error between manual and calibrated value (%)
		float error = 0.0f;
		if (cali_mv > 0) {
		    error = fabsf(u_manual - (float)cali_mv) / (float)cali_mv * 100.0f;
		}

		// tracking range and areas with maximum/minimum error
        if (raw_value < raw_min) raw_min = raw_value;
        if (raw_value > raw_max) raw_max = raw_value;
        if (error < err_min) { err_min = error; raw_at_err_min = raw_value; }
        if (error > err_max) { err_max = error; raw_at_err_max = raw_value; }

        printf("%4d  | %12.1f | %10d | %8.2f\n", raw_value, u_manual, cali_mv, error);
		 
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    }		 
}
