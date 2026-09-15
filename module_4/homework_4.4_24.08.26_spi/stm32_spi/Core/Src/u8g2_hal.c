#include "u8g2_hal.h"

static I2C_HandleTypeDef *s_hi2c;

void U8G2_HAL_Init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c = hi2c;
}

uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    static uint8_t buffer[32]; /* u8g2 never sends more than 32B per transfer for this driver */
    static uint8_t buf_idx;
    uint8_t *data;

    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            while (arg_int > 0) {
                buffer[buf_idx++] = *data;
                data++;
                arg_int--;
            }
            break;

        case U8X8_MSG_BYTE_INIT:
            /* HAL_I2C_Init() already done by CubeMX-generated MX_I2C1_Init(). */
            break;

        case U8X8_MSG_BYTE_SET_DC:
            /* Not used over I2C (only relevant for SPI displays). */
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_idx = 0;
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            HAL_I2C_Master_Transmit(s_hi2c, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 1000);
            break;

        default:
            return 0;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT:
            /* GPIO already configured by CubeMX (I2C pins), nothing to do. */
            break;

        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int);
            break;

        case U8X8_MSG_GPIO_I2C_CLOCK:
        case U8X8_MSG_GPIO_I2C_DATA:
            /* Only needed for bit-banged I2C; we use the hardware peripheral. */
            break;

        default:
            u8x8_SetGPIOResult(u8x8, 1); /* default "ok" for anything else u8g2 asks */
            break;
    }
    return 1;
}
