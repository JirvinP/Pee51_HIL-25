#pragma once

#include "ems.h"

enum {
    LOG_FAIL,
    LOG_OK,
    LOG_NOTE,
    LOG_WARN
};

void logprint(int level, char* text);
void parse_user_optimization_strategy(struct system* sys);
struct system* initialize_sys(void);
void wait_for_ship_data(struct system* sys);
void clear_screen(void);
void print_stats(struct system* sys);
