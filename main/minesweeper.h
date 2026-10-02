#pragma once

#include <stdbool.h>
#include <stdint.h>

#define MS_MAX_CELLS 480
#define MS_VIEW_SIZE 9
#define MS_MINE 0x10
#define MS_FLAG 0x20
#define MS_OPEN 0x40
#define MS_NUMBER 0x0f

typedef enum { MS_BEGINNER, MS_INTERMEDIATE, MS_EXPERT, MS_LEVEL_COUNT } ms_level_t;
typedef enum { MS_READY, MS_PLAYING, MS_WON, MS_LOST } ms_state_t;

typedef struct {
    uint8_t cells[MS_MAX_CELLS];
    uint8_t width, height;
    uint16_t mines, flags, opened;
    int16_t exploded;
    uint32_t rng;
    ms_state_t state;
    ms_level_t level;
} ms_game_t;

void ms_init(ms_game_t *game, ms_level_t level, uint32_t seed);
/* The first reveal generates mines outside the selected 3x3 neighborhood.
 * Revealing an open number chords only when its neighboring flags match. */
bool ms_reveal(ms_game_t *game, int x, int y);
bool ms_toggle_flag(ms_game_t *game, int x, int y);
uint8_t ms_cell(const ms_game_t *game, int x, int y);

typedef enum { MS_GAME_PAGE, MS_MENU_PAGE, MS_LEVEL_PAGE, MS_HELP_PAGE,
               MS_CONFIRM_PAGE } ms_page_t;
typedef enum { MS_PREV, MS_NEXT, MS_ROW_UP, MS_ROW_DOWN, MS_REVEAL,
               MS_TOGGLE_FLAG, MS_MENU, MS_WAKE } ms_action_t;

typedef struct {
    ms_game_t game;
    ms_page_t page;
    int x, y, view_x, view_y, selection;
    uint64_t elapsed_ms, last_tick_ms, last_input_ms, ignore_until_ms;
    uint32_t next_seed;
    ms_level_t pending_level;
    bool idle;
} ms_app_t;

void ms_app_init(ms_app_t *app, uint32_t seed, uint64_t now_ms);
void ms_app_tick(ms_app_t *app, uint64_t now_ms);
void ms_app_action(ms_app_t *app, ms_action_t action, uint64_t now_ms);
uint8_t ms_app_brightness(const ms_app_t *app, uint64_t now_ms);
uint16_t ms_app_seconds(const ms_app_t *app);
