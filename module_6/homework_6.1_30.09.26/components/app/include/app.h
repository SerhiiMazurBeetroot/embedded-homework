#pragma once
#include "blinker.h"
#include "config.h"

class App {
public:
	App();
	void start();
	
private:
	Blinker blinkers_[Config::LED_COUNT];
};