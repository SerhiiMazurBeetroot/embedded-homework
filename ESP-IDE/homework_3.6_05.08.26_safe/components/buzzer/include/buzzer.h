#pragma once

#include "stdint.h"

void buzzer_init(int pin_buzzer);
void buzzer_play_sound(uint32_t freq_hz, uint32_t duration_ms);
void buzzer_play_tick(void);	  // CW
void buzzer_play_tick_down(void); // CCW
void buzzer_play_success(void);
void buzzer_play_error(void);
void buzzer_play_alarm(void);
