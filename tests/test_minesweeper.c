#include "minesweeper.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void assert_board(const ms_game_t *g) {
    unsigned mines = 0, flags = 0, opened = 0;
    for (int y = 0; y < g->height; ++y) {
        for (int x = 0; x < g->width; ++x) {
            uint8_t c = ms_cell(g, x, y);
            mines += !!(c & MS_MINE);
            flags += !!(c & MS_FLAG);
            opened += !!((c & MS_OPEN) && !(c & MS_MINE));
            if (!(c & MS_MINE) && g->state != MS_READY) {
                unsigned count = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        count += !!(ms_cell(g, x + dx, y + dy) & MS_MINE);
                assert((c & MS_NUMBER) == count);
            }
            assert(!((c & MS_FLAG) && (c & MS_OPEN)));
        }
    }
    assert(mines == (g->state == MS_READY ? 0 : g->mines));
    assert(flags == g->flags);
    assert(opened == g->opened);
}

static void generation_and_wins(void) {
    for (int level = 0; level < MS_LEVEL_COUNT; ++level) {
        for (unsigned seed = 0; seed < 200; ++seed) {
            ms_game_t g;
            ms_init(&g, (ms_level_t)level, seed);
            int x = seed % g.width, y = (seed / 3) % g.height;
            assert(ms_reveal(&g, x, y));
            assert(ms_cell(&g, x, y) & MS_OPEN);
            assert(!(ms_cell(&g, x, y) & MS_NUMBER));
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    assert(!(ms_cell(&g, x + dx, y + dy) & MS_MINE));
            assert_board(&g);
            // Reveal only hidden safe cells; chording open cells is deliberately avoided.
            for (int cy = 0; cy < g.height; ++cy)
                for (int cx = 0; cx < g.width; ++cx)
                    if (!(ms_cell(&g, cx, cy) & (MS_MINE | MS_OPEN))) ms_reveal(&g, cx, cy);
            assert(g.state == MS_WON && g.flags == g.mines);
            assert_board(&g);
            ms_game_t saved = g;
            assert(!ms_toggle_flag(&g, 0, 0));
            assert(!ms_reveal(&g, 0, 0));
            assert(memcmp(&saved, &g, sizeof(g)) == 0);
        }
    }
}

static ms_game_t chord_fixture(void) {
    ms_game_t g;
    ms_init(&g, MS_BEGINNER, 1);
    g.width = g.height = 3;
    g.mines = 1;
    g.state = MS_PLAYING;
    g.cells[0] = MS_MINE;
    g.cells[1] = g.cells[3] = g.cells[4] = 1;
    ms_reveal(&g, 1, 1);
    return g;
}

static void flags_chord_and_loss(void) {
    ms_game_t g = chord_fixture();
    assert(!ms_reveal(&g, 1, 1)); // no matching flags
    assert(ms_toggle_flag(&g, 0, 0));
    assert(ms_reveal(&g, 1, 1));
    assert(g.state == MS_WON);
    assert_board(&g);
    g = chord_fixture();
    assert(ms_toggle_flag(&g, 1, 0)); // deliberately incorrect flag
    assert(ms_reveal(&g, 1, 1));
    assert(g.state == MS_LOST && g.exploded == 0);
    assert_board(&g);
    ms_game_t saved = g;
    assert(!ms_reveal(&g, 2, 2));
    assert(!ms_toggle_flag(&g, 1, 0));
    assert(memcmp(&saved, &g, sizeof(g)) == 0);

    ms_init(&g, MS_BEGINNER, 2);
    assert(ms_toggle_flag(&g, 4, 4));
    assert(!ms_reveal(&g, 4, 4) && g.state == MS_READY);
    assert(ms_reveal(&g, 3, 3));
    assert(ms_cell(&g, 4, 4) & MS_FLAG); // flood fill preserves a safe flag
    assert(ms_toggle_flag(&g, 4, 4));
    assert(ms_reveal(&g, 4, 4));
    assert_board(&g);
    assert(!ms_toggle_flag(&g, -1, 0));
    assert(!ms_reveal(&g, g.width, 0));
    assert(!ms_toggle_flag(&g, 3, 3)); // open cell

    // Worst-case flood queue occupancy: every cell of the largest board.
    ms_init(&g, MS_EXPERT, 8);
    g.state = MS_PLAYING;
    g.mines = 0;
    assert(ms_reveal(&g, 0, 0));
    assert(g.state == MS_WON && g.opened == MS_MAX_CELLS);
    assert_board(&g);

    ms_init(&g, MS_BEGINNER, 8);
    for (int i = 0; i < 81; ++i) ms_toggle_flag(&g, i % 9, i / 9);
    assert(g.flags == 81); // classic negative remaining-mine counter is permitted
    for (int i = 0; i < 81; ++i) ms_toggle_flag(&g, i % 9, i / 9);
    assert(g.flags == 0);
}

