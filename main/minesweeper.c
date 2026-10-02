#include "minesweeper.h"

#include <string.h>

static bool inside(const ms_game_t *g, int x, int y) {
    return x >= 0 && y >= 0 && x < g->width && y < g->height;
}

static int index_of(const ms_game_t *g, int x, int y) {
    return y * g->width + x;
}

uint8_t ms_cell(const ms_game_t *g, int x, int y) {
    return inside(g, x, y) ? g->cells[index_of(g, x, y)] : 0;
}

static uint32_t random_u32(ms_game_t *g) {
    uint32_t x = g->rng;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return g->rng = x;
}

/* Rejection sampling avoids modulo bias; no radio or heap allocation needed. */
static uint32_t random_below(ms_game_t *g, uint32_t n) {
    uint32_t r, threshold = (uint32_t)(-n) % n;
    do { r = random_u32(g); } while (r < threshold);
    return r % n;
}

void ms_init(ms_game_t *g, ms_level_t level, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    if (level < MS_BEGINNER || level >= MS_LEVEL_COUNT) level = MS_BEGINNER;
    g->level = level;
    g->width = level == MS_BEGINNER ? 9 : level == MS_INTERMEDIATE ? 16 : 30;
    g->height = level == MS_BEGINNER ? 9 : 16;
    g->mines = level == MS_BEGINNER ? 10 : level == MS_INTERMEDIATE ? 40 : 99;
    g->rng = seed ? seed : 0x91e10da5u;
    g->exploded = -1;
    g->state = MS_READY;
}

static void generate(ms_game_t *g, int safe_x, int safe_y) {
    uint16_t candidates[MS_MAX_CELLS];
    unsigned count = 0;
    for (int y = 0; y < g->height; ++y) {
        for (int x = 0; x < g->width; ++x) {
            if (x >= safe_x - 1 && x <= safe_x + 1 &&
                y >= safe_y - 1 && y <= safe_y + 1) continue;
            candidates[count++] = (uint16_t)index_of(g, x, y);
        }
    }
    for (unsigned i = 0; i < g->mines; ++i) {
        unsigned pick = i + random_below(g, count - i);
        uint16_t swap = candidates[i];
        candidates[i] = candidates[pick];
        candidates[pick] = swap;
        g->cells[candidates[i]] |= MS_MINE;
    }
    for (int y = 0; y < g->height; ++y) {
        for (int x = 0; x < g->width; ++x) {
            int at = index_of(g, x, y);
            if (g->cells[at] & MS_MINE) continue;
            unsigned adjacent = 0;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    adjacent += !!(ms_cell(g, x + dx, y + dy) & MS_MINE);
            g->cells[at] |= (uint8_t)adjacent;
        }
    }
    g->state = MS_PLAYING;
}

/* Iterative flood fill: mark on enqueue, so each cell enters the bounded
 * queue at most once. Flagged cells remain untouched, including safe flags. */
static void open_area(ms_game_t *g, int x, int y) {
    if (!inside(g, x, y)) return;
    int first = index_of(g, x, y);
    if (g->cells[first] & (MS_OPEN | MS_FLAG)) return;
    if (g->cells[first] & MS_MINE) {
        g->cells[first] |= MS_OPEN;
        g->exploded = (int16_t)first;
        g->state = MS_LOST;
        return;
    }
    uint16_t queue[MS_MAX_CELLS];
    unsigned head = 0, tail = 0;
    g->cells[first] |= MS_OPEN;
    g->opened++;
    queue[tail++] = (uint16_t)first;
    while (head < tail) {
        int at = queue[head++];
        if (g->cells[at] & MS_NUMBER) continue;
        int cx = at % g->width, cy = at / g->width;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (!inside(g, cx + dx, cy + dy)) continue;
                int neighbor = index_of(g, cx + dx, cy + dy);
                if (g->cells[neighbor] & (MS_OPEN | MS_FLAG | MS_MINE)) continue;
                g->cells[neighbor] |= MS_OPEN;
                g->opened++;
                queue[tail++] = (uint16_t)neighbor;
            }
        }
    }
}

static void check_win(ms_game_t *g) {
    if (g->state != MS_PLAYING ||
        g->opened != g->width * g->height - g->mines) return;
    g->state = MS_WON;
    for (int i = 0; i < g->width * g->height; ++i)
        if (g->cells[i] & MS_MINE) g->cells[i] |= MS_FLAG;
    g->flags = g->mines;
}

bool ms_reveal(ms_game_t *g, int x, int y) {
    if (!inside(g, x, y) || g->state == MS_LOST || g->state == MS_WON) return false;
    uint8_t cell = ms_cell(g, x, y);
    if (cell & MS_FLAG) return false;
    if (g->state == MS_READY) generate(g, x, y);
    unsigned before = g->opened;
    if (cell & MS_OPEN) {
        unsigned flags = 0;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx)
                flags += !!(ms_cell(g, x + dx, y + dy) & MS_FLAG);
        if (!(cell & MS_NUMBER) || flags != (cell & MS_NUMBER)) return false;
        for (int dy = -1; dy <= 1 && g->state != MS_LOST; ++dy)
            for (int dx = -1; dx <= 1 && g->state != MS_LOST; ++dx)
                open_area(g, x + dx, y + dy);
    } else {
        open_area(g, x, y);
    }
    check_win(g);
    return before != g->opened || g->state == MS_LOST;
}

bool ms_toggle_flag(ms_game_t *g, int x, int y) {
    if (!inside(g, x, y) || g->state == MS_WON || g->state == MS_LOST) return false;
    uint8_t *cell = &g->cells[index_of(g, x, y)];
    if (*cell & MS_OPEN) return false;
    if (*cell & MS_FLAG) { *cell &= (uint8_t)~MS_FLAG; g->flags--; }
    else { *cell |= MS_FLAG; g->flags++; }
    return true;
}

