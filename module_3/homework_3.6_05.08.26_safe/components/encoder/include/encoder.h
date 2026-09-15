#pragma once

#include <stdbool.h>

typedef enum {
	ENCODER_EVENT_NONE = 0,
	ENCODER_EVENT_ROTATE_CW,  // clockwise rotation
	ENCODER_EVENT_ROTATE_CCW, // counter clockwise rotation
	ENCODER_EVENT_BTN_PRESSED // press button
} encoder_event_t;

void encoder_init(int pin_a, int pin_b, int pin_btn);
encoder_event_t encoder_poll(void);
