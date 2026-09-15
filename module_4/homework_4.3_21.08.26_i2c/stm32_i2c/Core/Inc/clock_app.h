#ifndef CLOCK_APP_H_
#define CLOCK_APP_H_

#include "stm32f4xx_hal.h"

// Call once after MX_I2C1_Init() in CubeMX-generated main.c
void ClockApp_Init(I2C_HandleTypeDef *hi2c);

// Call repeatedly from the superloop (while(1) { ClockApp_Run(); })
void ClockApp_Run(void);

#endif /* CLOCK_APP_H_ */
