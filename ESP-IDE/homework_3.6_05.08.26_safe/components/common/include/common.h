#include "safe_logic.h"
#include <stdarg.h>
#include <stdio.h>

void print_prompt(void) {
	printf("\n\r[SAFE] Try %d/%d. Enter a number %d: %d",
		   (SAFE_MAX_ATTEMPTS - safe_logic_get_attempts_left() + 1),
		   SAFE_MAX_ATTEMPTS, safe_logic_get_digit_index() + 1,
		   safe_logic_get_current_digit());
	fflush(stdout);
}

void safe_print(const char *format, ...) {
	printf("\n\r");

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	printf("\n");
	fflush(stdout); // Ensures immediate output in ESP-IDF console
}
