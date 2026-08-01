#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

// pins setup
#define PIN_RED    GPIO_NUM_14
#define PIN_YELLOW GPIO_NUM_13
#define PIN_GREEN  GPIO_NUM_12
#define PIN_MODE_BUTTON GPIO_NUM_8

// states
typedef enum {
	STATE_GREEN,
	STATE_GREEN_BLINK,
	STATE_YELLOW,
	STATE_RED,
	STATE_RED_YELLOW,
	STATE_YELLOW_BLINK
} traffic_light_state_t;

// intervals
#define TIME_GREEN_MS       5000
#define TIME_GREEN_BLINK_MS 3000
#define TIME_YELLOW_MS      2000
#define TIME_RED_MS         5000
#define TIME_RED_YELLOW_MS  2000
#define BLINK_INTERVAL_MS   500

// helpers handle LEDs
static void set_leds(bool red, bool yellow, bool green) {
	gpio_set_level(PIN_RED, red ? 1 : 0);
	gpio_set_level(PIN_YELLOW, yellow ? 1 : 0);
	gpio_set_level(PIN_GREEN, green ? 1 : 0);
}

// init GPIO
static void init_gpio(void) {
	gpio_config_t io_conf = {
		.pin_bit_mask =
			(1ULL << PIN_RED) | (1ULL << PIN_YELLOW) | (1ULL << PIN_GREEN),
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&io_conf);

	gpio_config_t btn_conf = {
		.pin_bit_mask = (1ULL << PIN_MODE_BUTTON),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&btn_conf);
}

// button handle
static bool is_button_pressed(void) {
    static bool last_btn_state = true;
    bool current_state = (gpio_get_level(PIN_MODE_BUTTON) == 0); // true when pressed (Pull-Up)

    if (current_state && !last_btn_state) {
        vTaskDelay(pdMS_TO_TICKS(30)); // debounce
        if (gpio_get_level(PIN_MODE_BUTTON) == 0) {
            last_btn_state = true;
            return true;
        }
    } else if (!current_state) {
        last_btn_state = false;
    }
    return false;
}

static bool wait_with_button_check(uint32_t total_ms, bool *blink_mode) {
    uint32_t elapsed = 0;
    const uint32_t step_ms = 50;

    while (elapsed < total_ms) {
        if (is_button_pressed()) {
            *blink_mode = !(*blink_mode);
            return true; // button pressed - exit
        }
        vTaskDelay(pdMS_TO_TICKS(step_ms));
        elapsed += step_ms;
    }
    return false;
}

void traffic_light_task(void *pvParameters) {
	traffic_light_state_t state = STATE_GREEN;
	bool blink_mode = false;
    bool yellow_blink_state = false;

	while (1) {
		if (blink_mode) {
            yellow_blink_state = !yellow_blink_state;
            set_leds(false, yellow_blink_state, false);
            
            // during blinking pressed button - back to default mode 
            if (wait_with_button_check(BLINK_INTERVAL_MS, &blink_mode)) {
                if (!blink_mode) {
                    state = STATE_GREEN;
                }
            }
            continue;
        }

		switch (state) {	
			case STATE_GREEN:
				set_leds(false, false, true);
				if (wait_with_button_check(TIME_GREEN_MS, &blink_mode)) break;
				state = STATE_GREEN_BLINK;
				break;
	
			case STATE_GREEN_BLINK: {
				int cycles = TIME_GREEN_BLINK_MS / BLINK_INTERVAL_MS;
				bool led_on = false;

				for(int i = 0; i < cycles; i++) {
					led_on = !led_on;
					set_leds(false, false, led_on);
					if (wait_with_button_check(BLINK_INTERVAL_MS, &blink_mode)) break;
				}

				if (!blink_mode) state = STATE_YELLOW;
				break;
			}

			case STATE_YELLOW:
				set_leds(false, true, false);
				if (wait_with_button_check(TIME_YELLOW_MS, &blink_mode)) break;
				state = STATE_RED;
				break;

			case STATE_RED:
				set_leds(true, false, false);
				if (wait_with_button_check(TIME_RED_MS, &blink_mode)) break;
				state = STATE_RED_YELLOW;
				break;

			case STATE_RED_YELLOW:
				set_leds(true, true, false);
				if (wait_with_button_check(TIME_RED_YELLOW_MS, &blink_mode)) break;
				state = STATE_GREEN;
				break;

			default:
                state = STATE_GREEN;
                break;
		}
	}
}

void app_main(void)
{
	init_gpio();

	xTaskCreate(traffic_light_task, "traffic_light_task", 2048, NULL, 5, NULL);
}
