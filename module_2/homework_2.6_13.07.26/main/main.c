/*
 * GPIO8 -> 100 Om --|--> 100 nF -> GND
 *                   |
 *                  BTN
 *                   |
 *                  GND
 */

#include <stdio.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

/*
 *  * TASKS:
 * 1. Basic implementation (without debounce)
 * 2. Software debounce through a timer (time-based)
 * 3. Debounce via level checking (state-based)
 * 4. Polling + debounce (without interrupts)
 * 5. Hardware debounce (RC filter)
 */

#define TASK_NUMBER 1
#define BTN_PIN GPIO_NUM_8

#define DEBOUNCE_MS 50
#define DEBOUNCE_US (DEBOUNCE_MS * 1000)

static const char *TAG = "btn_debounce";

/* common variables */
static volatile uint32_t counter = 0;

/* variables for tasks 2, 3 */
#if TASK_NUMBER != 4
	static QueueHandle_t gpio_evt_queue = NULL;
#endif

#if TASK_NUMBER == 2
	static volatile int64_t last_interrupt_time_us = 0;
#endif

/* ISR */
#if TASK_NUMBER != 4
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)(uintptr_t)arg;

#if TASK_NUMBER == 1 || TASK_NUMBER == 5
    /* basic / hardware-debounced */
    counter++;
#endif
    /* tasks 2,3 */

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}
#endif

/* tasks 1, 2, 3, 5 */
#if TASK_NUMBER != 4
static void button_event_task(void *arg)
{
    uint32_t io_num;

    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
			#if TASK_NUMBER == 1
            	ESP_LOGI(TAG, "Task 1: Interrupt triggered! Counter: %lu", counter);

			#elif TASK_NUMBER == 2
            	int64_t now = esp_timer_get_time();
				if (now - last_interrupt_time_us >= DEBOUNCE_US) {
					counter++;
					ESP_LOGI(TAG, "Task 2: Time-based Debounce. Counter: %lu", counter);
					last_interrupt_time_us = now;
				}

			#elif TASK_NUMBER == 3
				/* re-check the level after the ISR fired: INPUT_PULLUP -> pressed == 0 */
				if (gpio_get_level(BTN_PIN) == 0) {
					counter++;
					ESP_LOGI(TAG, "Task 3: State-based Debounce. Counter: %lu", counter);
				}

			#elif TASK_NUMBER == 5
				ESP_LOGI(TAG, "Task 5: Hardware Debounce. Counter: %lu", counter);
			#endif
        }
    }
}
#endif

/* task 4 */
#if TASK_NUMBER == 4
typedef enum {
    RELEASED,
    DEBOUNCE,
    PRESSED
} button_state_t;

static void button_poll_task(void *arg)
{
    button_state_t state = RELEASED;
    int64_t last_state_change_us = 0;

    for (;;) {
        int raw = gpio_get_level(BTN_PIN); /* INPUT_PULLUP -> pressed == 0 */

        switch (state) {
        case RELEASED:
            if (raw == 0) {
                state = DEBOUNCE;
                last_state_change_us = esp_timer_get_time();
            }
            break;

        case DEBOUNCE:
            if (esp_timer_get_time() - last_state_change_us >= 10000) {
                if (gpio_get_level(BTN_PIN) == 0) {
                    state = PRESSED;
                    counter++;
                    ESP_LOGI(TAG, "Task 4: Polling Debounce. Counter: %lu", counter);
                } else {
                    state = RELEASED;
                }
            }
            break;

        case PRESSED:
            if (raw != 0) {
                state = RELEASED;
            }
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
#endif


void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
#if TASK_NUMBER != 4
        .intr_type = GPIO_INTR_NEGEDGE, /* HIGH -> LOW: press with PULLUP */
#else
        .intr_type = GPIO_INTR_DISABLE,
#endif
    };
    gpio_config(&io_conf);

#if TASK_NUMBER != 4
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_PIN, gpio_isr_handler, (void *)(uintptr_t)BTN_PIN);

    xTaskCreate(button_event_task, "button_event_task", 4096, NULL, 10, NULL);
#else
    xTaskCreate(button_poll_task, "button_poll_task", 4096, NULL, 10, NULL);
#endif

    ESP_LOGI(TAG, "Start task %d", TASK_NUMBER);
}
