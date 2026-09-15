#include "safe_logic.h"
#include "string.h"

static int secret_code[SAFE_CODE_LENGTH];
static int user_code[SAFE_CODE_LENGTH];
static int current_idx = 0;
static int attempts_left = SAFE_MAX_ATTEMPTS;

void safe_logic_init(const int target_code[SAFE_CODE_LENGTH]) {
	memcpy(secret_code, target_code, sizeof(secret_code));
	safe_logic_reset_attempt();
}

void safe_logic_reset_attempt(void) {
	current_idx = 0;
	memset(user_code, 0, sizeof(user_code));
}

void safe_logic_increment_digit(void) {
	user_code[current_idx] = (user_code[current_idx] + 1) % 10;
}

void safe_logic_confirm_digit(void) {
	if(current_idx < SAFE_CODE_LENGTH -1) {
		current_idx++;
		user_code[current_idx] = 0;
	}
}

int safe_logic_get_current_digit(void) { return user_code[current_idx]; }
int safe_logic_get_digit_index(void) { return current_idx; }
int safe_logic_get_attempts_left(void) { return attempts_left; }

safe_status_t safe_logic_check(void) {
	bool is_correct = true;
	for(int i = 0; i < SAFE_CODE_LENGTH; i++) {
		if(user_code[i] != secret_code[i]) {
			is_correct = false;
			break;
		}
	}

	if(is_correct) {
		return SAFE_STATUS_UNLOCKED;
	} else {
		attempts_left--;
		if(attempts_left <= 0) {
			return SAFE_STATUS_BLOCKED;
		}
		return SAFE_STATUS_WRONG_CODE;
	}
}
