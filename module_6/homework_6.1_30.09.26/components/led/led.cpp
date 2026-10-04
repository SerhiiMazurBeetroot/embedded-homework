#include "led.h"

void Led::init() {
	gpio_reset_pin(pin_);
	gpio_set_direction(pin_, GPIO_MODE_OUTPUT);
	set(LedState::Off);
}

void Led::set(LedState state) {
	state_ = state;
	gpio_set_level(pin_, static_cast<uint32_t>(state_));
}

void Led::toggle() {
	set(state_ == LedState::Off ? LedState::On : LedState::Off);
}
