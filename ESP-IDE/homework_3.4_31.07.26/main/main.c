#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "hal/ledc_types.h"
#include "soc/clk_tree_defs.h"

#define BUZZER_GPIO GPIO_NUM_18
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT // 0...1023
#define LEDC_DUTY_50PCT 512				// 50%
#define LEDC_BASE_FREQ_HZ 2000

// player tick
#define TICK_MS 50
#define TICK_US (TICK_MS * 1000)

#define BTN_GPIO GPIO_NUM_8
#define BTN_DEBOUNCE_MS 30

static const char *TAG = "buzzer";

typedef enum {
	NOTE_REST = 0,
	NOTE_C4 = 262,
	NOTE_D4 = 294,
	NOTE_E4 = 330,
	NOTE_F4 = 349,
	NOTE_G4 = 392,
	NOTE_A4 = 440,
	NOTE_B4 = 494,
	NOTE_C5 = 523,
} note_freq_t;

typedef struct {
	uint16_t freq;
	uint8_t ticks;
} note_t;

// Baby Shark: C D E | C D E | C D E
static const note_t song_baby_shark[] = {
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_D4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 1},
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_D4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 1},
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_D4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 4},
};
 
// Jingle Bells: E E E | E E E | E G C D E
static const note_t song_jingle_bells[] = {
    {NOTE_E4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 1}, {NOTE_E4, 8}, {NOTE_REST, 1},
    {NOTE_E4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 1}, {NOTE_E4, 8}, {NOTE_REST, 1},
    {NOTE_E4, 4}, {NOTE_REST, 1}, {NOTE_G4, 4}, {NOTE_REST, 1}, {NOTE_C4, 4}, {NOTE_REST, 1},
    {NOTE_D4, 4}, {NOTE_REST, 1}, {NOTE_E4, 8}, {NOTE_REST, 4},
};
 
// We Will Rock You: C C D | C C D | C C D E
static const note_t song_we_will_rock_you[] = {
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_D4, 8}, {NOTE_REST, 1},
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_D4, 8}, {NOTE_REST, 1},
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_D4, 4}, {NOTE_REST, 1},
    {NOTE_E4, 8}, {NOTE_REST, 4},
};
 
// Twinkle Twinkle Little Star (спрощена): C C D D | E E D
static const note_t song_twinkle[] = {
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_C4, 4}, {NOTE_REST, 1},
    {NOTE_D4, 4}, {NOTE_REST, 1}, {NOTE_D4, 4}, {NOTE_REST, 1},
    {NOTE_E4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 1},
    {NOTE_D4, 8}, {NOTE_REST, 4},
};
 
// Old MacDonald (спрощена): C C C D E E D
static const note_t song_old_macdonald[] = {
    {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_C4, 4}, {NOTE_REST, 1}, {NOTE_C4, 4}, {NOTE_REST, 1},
    {NOTE_D4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 1}, {NOTE_E4, 4}, {NOTE_REST, 1},
    {NOTE_D4, 8}, {NOTE_REST, 4},
};

// Playlist loop
typedef struct {
    const note_t *notes;
    size_t        length;
    const char   *name;
} melody_t;

#define SONG(arr, title) {arr, sizeof(arr) / sizeof(arr[0]), title}

static const melody_t playlist[] = {
	SONG(song_baby_shark,        "Baby Shark"),
    SONG(song_jingle_bells,      "Jingle Bells"),
    SONG(song_we_will_rock_you,  "We Will Rock You"),
    SONG(song_twinkle,           "Twinkle Twinkle Little Star"),
    SONG(song_old_macdonald,     "Old MacDonald"),
};

static const size_t playlist_len = sizeof(playlist) / sizeof(playlist[0]);

#define SONG_GAP_MS 800 // melodies pause


//
typedef struct {
	const note_t *melody;
	size_t length;
	size_t index;
	uint8_t ticks_left;
	bool playing;
	int64_t last_tick_us;
} buzzer_player_t;

static buzzer_player_t player = {0};

