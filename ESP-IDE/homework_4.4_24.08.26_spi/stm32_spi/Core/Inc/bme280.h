#ifndef BME280_H
#define BME280_H

#include "main.h"   // brings in HAL types (SPI_HandleTypeDef, GPIO_TypeDef, ...) via CubeMX
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float temperature; // deg C
    float humidity;    // %RH
    float pressure;     // hPa
} BME280_Data;

// hspi   - SPI handle the sensor is wired to (full-duplex, mode 0)
// cs_port/cs_pin - chip-select GPIO (driven manually, not by hardware NSS)
// Returns false if the chip-id readback doesn't match 0x60 or the SPI transfer fails.
bool BME280_Init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

// Triggers a forced-mode-equivalent read in normal mode: pulls the latest
// T/P/H data registers and applies the Bosch compensation formulas.
// Returns false on SPI/communication failure; *out is left untouched in that case.
bool BME280_Read(BME280_Data *out);

#endif // BME280_H