static void keep_visible(ms_app_t *a) {
    if (a->x < a->view_x) a->view_x = a->x;
    if (a->x >= a->view_x + MS_VIEW_SIZE) a->view_x = a->x - MS_VIEW_SIZE + 1;
    if (a->y < a->view_y) a->view_y = a->y;
    if (a->y >= a->view_y + MS_VIEW_SIZE) a->view_y = a->y - MS_VIEW_SIZE + 1;
}

static void new_game(ms_app_t *a, ms_level_t level, uint64_t now) {
    /* Each new game uses fresh entropy supplied by the application worker. */
    ms_init(&a->game, level, a->next_seed);
    a->x = a->y = a->view_x = a->view_y = 0;
    a->elapsed_ms = 0;
    a->last_tick_ms = now;
    a->page = MS_GAME_PAGE;
}

void ms_app_init(ms_app_t *a, uint32_t seed, uint64_t now) {
    memset(a, 0, sizeof(*a));
    a->next_seed = seed;
    a->last_input_ms = now;
    new_game(a, MS_BEGINNER, now);
}

void ms_app_tick(ms_app_t *a, uint64_t now) {
    if (now < a->last_tick_ms) return;
    if (!a->idle && a->page == MS_GAME_PAGE && a->game.state == MS_PLAYING) {
        /* Stop precisely at the idle deadline, even when a tick is delayed. */
        uint64_t end = now < a->last_input_ms + 60000 ? now : a->last_input_ms + 60000;
        if (end > a->last_tick_ms) a->elapsed_ms += end - a->last_tick_ms;
    }
    a->last_tick_ms = now;
    if (!a->idle && now - a->last_input_ms >= 60000) {
        a->idle = true;
        if (a->page == MS_GAME_PAGE) {
            a->page = MS_MENU_PAGE;
            a->selection = 0;
        }
    }
}

uint8_t ms_app_brightness(const ms_app_t *a, uint64_t now) {
    if (!a->idle) return 80;
    return now - a->last_input_ms >= 120000 ? 0 : 15;
}

uint16_t ms_app_seconds(const ms_app_t *a) {
    return a->elapsed_ms / 1000 > 999 ? 999 : (uint16_t)(a->elapsed_ms / 1000);
}

void ms_app_action(ms_app_t *a, ms_action_t action, uint64_t now) {
    ms_app_tick(a, now);
    if (a->idle) {
        a->idle = false;
        a->last_input_ms = now;
        a->ignore_until_ms = now + 800;
        return; // Wake gesture never reveals a cell or confirms a menu item.
    }
    if (now < a->ignore_until_ms) return;
    a->last_input_ms = now;
    if (action == MS_WAKE) return;
    if (a->page == MS_HELP_PAGE) {
        if (action == MS_REVEAL || action == MS_MENU) {
            a->page = MS_MENU_PAGE;
            a->selection = 3;
        }
    } else if (a->page == MS_CONFIRM_PAGE) {
        if (action == MS_PREV || action == MS_NEXT) a->selection ^= 1;
        if (action == MS_MENU) { a->page = MS_MENU_PAGE; a->selection = 0; }
        if (action == MS_REVEAL) {
            if (a->selection == 0) { a->page = MS_MENU_PAGE; a->selection = 0; }
            else new_game(a, a->pending_level, now);
        }
    } else if (a->page == MS_LEVEL_PAGE) {
        if (action == MS_PREV) a->selection = (a->selection + 2) % MS_LEVEL_COUNT;
        if (action == MS_NEXT) a->selection = (a->selection + 1) % MS_LEVEL_COUNT;
        if (action == MS_MENU) { a->page = MS_MENU_PAGE; a->selection = 2; }
        if (action == MS_REVEAL) {
            a->pending_level = (ms_level_t)a->selection;
            if (a->game.state == MS_PLAYING) { a->page = MS_CONFIRM_PAGE; a->selection = 0; }
            else new_game(a, a->pending_level, now);
        }
    } else if (a->page == MS_MENU_PAGE) {
        if (action == MS_PREV) a->selection = (a->selection + 3) % 4;
        if (action == MS_NEXT) a->selection = (a->selection + 1) % 4;
        if (action == MS_MENU) a->page = MS_GAME_PAGE;
        if (action == MS_REVEAL) {
            switch (a->selection) {
                case 0: a->page = MS_GAME_PAGE; break;
                case 1:
                    a->pending_level = a->game.level;
                    if (a->game.state == MS_PLAYING) { a->page = MS_CONFIRM_PAGE; a->selection = 0; }
                    else new_game(a, a->game.level, now);
                    break;
                case 2: a->page = MS_LEVEL_PAGE; a->selection = a->game.level; break;
                case 3: a->page = MS_HELP_PAGE; break;
            }
        }
    } else {
        int at = a->y * a->game.width + a->x;
        int total = a->game.width * a->game.height;
        if (action == MS_PREV || action == MS_NEXT) {
            at = (at + (action == MS_PREV ? total - 1 : 1)) % total;
            a->x = at % a->game.width;
            a->y = at / a->game.width;
        }
        if (action == MS_ROW_UP) a->y = (a->y + a->game.height - 1) % a->game.height;
        if (action == MS_ROW_DOWN) a->y = (a->y + 1) % a->game.height;
        if (action == MS_REVEAL) ms_reveal(&a->game, a->x, a->y);
        if (action == MS_TOGGLE_FLAG) ms_toggle_flag(&a->game, a->x, a->y);
        if (action == MS_MENU) { a->page = MS_MENU_PAGE; a->selection = 0; }
        keep_visible(a);
    }
}
