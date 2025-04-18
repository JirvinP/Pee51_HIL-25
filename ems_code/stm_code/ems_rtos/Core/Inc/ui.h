#pragma once

#include "ems.h"
#include "UARTqueue.h"

// forward declaration "ems.h"
struct system;

enum {
	LOG_FAIL,
	LOG_OK,
	LOG_NOTE,
	LOG_WARN
};

void logprint(int level, char* text, struct queue* qu);
int parse_user_optimization_strategy(struct system* sys, char* c);
struct system* initialize_sys(struct queue* qu);
void wait_for_ship_data(struct system* sys, struct queue* qu);
void clear_screen(struct queue* qu);
void print_stats(struct system* sys, struct queue* qu);
void print_choice_menu(struct queue* qu);
