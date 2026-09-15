#include "main.h"       // pulls in BME280_CS_GPIO_Port / BME280_CS_Pin from CubeMX
#include <bme280.h>
#include "clock_app.h"
#include "u8g2.h"
#include "u8g2_hal.h"
#include "ds1307.h"
#include <stdio.h>
#include <string.h>

#define SET_TIME 0 // Set to 1 once to flash compile-time RTC stamp

#define SSD1306_WIDTH    128
#define SSD1306_HEIGHT   64
#define SSD1306_LAST_ROW (SSD1306_HEIGHT - 1)

#define FONT_BIG   u8g2_font_logisoso18_tr
#define FONT_SMALL u8g2_font_6x10_tf
#define FONT_ENV   u8g2_font_5x7_tf

#define BME280_SPI_HANDLE hspi1
#define BME280_CS_GPIO_PORT BME280_CS_GPIO_Port
#define BME280_CS_GPIO_PIN BME280_CS_Pin

#define ENV_UPDATE_PERIOD_MS 1000u // BME280 refresh cadence, independent of the RTC 1Hz tick

extern SPI_HandleTypeDef BME280_SPI_HANDLE;

static u8g2_t s_u8g2;
static uint8_t s_last_second = 0xFF; // Forces initial frame redraw

//  Layout Baselines
static uint8_t s_hm_baseline;
static uint8_t s_date_baseline;
static uint8_t s_env_baseline;
static uint8_t s_bar_y;

// BME280 state
static BME280_Data s_env;
static bool s_env_ok = false;
static uint32_t s_env_last_tick;

// Calculates day of the week (1=Sunday ... 7=Saturday) from date components.
static uint8_t Calculate_DayOfWeek(int y, int m, int d)
{
    static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    if (m < 3) {
        y -= 1;
    }
    int dow = (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
    return (uint8_t)(dow + 1);
}


static void LayoutInit(void)
{
    const uint8_t top_margin = 2;
    const uint8_t line_gap = 4;
    const uint8_t env_line_gap = 6;

    u8g2_SetFont(&s_u8g2, FONT_BIG);
    int8_t big_ascent  = u8g2_GetAscent(&s_u8g2);   // pixels above baseline
    int8_t big_descent = u8g2_GetDescent(&s_u8g2);  // <= 0, pixels below baseline

    u8g2_SetFont(&s_u8g2, FONT_SMALL);
    int8_t small_ascent = u8g2_GetAscent(&s_u8g2);
    int8_t small_descent = u8g2_GetDescent(&s_u8g2); // Також врахуємо хвостики літер дати (g, p, q тощо)

    // BME280 font
	u8g2_SetFont(&s_u8g2, FONT_ENV);
	int8_t env_ascent = u8g2_GetAscent(&s_u8g2);

    s_hm_baseline   = (uint8_t)(top_margin + big_ascent);
    s_date_baseline = (uint8_t)(s_hm_baseline - big_descent + line_gap + small_ascent);
    s_env_baseline  = (uint8_t)(s_date_baseline - small_descent + env_line_gap + env_ascent);

    // Anchor progress bar near the bottom edge
	s_bar_y = SSD1306_LAST_ROW - 3;

	// Sanity check at build time would be nicer, but a runtime guard keeps a
	// font swap from silently drawing the env line into the progress bar.
	if (s_env_baseline + 2 > s_bar_y) {
		s_env_baseline = s_bar_y - 2;
	}
}

#if SET_TIME
void Set_RTC_CurrentTime(I2C_HandleTypeDef *hi2c)
{
    DS1307_Time t;
    char month_str[4];
    int date = 1, year = 2026, hours = 0, minutes = 0, seconds = 0;

    sscanf(__DATE__, "%3s %d %d", month_str, &date, &year);
	sscanf(__TIME__, "%d:%d:%d", &hours, &minutes, &seconds);

    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    uint8_t month = 1;
    for (uint8_t i = 0; i < 12; i++) {
        if (strncmp(month_str, months[i], 3u) == 0) {
            month = i + 1;
            break;
        }
    }

    t.seconds = (uint8_t)seconds;
    t.minutes = (uint8_t)minutes;
    t.hours   = (uint8_t)hours;
    t.date    = (uint8_t)date;
    t.month   = (uint8_t)month;
    t.year    = (uint16_t)year;
    t.dayOfWeek = Calculate_DayOfWeek(year, month, date);

    DS1307_SetTime(&t);
}
#endif

void ClockApp_Init(I2C_HandleTypeDef *hi2c)
{
    U8G2_HAL_Init(hi2c);

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &s_u8g2, U8G2_R0,
        u8x8_byte_hw_i2c,
        u8x8_gpio_and_delay);

    u8g2_InitDisplay(&s_u8g2);   // sends the SSD1306 init sequence
    u8g2_SetPowerSave(&s_u8g2, 0); // wake the panel up (init leaves it off)
    u8g2_ClearBuffer(&s_u8g2);

    LayoutInit();

    if (!DS1307_Init(hi2c)) {
		u8g2_SetFont(&s_u8g2, FONT_SMALL);
		u8g2_DrawStr(&s_u8g2, 0, 20, "RTC Init Failed!");
		u8g2_SendBuffer(&s_u8g2);
		while (1) {
			HAL_Delay(500);
		}
	}

#if SET_TIME
    Set_RTC_CurrentTime(hi2c);
#endif

    // BME280 is treated as non-critical: unlike the RTC, its absence shouldn't
	// brick the clock. s_env_ok gates the display/log output instead.
    s_env_ok = BME280_Init(&BME280_SPI_HANDLE, BME280_CS_GPIO_PORT, BME280_CS_GPIO_PIN);
    s_env_last_tick = HAL_GetTick();

    u8g2_SetFont(&s_u8g2, FONT_SMALL);
    u8g2_DrawStr(&s_u8g2, 0, s_date_baseline, s_env_ok ? "I2C Clock ready" : "BME280 not found");
    u8g2_SendBuffer(&s_u8g2);
    HAL_Delay(500);
}