static void fuzz(void) {
    uint32_t rng = 777;
    for (int n = 0; n < 100; ++n) {
        ms_game_t g;
        ms_init(&g, (ms_level_t)(n % 3), (uint32_t)n);
        for (int i = 0; i < 1000; ++i) {
            rng = rng * 1664525u + 1013904223u;
            int x = (rng >> 8) % g.width, y = (rng >> 16) % g.height;
            if (rng & 1) ms_reveal(&g, x, y);
            else ms_toggle_flag(&g, x, y);
            assert_board(&g);
        }
    }
}

static void navigation_and_clock(void) {
    ms_app_t a;
    ms_app_init(&a, 13, 0);
    ms_app_action(&a, MS_PREV, 10);
    assert(a.x == 8 && a.y == 8);
    ms_app_action(&a, MS_NEXT, 20);
    assert(a.x == 0 && a.y == 0);
    ms_app_action(&a, MS_ROW_UP, 30);
    assert(a.y == 8 && a.x == 0);
    ms_app_action(&a, MS_ROW_DOWN, 40);
    assert(a.y == 0);
    ms_app_tick(&a, 500);
    assert(a.elapsed_ms == 0);
    ms_app_action(&a, MS_REVEAL, 1000);
    ms_app_tick(&a, 4000);
    assert(a.elapsed_ms == 3000);
    ms_app_action(&a, MS_MENU, 5000);
    ms_app_tick(&a, 10000);
    assert(a.elapsed_ms == 4000 && a.page == MS_MENU_PAGE);
    ms_app_action(&a, MS_REVEAL, 11000); // resume
    ms_app_tick(&a, 12000);
    assert(a.elapsed_ms == 5000);
    ms_app_action(&a, MS_MENU, 12000);
    ms_app_action(&a, MS_NEXT, 12001);
    ms_app_action(&a, MS_REVEAL, 12002);
    assert(a.page == MS_CONFIRM_PAGE && a.selection == 0);
    ms_app_action(&a, MS_REVEAL, 12003); // default cancels
    assert(a.page == MS_MENU_PAGE && a.elapsed_ms == 5000);
    ms_app_action(&a, MS_NEXT, 12004);
    ms_app_action(&a, MS_NEXT, 12005);
    ms_app_action(&a, MS_REVEAL, 12006); // difficulty
    ms_app_action(&a, MS_PREV, 12007); // expert
    ms_app_action(&a, MS_REVEAL, 12008);
    assert(a.page == MS_CONFIRM_PAGE);
    ms_app_action(&a, MS_NEXT, 12009);
    ms_app_action(&a, MS_REVEAL, 12010);
    assert(a.game.level == MS_EXPERT && a.elapsed_ms == 0);
    for (int i = 0; i < 479; ++i) ms_app_action(&a, MS_NEXT, 13000 + i);
    assert(a.x == 29 && a.y == 15 && a.view_x == 21 && a.view_y == 7);
    ms_app_action(&a, MS_NEXT, 14000);
    assert(a.x == 0 && a.y == 0 && a.view_x == 0 && a.view_y == 0);

    ms_app_init(&a, 1, 0);
    ms_app_action(&a, MS_REVEAL, 1000);
    ms_app_tick(&a, 121000); // deliberately late tick: only 60s counted
    assert(a.elapsed_ms == 60000 && a.idle && a.page == MS_MENU_PAGE);
    assert(ms_app_brightness(&a, 121000) == 0);
    ms_app_action(&a, MS_WAKE, 122000);
    ms_app_action(&a, MS_REVEAL, 122400); // tail of wake click must be discarded
    assert(a.page == MS_MENU_PAGE && !a.idle);
    ms_app_action(&a, MS_REVEAL, 123000);
    assert(a.page == MS_GAME_PAGE);
    ms_app_tick(&a, 124000);
    assert(a.elapsed_ms == 61000);
    a.elapsed_ms = 1000000;
    assert(ms_app_seconds(&a) == 999);
    a.game.state = MS_LOST;
    ms_app_tick(&a, 125000);
    assert(a.elapsed_ms == 1000000);
    ms_app_tick(&a, 1); // backwards timestamp is ignored
    assert(a.last_tick_ms == 125000);
}

int main(void) {
    generation_and_wins();
    flags_chord_and_loss();
    navigation_and_clock();
    fuzz();
    puts("Minesweeper: PASS (600 boards, 100000 actions, rules/navigation/time)");
}
