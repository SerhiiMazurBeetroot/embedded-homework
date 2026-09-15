#include "bme280.h"
#include <string.h>

// Register map
#define BME280_REG_CHIP_ID     0xD0
#define BME280_REG_RESET       0xE0
#define BME280_REG_CTRL_HUM    0xF2
#define BME280_REG_STATUS      0xF3
#define BME280_REG_CTRL_MEAS   0xF4
#define BME280_REG_CONFIG      0xF5
#define BME280_REG_PRESS_MSB   0xF7
#define BME280_REG_CALIB_00    0x88 // dig_T1..dig_H1, 26 bytes
#define BME280_REG_CALIB_26    0xE1 // dig_H2..dig_H6, 7 bytes

#define BME280_CHIP_ID_VALUE   0x60
#define BME280_SPI_READ_BIT    0x80

#define BME280_SPI_TIMEOUT_MS  10

// Calibration trim values, read once at init (Bosch datasheet section 4.2.2).
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2, dig_T3;
    uint16_t dig_P1;
    int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4, dig_H5;
    int8_t   dig_H6;
} BME280_CalibData;

static SPI_HandleTypeDef *s_hspi;
static GPIO_TypeDef      *s_cs_port;
static uint16_t           s_cs_pin;
static BME280_CalibData   s_calib;

static inline void CS_Low(void)  { HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_RESET); }
static inline void CS_High(void) { HAL_GPIO_WritePin(s_cs_port, s_cs_pin, GPIO_PIN_SET); }

static bool BME280_ReadRegs(uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint8_t addr = reg | BME280_SPI_READ_BIT;
    bool ok = true;

    CS_Low();
    if (HAL_SPI_Transmit(s_hspi, &addr, 1, BME280_SPI_TIMEOUT_MS) != HAL_OK) {
        ok = false;
    }
    if (ok && HAL_SPI_Receive(s_hspi, buf, len, BME280_SPI_TIMEOUT_MS) != HAL_OK) {
        ok = false;
    }
    CS_High();

    return ok;
}

static bool BME280_WriteReg(uint8_t reg, uint8_t val)
{
    uint8_t payload[2] = { (uint8_t)(reg & 0x7Fu), val }; // MSB=0 selects write
    bool ok = true;

    CS_Low();
    if (HAL_SPI_Transmit(s_hspi, payload, sizeof(payload), BME280_SPI_TIMEOUT_MS) != HAL_OK) {
        ok = false;
    }
    CS_High();

    return ok;
}

static bool BME280_ReadCalibration(void)
{
    uint8_t buf0[26];
    uint8_t buf1[7];

    if (!BME280_ReadRegs(BME280_REG_CALIB_00, buf0, sizeof(buf0))) {
        return false;
    }
    if (!BME280_ReadRegs(BME280_REG_CALIB_26, buf1, sizeof(buf1))) {
        return false;
    }

    s_calib.dig_T1 = (uint16_t)(buf0[1] << 8 | buf0[0]);
    s_calib.dig_T2 = (int16_t)(buf0[3] << 8 | buf0[2]);
    s_calib.dig_T3 = (int16_t)(buf0[5] << 8 | buf0[4]);

    s_calib.dig_P1 = (uint16_t)(buf0[7] << 8 | buf0[6]);
    s_calib.dig_P2 = (int16_t)(buf0[9] << 8 | buf0[8]);
    s_calib.dig_P3 = (int16_t)(buf0[11] << 8 | buf0[10]);
    s_calib.dig_P4 = (int16_t)(buf0[13] << 8 | buf0[12]);
    s_calib.dig_P5 = (int16_t)(buf0[15] << 8 | buf0[14]);
    s_calib.dig_P6 = (int16_t)(buf0[17] << 8 | buf0[16]);
    s_calib.dig_P7 = (int16_t)(buf0[19] << 8 | buf0[18]);
    s_calib.dig_P8 = (int16_t)(buf0[21] << 8 | buf0[20]);
    s_calib.dig_P9 = (int16_t)(buf0[23] << 8 | buf0[22]);

    s_calib.dig_H1 = buf0[25];

    s_calib.dig_H2 = (int16_t)(buf1[1] << 8 | buf1[0]);
    s_calib.dig_H3 = buf1[2];
    s_calib.dig_H4 = (int16_t)((buf1[3] << 4) | (buf1[4] & 0x0Fu));
    s_calib.dig_H5 = (int16_t)((buf1[5] << 4) | (buf1[4] >> 4));
    s_calib.dig_H6 = (int8_t)buf1[6];

    return true;
}

