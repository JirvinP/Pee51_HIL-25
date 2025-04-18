#include "ui.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

extern struct ship_state_subroutines subroutines[];
uint32_t latencyStored;
uint8_t latencyAnimator;

char STRING_KEUS[] =
	"Which optimization strategy should be used? Type and enter\r\n"
	"1: Inefficient\r\n"
	"2: Prioritise battery SOC\r\n"
	"3: Prioritise diesel efficiency\r\n"
	"> ";

void print_choice_menu(struct queue* qu) {
	enqueue(qu, STRING_KEUS);
}

void logprint(int level, char* text, struct queue* qu) {
	char log_msg[150] = {'\0'};
	switch (level) {
	case LOG_FAIL:
		snprintf(log_msg, 128, "[\033[31m FAIL \033[0m] %s", text);
		break;
	case LOG_OK:
		snprintf(log_msg, 128, "[\033[32m OK \033[0m] %s", text);
		break;
	case LOG_WARN:
		snprintf(log_msg, 128, "[\033[33m WARN \033[0m] %s", text);
		break;
	default:
		snprintf(log_msg, 128, "[note] %s", text);
		break;
	}
	enqueue(qu, log_msg);
}

int parse_user_optimization_strategy(struct system* sys, char* c) {
	unsigned int choice = 0;
	sscanf(c, "%u", &choice);
	if (choice < 1 || choice > 3) {
		return -1;
	}
	sys->user_setting = choice;
	return 0;
}

struct system* initialize_sys(struct queue* qu) {
	struct system* sys = construct_sys();
	assert(sys != NULL);
	logprint(LOG_OK, "System struct has been initialized\r\n", qu);
	return sys;
}

void wait_for_ship_data(struct system* sys, struct queue* qu) {
	logprint(LOG_NOTE, "Waiting for data from ship...\r\n", qu);


	while (sys->goat_preference->mode == INIT) {
		logprint(LOG_WARN, "placeholder: putting mode into `bunkering`\r\n", qu);
		sys->goat_preference->mode = BUNKERING;
	}
}

void clear_screen(struct queue* qu) {
	enqueue(qu, "\033[2J\033[H");
}

void print_stats(struct system* sys, struct queue* qu) {
	size_t index = sys->goat_preference->mode - 1;
	char to_send[150] = {'\0'};
	char* routine_name = (sys->goat_preference->mode == INIT || sys->goat_preference->mode == 0) ? "ship initializing..." : subroutines[index].name;
	clear_screen(qu);
	snprintf(to_send, 150, "Ship state:\t\t %s\r\n", routine_name);
	enqueue(qu, to_send);

	memset(to_send, '\0', 150);
	snprintf(to_send, 150, "SOC battery:\t\t%12.1f%%,\t%12.1f%%\r\n", sys->battery_soc[0], sys->battery_soc[1]);
	enqueue(qu, to_send);

	memset(to_send, '\0', 150);
	snprintf(to_send, 150, "SFOC (g/kWh):\t\t%12.1f,\t%12.1f\r\n", sys->fuel_efficiency[0], sys->fuel_efficiency[1]);
	enqueue(qu, to_send);

	memset(to_send, '\0', 150);
    snprintf(to_send, 150, "Power DG (kW):\t\t%12u,\t%12u\r\n", sys->power_dg[0], sys->power_dg[1]);
	enqueue(qu, to_send);

	memset(to_send, '\0', 150);
	snprintf(to_send, 150, "Power battery (kW)\t%12.1f,\t%12.1f\r\n", sys->power_battery[0], sys->power_battery[1]);
	enqueue(qu, to_send);

	char spinny;
	switch (latencyAnimator) {
		case 0:
			spinny = '-';
			break;
		case 1:
			spinny = '\\';
			break;
		case 2:
			spinny = '|';
			break;
		case 3:
			spinny = '/';
			break;
		default:
			break;
	}
	memset(to_send, '\0', 150);
	snprintf(to_send, 150, "Latency:\t\t%12.1fms%c\r\n", ((double)latencyStored/10.0), spinny);
	enqueue(qu, to_send);
}
