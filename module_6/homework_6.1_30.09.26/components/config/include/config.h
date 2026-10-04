#pragma once
#include <cstddef>
#include <cstdint>
#include "driver/gpio.h"

struct LedConfig {
	gpio_num_t pin;
	uint32_t intervalMs;
	const char *name;
};

class Config {
public:
	static constexpr LedConfig LEDS[] = {
		{GPIO_NUM_4, 200, "LED1"},
        {GPIO_NUM_5, 500, "LED2"},
        {GPIO_NUM_6, 1000, "LED3"},
	};

	static constexpr size_t LED_COUNT = sizeof(LEDS) / sizeof(LEDS[0]);
	static constexpr int TASK_CORE = 1;
	static constexpr uint32_t TASK_STACK_BYTES = 2048;
};
