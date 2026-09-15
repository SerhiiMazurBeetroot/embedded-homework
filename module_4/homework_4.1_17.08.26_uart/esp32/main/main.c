#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include <stdint.h>

#define UART_PORT      UART_NUM_1
#define UART_TX_PIN    17
#define UART_RX_PIN    18
#define UART_BAUD      115200
#define UART_BUF_SIZE  256

#define LED_PIN GPIO_NUM_4 // esp32 LED
#define BTN_PIN GPIO_NUM_5 // esp32 button 

static const char *TAG = "homework_4.1_uart";

void hardware_init(void) {
	// init UART
    uart_config_t uart_cfg = {
        .baud_rate  = UART_BAUD,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
	ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_cfg));
	ESP_ERROR_CHECK(uart_set_pin(UART_PORT, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE, UART_BUF_SIZE, 0, NULL, 0));


	// init GPIO
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

	gpio_reset_pin(BTN_PIN);
	gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
	gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
	
	ESP_LOGI(TAG, "Hardware initialized successfully.");
}

// task send signalwhen press button esp32 -> stm32
void button_task(void *pv) {
	int last_state = 1;
	while(1) {
		int current_state = gpio_get_level(BTN_PIN);
		// from Hight to Low (PULLUP)
		if(last_state == 1 && current_state == 0) {
			uint8_t cmd = '1';
			uart_write_bytes(UART_PORT, (const char *)&cmd, 1);
			ESP_LOGI(TAG, "Button pressed! Sent '1' to STM32");
			vTaskDelay(pdMS_TO_TICKS(200)); // debounce
		}
		last_state = current_state;
		vTaskDelay(pdMS_TO_TICKS(20));
	}
}

// task receive stm32 -> esp32
void rx_task(void *pv) {
	uint8_t data;
	
	while(1) {
		int len = uart_read_bytes(UART_PORT, &data, 1, portMAX_DELAY);
		if(len > 0 && data == '1') {
			// LEDs state change esp32
			static int led_state = 0;
			led_state = !led_state;
			gpio_set_level(LED_PIN, led_state);
			ESP_LOGI(TAG, "Received '1' from STM32! LED state: %d", led_state);
		}
	}
}

void app_main(void)
{
    hardware_init();
	xTaskCreate(button_task, "button_task", 2048, NULL, 5, NULL);
	xTaskCreate(rx_task, "rx_task", 2048, NULL, 5, NULL);

}