// rendering helpers
static void DrawSecondsBar(uint8_t seconds)
{
    const uint8_t bar_h = 3;   // Increased height for better visibility

    // Limit seconds to a range 0..59
    if (seconds > 59) {
        seconds = 59;
    }

    // Accurate calculation of width in pixels (0..128)
    uint8_t filled = (uint8_t)(((uint16_t)seconds * SSD1306_WIDTH) / 59);

    u8g2_SetDrawColor(&s_u8g2, 1);

    // We only draw a line/shaded strip that grows from 0 to 128px
    if (filled > 0) {
        u8g2_DrawBox(&s_u8g2, 0, s_bar_y, filled, bar_h);
    }
}

// Formats the environment line shown on screen; shared by the renderer and the
// logger so the logged values can never drift from what's actually displayed.
static void FormatEnvLine(char *buf, size_t buf_len)
{
    if (s_env_ok) {
        snprintf(buf, buf_len, "T:%.1fC RH:%.0f%% P:%.0fhPa",
                  s_env.temperature, s_env.humidity, s_env.pressure);
    } else {
        snprintf(buf, buf_len, "BME280 unavailable");
    }
}

static void RenderClock(const DS1307_Time *t)
{
    char hm_str[9];    // HH:MM:SS
    char date_str[20]; // Sat 14.02.2026
    char env_str[32]; // T:23.4C RH:45% P:1013hPa

    snprintf(hm_str, sizeof(hm_str), "%02u:%02u:%02u", t->hours, t->minutes, t->seconds);

    // Safe index access protecting against uninitialized RTC week values
	uint8_t dow_idx = (t->dayOfWeek >= 1 && t->dayOfWeek <= 7) ? (t->dayOfWeek - 1) : 0;

	snprintf(date_str, sizeof(date_str), "%s %02u.%02u.%04u",
                 DS1307_WEEKDAY_NAMES[dow_idx],
                 t->date, t->month, t->year);

	FormatEnvLine(env_str, sizeof(env_str));

    u8g2_ClearBuffer(&s_u8g2);

    // Time
    u8g2_SetFont(&s_u8g2, FONT_BIG);
    u8g2_DrawStr(&s_u8g2, 0, s_hm_baseline, hm_str);

    // Date
    u8g2_SetFont(&s_u8g2, FONT_SMALL);
	u8g2_DrawStr(&s_u8g2, 0, s_date_baseline, date_str);

	// BME280
	u8g2_SetFont(&s_u8g2, FONT_ENV);
	u8g2_DrawStr(&s_u8g2, 0, s_env_baseline, env_str);

    DrawSecondsBar(t->seconds);

    u8g2_SendBuffer(&s_u8g2);

    // TODO: swap printf() for the project's actual logging macro (e.g. LOG_INFO)
	// if it differs — this mirrors exactly what's on screen, per assignment spec.
	printf("%s %s | %s\r\n", hm_str, date_str, env_str);
}

// main
void ClockApp_Run(void)
{
    DS1307_Time t;
    if (!DS1307_GetTime(&t)) {
        u8g2_ClearBuffer(&s_u8g2);
        u8g2_SetFont(&s_u8g2, FONT_SMALL);
        u8g2_DrawStr(&s_u8g2, 0, s_date_baseline, "DS1307 read error");
        u8g2_SendBuffer(&s_u8g2);
        return; // transient I2C error - try again next tick
    }

    // Non-blocking BME280 refresh: tick-based, no delay(), independent of the
	// 1Hz RTC redraw cadence below (BME280 registers only change ~4Hz in
	// normal mode anyway, so 1s is plenty and keeps SPI traffic low).
	uint32_t now = HAL_GetTick();
	if ((now - s_env_last_tick) >= ENV_UPDATE_PERIOD_MS) {
		s_env_last_tick = now;
		s_env_ok = BME280_Read(&s_env);
	}

    // Only redraw when the second actually changes: guarantees the
    // >=1 Hz refresh requirement without hammering the I2C bus.
    if (t.seconds == s_last_second) {
        return;
    }
    s_last_second = t.seconds;


    RenderClock(&t);

}
