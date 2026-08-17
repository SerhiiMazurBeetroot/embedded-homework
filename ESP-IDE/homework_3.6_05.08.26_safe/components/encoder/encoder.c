#include "encoder.h"
#include <stdio.h>
#include <stdlib.h>
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static pcnt_unit_handle_t pcnt_unit = NULL;
static int last_pcnt_count = 0;
static int btn_gpio = -1;
static int btn_last_state = 1;

void encoder_init(int pin_a, int pin_b, int pin_btn) {
	btn_gpio = pin_btn;

	// button
	gpio_config_t btn_cfg = {
		.pin_bit_mask = (1ULL << pin_btn),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.intr_type = GPIO_INTR_DISABLE
	};
	gpio_config(&btn_cfg);

	// PCNT for rotation
	pcnt_unit_config_t unit_cfg = {
		.low_limit = -100,
		.high_limit = 100,
	};
	pcnt_new_unit(&unit_cfg, &pcnt_unit);

	pcnt_chan_config_t chan_a_cfg = {
		.edge_gpio_num = pin_a,
		.level_gpio_num = pin_b,
	};
	pcnt_channel_handle_t pcnt_chan = NULL;
	pcnt_new_channel(pcnt_unit, &chan_a_cfg, &pcnt_chan);

	pcnt_channel_set_edge_action(pcnt_chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
	pcnt_channel_set_level_action(pcnt_chan, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

	pcnt_unit_enable(pcnt_unit);
	pcnt_unit_clear_count(pcnt_unit);
	pcnt_unit_start(pcnt_unit);
}

encoder_event_t encoder_poll(void) {
	// button handler
	int btn_state = gpio_get_level(btn_gpio);
	if(btn_state == 0 && btn_last_state == 1) {
		vTaskDelay(pdMS_TO_TICKS(50)); // debounce

		if(gpio_get_level(btn_gpio) == 0) {
			btn_last_state = btn_state;
			return ENCODER_EVENT_BTN_PRESSED;
		}
	}
	btn_last_state = btn_state;

	// rotation handler
	int current_count = 0;
	pcnt_unit_get_count(pcnt_unit, &current_count);
	int diff = current_count - last_pcnt_count;
	if(abs(diff) >= 2) { // 2 only for 1 click
		last_pcnt_count = current_count;
		return (diff >0) ? ENCODER_EVENT_ROTATE_CW : ENCODER_EVENT_ROTATE_CCW;
	}

	return ENCODER_EVENT_NONE;
}
