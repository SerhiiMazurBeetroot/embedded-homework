#include "esp_adc/adc_continuous.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


// ---- ADC1 continuous mode (P2M DMA): потенціометр на GPIO4 (ADC1_CH3) ----
#define ADC_UNIT        ADC_UNIT_1
#define ADC_CHANNEL     ADC_CHANNEL_3      // GPIO4 на ESP32-S3
#define ADC_ATTEN       ADC_ATTEN_DB_12    // повний діапазон ~0-3.3В
#define ADC_BIT_WIDTH   ADC_BITWIDTH_12

#define FRAME_SIZE      256   // байтів на один "готовий блок" DMA-кільця
#define POOL_FRAMES     4     // скільки таких блоків тримати в кільці одночасно
#define SEND_EVERY_N_FRAMES  16   // ~16 * 3.2мс ≈ 50мс між рядками в консоль

// ---- UART0 (буферизований, неблокуючий драйвер) ----
#define UART_PORT       UART_NUM_0
#define UART_BAUD       115200
#define UART_TX_BUF     1024
#define UART_RX_BUF     256   // RX не використовується, але драйвер вимагає буфер > UART_FIFO_LEN (128)


static const char *TAG = "ADC_DMA";
static adc_continuous_handle_t adc_handle;
static TaskHandle_t adc_task_handle;

// procees and send vars
static uint32_t acc_sum = 0;
static uint32_t acc_count = 0;
static uint32_t acc_min = 0xFFFFFFFF;
static uint32_t acc_max = 0;
static uint32_t frames_since_send = 0;

static bool IRAM_ATTR adc_conv_done_cb(adc_continuous_handle_t handle,
                                        const adc_continuous_evt_data_t *edata, void *user_data) {
    BaseType_t must_yield = pdFALSE;
    // Викликається з ISR -- лише будимо задачу-споживач, дані тут не обробляємо
    vTaskNotifyGiveFromISR(adc_task_handle, &must_yield);
    return must_yield == pdTRUE;
}

static void uart_dma_init(void) {
    uart_config_t cfg = {
        .baud_rate = UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));
    // tx_buffer_size > 0 -> uart_write_bytes() неблокуючий, копіює у кільцевий буфер драйвера
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_RX_BUF, UART_TX_BUF, 0, NULL, 0));
}

static void adc_dma_init(void) {
    adc_continuous_handle_cfg_t handle_cfg = {
        .max_store_buf_size = FRAME_SIZE * POOL_FRAMES,
        .conv_frame_size = FRAME_SIZE,
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle_cfg, &adc_handle));

    adc_digi_pattern_config_t pattern[1] = {
        {
            .atten = ADC_ATTEN,
            .channel = ADC_CHANNEL,
            .unit = ADC_UNIT,
            .bit_width = ADC_BIT_WIDTH,
        },
    };
    adc_continuous_config_t dig_cfg = {
        .pattern_num = 1,
        .adc_pattern = pattern,
        .sample_freq_hz = 20000,
        .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    ESP_ERROR_CHECK(adc_continuous_config(adc_handle, &dig_cfg));

    adc_continuous_evt_cbs_t cbs = { .on_conv_done = adc_conv_done_cb };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(adc_handle, &cbs, NULL));
}

static void process_and_send(adc_digi_output_data_t *data, int count) {
    // накопичуємо статистику з поточного кадру в загальні акумулятори
    for (int i = 0; i < count; i++) {
        uint32_t raw = data[i].type2.data;
        acc_sum += raw;
        acc_count++;
        if (raw < acc_min) acc_min = raw;
        if (raw > acc_max) acc_max = raw;
    }

    frames_since_send++;
    if (frames_since_send < SEND_EVERY_N_FRAMES) {
        return;   // ще не час відправляти -- просто накопичуємо далі
    }
    frames_since_send = 0;

    uint32_t avg = acc_sum / acc_count;
    uint32_t vpp = acc_max - acc_min;
    float v_avg = (avg / 4095.0f) * 3.3f;

    char line[96];
    int n = snprintf(line, sizeof(line),
                      "raw: min=%lu max=%lu avg=%lu vpp=%lu | avg=%.2fV\r\n",
                      acc_min, acc_max, avg, vpp, v_avg);
    uart_write_bytes(UART_PORT, line, n);

    // скинути акумулятори для наступного вікна
    acc_sum = 0;
    acc_count = 0;
    acc_min = 0xFFFFFFFF;
    acc_max = 0;
}

static void adc_read_task(void *arg) {
    uint8_t result[FRAME_SIZE];
    uint32_t bytes_read;

    ESP_ERROR_CHECK(adc_continuous_start(adc_handle));

    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);   // спати, поки DMA не наповнить черговий блок

        esp_err_t err = adc_continuous_read(adc_handle, result, FRAME_SIZE, &bytes_read, 0);
        if (err == ESP_OK) {
            process_and_send((adc_digi_output_data_t *)result, bytes_read / sizeof(adc_digi_output_data_t));
        }
    }
}

void app_main(void)
{
	uart_dma_init();
	adc_dma_init();
	xTaskCreate(adc_read_task, "adc_read_task", 4096, NULL, 5, &adc_task_handle);

    while (true) {
    	vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
