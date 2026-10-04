#pragma once
#include <cstdint>
#include "driver/gpio.h"

enum class LedState : uint8_t {Off = 0, On = 1};

class Led {
public:
	explicit Led(gpio_num_t pin) : pin_(pin){}
	
	void init();
	void set(LedState state);
	void toggle();
	
private:
	gpio_num_t pin_;
	LedState state_ = LedState::Off;	
};