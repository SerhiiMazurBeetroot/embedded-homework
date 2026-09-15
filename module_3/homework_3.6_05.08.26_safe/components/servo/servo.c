#include "servo.h"
#include "driver/ledc.h"

static int servo_gpio = -1;

void servo_init(int pin_servo) {
	servo_gpio = pin_servo;

	// setup timer LEDC 50 Hz (20ms)
	ledc_timer_config_t timer_cfg = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.duty_resolution = LEDC_TIMER_13_BIT, // 0...8191
		.timer_num = LEDC_TIMER_1,
		.freq_hz = 50,
		.clk_cfg = LEDC_AUTO_CLK,
	};
	ledc_timer_config(&timer_cfg);

	ledc_channel_config_t chan_cfg = {
		.gpio_num = servo_gpio,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = LEDC_CHANNEL_1,
		.timer_sel = LEDC_TIMER_1,
		.duty = 0,
	};
	ledc_channel_config(&chan_cfg);

	// init state - closed (0 deg)
	servo_set_angle(0);
}

void servo_set_angle(int angle_deg) {
	if (angle_deg < 0) angle_deg = 0;
    if (angle_deg > 180) angle_deg = 180;

	// convert deg (0..180) into PWM 
	uint32_t duty = 205 + (angle_deg * (1024 - 205) / 180);

	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

