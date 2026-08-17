#pragma once

#include <stdbool.h>

#define SAFE_CODE_LENGTH 4
#define SAFE_MAX_ATTEMPTS 3

typedef enum {
	SAFE_STATUS_OK,
	SAFE_STATUS_UNLOCKED,
	SAFE_STATUS_WRONG_CODE,
	SAFE_STATUS_BLOCKED,
} safe_status_t;

void safe_logic_init(const int target_code[SAFE_CODE_LENGTH]);
void safe_logic_reset_attempt(void);
void safe_logic_increment_digit(void);
void safe_logic_confirm_digit(void);

int safe_logic_get_current_digit(void);
int safe_logic_get_digit_index(void);
int safe_logic_get_attempts_left(void);

safe_status_t safe_logic_check(void);