static void buzzer_set_freq(uint16_t freq) {
	if(freq == NOTE_REST || freq == 0) {
		// silence: turn off PWM (duty = 0), timer frequency does not matter
		ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, 0);
		ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
	} else {
		ledc_set_freq(LEDC_MODE, LEDC_TIMER, freq);
		ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_50PCT);
		ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
	}
}

static void buzzer_init(void){
	ledc_timer_config_t timer_cfg = {
		.speed_mode = LEDC_MODE,
		.timer_num = LEDC_TIMER,
		.duty_resolution = LEDC_DUTY_RES,
		.freq_hz = LEDC_BASE_FREQ_HZ,
		.clk_cfg = LEDC_AUTO_CLK
	};
	ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

	ledc_channel_config_t channel_cfg = {
		.gpio_num = BUZZER_GPIO,
		.speed_mode = LEDC_MODE,
		.channel = LEDC_CHANNEL,
		.intr_type = LEDC_INTR_DISABLE,
		.timer_sel = LEDC_TIMER,
		.duty = 0,
		.hpoint = 0,
	};
	ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
}

static void button_init(void) {
	gpio_config_t io_cfg = {
		.pin_bit_mask = (1ULL << BTN_GPIO),
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE, // polling in the loop
	};
	ESP_ERROR_CHECK(gpio_config(&io_cfg));
}

static void player_play(const note_t *m, size_t len) {
	player.melody = m;
	player.length = len;
	player.index = 0;
	player.ticks_left = 0;
	player.playing = (len > 0);
	player.last_tick_us = esp_timer_get_time();
}

static void player_stop(void){
	player.playing = false;
	buzzer_set_freq(NOTE_REST);
}

static void player_update(void){
	if(!player.playing) {
		return;
	}

	int64_t now = esp_timer_get_time();
	if(now - player.last_tick_us < TICK_US) {
		return; // not started tick yet
	}
	player.last_tick_us += TICK_US;

	if(player.ticks_left == 0) {
		if(player.index >= player.length) {
			player_stop();
			return;
		}
		const note_t *n = &player.melody[player.index++];
		player.ticks_left = n->ticks;
		buzzer_set_freq(n->freq);
	}

	player.ticks_left--;
}

static void next_song(size_t *current_song)
{
    *current_song = (*current_song + 1) % playlist_len;
    ESP_LOGI(TAG, "Playing: %s", playlist[*current_song].name);
    player_play(playlist[*current_song].notes, playlist[*current_song].length);
}

void app_main(void) {
	buzzer_init();
	button_init();

	size_t current_song = 0;
	bool waiting_gap = false;
	int64_t gap_start_us = 0;

	int btn_raw_last = 1;
	int btn_stable = 1;
	int64_t btn_change_us = 0;

	ESP_LOGI(TAG, "Playing: %s", playlist[current_song].name);
	player_play(playlist[current_song].notes, playlist[current_song].length);

	int64_t last_log_us = esp_timer_get_time();

	while (true) {
		player_update();

		int64_t now = esp_timer_get_time();

		int btn_raw = gpio_get_level(BTN_GPIO);
		if (btn_raw != btn_raw_last) {
			btn_raw_last = btn_raw;
			btn_change_us = now;
		} else if(btn_raw != btn_stable && now - btn_change_us >= (int64_t)BTN_DEBOUNCE_MS * 1000) {
			btn_stable = btn_raw;
			if (btn_stable == 0) {
				// switch immediately, without a pause SONG_GAP_MS
				waiting_gap = false;
				next_song(&current_song);
			}
		}
		
		
		if(now - last_log_us >= 1000000) { // every 1s
			last_log_us = now;
			ESP_LOGI(TAG, "Working: %lld ms, playing: %s",
			                    now / 1000, player.playing ? "YES" : "NO");
		}
		
		if (!player.playing) {
			if(!waiting_gap) {
				waiting_gap = true;
				gap_start_us = now;
			} else if(now - gap_start_us >= (int64_t)SONG_GAP_MS *1000) {
				waiting_gap = false;
				player_play(playlist[current_song].notes, playlist[current_song].length);

			}
		}

		vTaskDelay(1); // stop trigger watchdog
    }
}
