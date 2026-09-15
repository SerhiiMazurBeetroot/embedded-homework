#ifndef INC_DS1307_H_
#define INC_DS1307_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define DS1307_I2C_ADDR (0x68 << 1) // Address DS1307 for STM32 HAL (0xD0)

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t dayOfWeek;   /* 1=Mon .. 7=Sun (project convention, see DS1307_WEEKDAY_NAMES) */
    uint8_t date;
    uint8_t month;
    uint16_t year;     // 2026
} DS1307_Time;

//  Short day names indexed by (dayOfWeek - 1), matches DS1307_SetTime() convention
extern const char *const DS1307_WEEKDAY_NAMES[7];

bool DS1307_Init(I2C_HandleTypeDef *hi2c);

// Reads the current time/date from the RTC. Returns false on I2C error
bool DS1307_GetTime(DS1307_Time *out);

// Writes time/date to the RTC (e.g. once, to set the clock at flash time)
bool DS1307_SetTime(const DS1307_Time *time);


#endif /* INC_DS1307_H_ */
