#ifndef U8G2_HAL_H_
#define U8G2_HAL_H_

#include "stm32f4xx_hal.h"
#include "u8g2.h"

void U8G2_HAL_Init(I2C_HandleTypeDef *hi2c);

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#endif /* U8G2_HAL_H_ */
