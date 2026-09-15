#include "ds1307.h"

const char *const DS1307_WEEKDAY_NAMES[7] = {
    "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
};

static I2C_HandleTypeDef *s_hi2c;

// helper functions
static inline uint8_t bcd_to_bin(uint8_t bcd) { return (bcd & 0x0F) + ((bcd >> 4) * 10); }
static inline uint8_t bin_to_bcd(uint8_t bin) { return (uint8_t)(((bin / 10) << 4) | (bin % 10)); }

// check clock exists on bus I2C
bool DS1307_Init(I2C_HandleTypeDef *hi2c)
{
    s_hi2c = hi2c;

    if (HAL_I2C_IsDeviceReady(s_hi2c, DS1307_I2C_ADDR, 3, 100) != HAL_OK) {
        return false;
    }

    /* Make sure the oscillator is running (clock-halt bit, reg 0x00 bit7 = 0).
     * If it was previously halted (fresh chip / dead battery), clear it
     * without disturbing the current seconds value. */
    uint8_t sec_reg = 0;
    if (HAL_I2C_Mem_Read(s_hi2c, DS1307_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT,
                          &sec_reg, 1, 100) != HAL_OK) {
        return false;
    }
    if (sec_reg & 0x80) {
        sec_reg &= 0x7F;
        HAL_I2C_Mem_Write(s_hi2c, DS1307_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT,
                           &sec_reg, 1, 100);
    }
    return true;
}

// read time from DS1307
bool DS1307_GetTime(DS1307_Time *out)
{
    uint8_t regs[7];
    if (HAL_I2C_Mem_Read(s_hi2c, DS1307_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT,
                          regs, sizeof(regs), 100) != HAL_OK) {
        return false;
    }

    out->seconds = bcd_to_bin(regs[0] & 0x7F);
    out->minutes = bcd_to_bin(regs[1]);
    out->hours   = bcd_to_bin(regs[2] & 0x3F);   // mask out 12/24h mode bits, we always init 24h
    out->dayOfWeek = regs[3];                      // 1..7 as previously programmed
    out->date    = bcd_to_bin(regs[4]);
    out->month   = bcd_to_bin(regs[5]);
    out->year    = 2000 + bcd_to_bin(regs[6]);
    return true;
}

// write new time to DS1307
bool DS1307_SetTime(const DS1307_Time *time)
{
    uint8_t regs[7];
    regs[0] = bin_to_bcd(time->seconds) & 0x7F; // bit7=0 -> oscillator enabled
    regs[1] = bin_to_bcd(time->minutes);
    regs[2] = bin_to_bcd(time->hours) & 0x3F;   // 24h mode
    regs[3] = time->dayOfWeek;
    regs[4] = bin_to_bcd(time->date);
    regs[5] = bin_to_bcd(time->month);
    regs[6] = bin_to_bcd((uint8_t)(time->year - 2000));

    return HAL_I2C_Mem_Write(s_hi2c, DS1307_I2C_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT,
                              regs, sizeof(regs), 100) == HAL_OK;
}
