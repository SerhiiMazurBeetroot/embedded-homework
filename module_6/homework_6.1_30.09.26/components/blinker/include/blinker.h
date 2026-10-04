#pragma once
#include "led.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Blinker {
public:
	Blinker(gpio_num_t pin, uint32_t intervalMs) : led_(pin), interval_(intervalMs) {}
	
	void init() { led_.init(); }
	bool start(const char *name, BaseType_t core, UBaseType_t priority = 1);
	
private:
	static void taskEntry(void *self);
	void run();
	
	Led led_;
	uint32_t interval_;
};