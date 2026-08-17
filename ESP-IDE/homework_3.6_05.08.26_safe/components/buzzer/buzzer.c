#include "buzzer.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

static int buzzer_pin = -1;
static bool buzzer_configured = false;

void buzzer_init(int pin_buzzer) {
	buzzer_pin = pin_buzzer;

	ledc_timer_config_t timer_cfg = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = LEDC_TIMER_10_BIT,
		.timer_num = LEDC_TIMER_0,
		.freq_hz = 1000,
		.clk_cfg = LEDC_AUTO_CLK,
	};
	ledc_timer_config(&timer_cfg);

	ledc_channel_config_t chan_cfg = {
		.gpio_num = buzzer_pin,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = LEDC_CHANNEL_0,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
	};
	ledc_channel_config(&chan_cfg);
	buzzer_configured = true;
}

void buzzer_play_sound(uint32_t freq_hz, uint32_t duration_ms) {
	if(freq_hz == 0) {
		vTaskDelay(pdMS_TO_TICKS(duration_ms));
		return;
	}

	ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq_hz);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

// tick for CW rotation
void buzzer_play_tick(void) {
	buzzer_play_sound(2000, 15);
}

// tick for CCW rotation
void buzzer_play_tick_down(void) {
	buzzer_play_sound(1200, 15);
}

void buzzer_play_success(void) {
	buzzer_play_sound(523, 150);
	buzzer_play_sound(659, 150);
	buzzer_play_sound(784, 150);
	buzzer_play_sound(1046, 300);
}

void buzzer_play_error(void) {
	buzzer_play_sound(200, 150);
    vTaskDelay(pdMS_TO_TICKS(100));
    buzzer_play_sound(200, 150);
}

void buzzer_play_alarm(void) {
    // enable sound (duty 50%)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // play 5 times siren
    for (int cycle = 0; cycle < 5; cycle++) {
        for (uint32_t freq = 600; freq <= 1600; freq += 40) {
            ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        for (uint32_t freq = 1600; freq >= 600; freq -= 40) {
            ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, freq);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }

    // disable sound
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