// --- Bosch reference compensation formulas (double-precision variants) ---
// t_fine carries temperature state into the pressure/humidity formulas, as required
// by the datasheet — it must come from the same sample as the P/H raw readings.

static double CompensateTemperature(int32_t adc_T, int32_t *t_fine)
{
    double var1 = (((double)adc_T) / 16384.0 - ((double)s_calib.dig_T1) / 1024.0) * (double)s_calib.dig_T2;
    double var2 = ((((double)adc_T) / 131072.0 - ((double)s_calib.dig_T1) / 8192.0)
                  * (((double)adc_T) / 131072.0 - ((double)s_calib.dig_T1) / 8192.0)) * (double)s_calib.dig_T3;
    *t_fine = (int32_t)(var1 + var2);
    return (var1 + var2) / 5120.0;
}

static double CompensatePressure(int32_t adc_P, int32_t t_fine)
{
    double var1 = ((double)t_fine / 2.0) - 64000.0;
    double var2 = var1 * var1 * ((double)s_calib.dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)s_calib.dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)s_calib.dig_P4) * 65536.0);
    var1 = (((double)s_calib.dig_P3) * var1 * var1 / 524288.0 + ((double)s_calib.dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)s_calib.dig_P1);

    if (var1 == 0.0) {
        return 0.0; // avoid divide-by-zero, mirrors the Bosch reference driver
    }

    double p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)s_calib.dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)s_calib.dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)s_calib.dig_P7)) / 16.0;

    return p / 100.0; // Pa -> hPa
}

static double CompensateHumidity(int32_t adc_H, int32_t t_fine)
{
    double h = ((double)t_fine) - 76800.0;
    h = (adc_H - (((double)s_calib.dig_H4) * 64.0 + ((double)s_calib.dig_H5) / 16384.0 * h))
        * (((double)s_calib.dig_H2) / 65536.0
           * (1.0 + ((double)s_calib.dig_H6) / 67108864.0 * h
              * (1.0 + ((double)s_calib.dig_H3) / 67108864.0 * h)));
    h = h * (1.0 - ((double)s_calib.dig_H1) * h / 524288.0);

    if (h > 100.0) h = 100.0;
    if (h < 0.0)   h = 0.0;
    return h;
}

bool BME280_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin)
{
    s_hspi    = hspi;
    s_cs_port = cs_port;
    s_cs_pin  = cs_pin;
    CS_High();

    uint8_t chip_id = 0;
    if (!BME280_ReadRegs(BME280_REG_CHIP_ID, &chip_id, 1) || chip_id != BME280_CHIP_ID_VALUE) {
        return false;
    }

    if (!BME280_WriteReg(BME280_REG_RESET, 0xB6)) {
        return false;
    }
    HAL_Delay(5); // one-time startup reset, not part of the periodic sampling loop

    // osrs_h = x1
    if (!BME280_WriteReg(BME280_REG_CTRL_HUM, 0x01)) {
        return false;
    }
    // t_sb = 0.5 ms, filter off, SPI 3-wire disabled
    if (!BME280_WriteReg(BME280_REG_CONFIG, 0x00)) {
        return false;
    }
    // osrs_t = x1, osrs_p = x1, mode = normal — ctrl_hum only takes effect after this write
    if (!BME280_WriteReg(BME280_REG_CTRL_MEAS, 0x27)) {
        return false;
    }

    return BME280_ReadCalibration();
}

bool BME280_Read(BME280_Data *out)
{
    uint8_t raw[8]; // press(3) + temp(3) + hum(2), contiguous from 0xF7
    if (!BME280_ReadRegs(BME280_REG_PRESS_MSB, raw, sizeof(raw))) {
        return false;
    }

    int32_t adc_P = ((int32_t)raw[0] << 12) | ((int32_t)raw[1] << 4) | (raw[2] >> 4);
    int32_t adc_T = ((int32_t)raw[3] << 12) | ((int32_t)raw[4] << 4) | (raw[5] >> 4);
    int32_t adc_H = ((int32_t)raw[6] << 8) | raw[7];

    int32_t t_fine = 0;
    out->temperature = (float)CompensateTemperature(adc_T, &t_fine);
    out->pressure     = (float)CompensatePressure(adc_P, t_fine);
    out->humidity      = (float)CompensateHumidity(adc_H, t_fine);

    return true;
}
