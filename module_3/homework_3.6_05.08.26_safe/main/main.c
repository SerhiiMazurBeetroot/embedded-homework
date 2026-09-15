#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

#include "encoder.h"
#include "buzzer.h"
#include "safe_logic.h"
#include "servo.h"
#include "common.h"

#define PIN_ENC_A 17
#define PIN_ENC_B 18
#define PIN_ENC_BTN 8
#define PIN_BUZZER 5
#define PIN_SERVO 10

static const int SECRET[SAFE_CODE_LENGTH] = {1, 2, 3, 4};

void app_main(void) {
	encoder_init(PIN_ENC_A, PIN_ENC_B, PIN_ENC_BTN);
	buzzer_init(PIN_BUZZER);
	servo_init(PIN_SERVO);
	safe_logic_init(SECRET);

	print_prompt();

	int last_dir = 0; // 1 = CW, -1 = CCW

	while (true) {
		encoder_event_t event = encoder_poll();

		if(event == ENCODER_EVENT_BTN_PRESSED) {
			safe_print("[RESET] New try");
			safe_logic_reset_attempt();
			last_dir = 0;
			print_prompt();
		} else if(event == ENCODER_EVENT_ROTATE_CW || event == ENCODER_EVENT_ROTATE_CCW) {
			int current_dir = (event == ENCODER_EVENT_ROTATE_CW) ? 1 : -1;

			// change direction
			if(last_dir != 0 && current_dir != last_dir) {
				if(safe_logic_get_digit_index() == SAFE_CODE_LENGTH - 1) {
					// last number
					safe_status_t status = safe_logic_check();
					
					if (status == SAFE_STATUS_UNLOCKED) {
						safe_print("[SUCCESS] Access granted. Open the lock...");
						servo_set_angle(90);
						buzzer_play_success();

						safe_print("[SUCCESS] The lock is open. Auto-closes in 5 seconds...");
						vTaskDelay(pdMS_TO_TICKS(5000));

						servo_set_angle(0);
						safe_print("[SUCCESS] Lock is closed!\n");
                        
                        safe_logic_reset_attempt();
                        last_dir = 0;
                        print_prompt();
					} else if(status == SAFE_STATUS_BLOCKED) {
						safe_print("[BLOCKED] No more try!");

						servo_set_angle(0);
						buzzer_play_alarm();
						while(1) vTaskDelay(pdMS_TO_TICKS(1000));
					} else {
						safe_print("[ERROR] Wrong pin code!");

						buzzer_play_error();
						safe_logic_reset_attempt();
						last_dir = 0;
						print_prompt();
					}
				} else {
					buzzer_play_tick_down(); // CCW
					safe_logic_confirm_digit();
					last_dir = 0; // Reset the direction for the new number
					print_prompt();
				}
			} else {
				// increment/decrement based on rotation
				safe_logic_increment_digit();

				buzzer_play_tick(); // CW
				
				printf("\b%d", safe_logic_get_current_digit());
                fflush(stdout);

				last_dir = current_dir;
			}
		}

		vTaskDelay(pdMS_TO_TICKS(20));
    }
}
