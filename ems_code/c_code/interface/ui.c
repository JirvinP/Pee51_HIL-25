#include "ui.h"

#include <stdio.h>
#include <assert.h>

extern struct ship_state_subroutines subroutines[];

char STRING_KEUS[] =
    "Which optimization strategy should be used? Type and enter\n"
    "0: Inefficient\n"
    "1: Prioritise battery SOC\n"
    "2: Prioritise diesel efficiency\n"
    "> ";


void logprint(int level, char* text)
{
    char* formatted_level;

    if (level == LOG_FAIL)
        formatted_level = "\033[0;31mfail\033[0m";
    else if (level == LOG_OK)
        formatted_level = "\033[0;32m ok \033[0m";
    else if (level == LOG_WARN)
        formatted_level = "\033[0;33mwarn\033[0m";
    else
        formatted_level = "note";

    printf("[%s] %s\n", formatted_level, text);
}


void parse_user_optimization_strategy(struct system* sys)
{
    unsigned choice;
    printf("%s", STRING_KEUS);
    scanf("%u", &choice);

    if (choice > 2)
        parse_user_optimization_strategy(sys);

    sys->user_setting = choice;

    return;
}


struct system* initialize_sys(void)
{
    struct system* sys = construct_sys();
    assert(sys != NULL);
    logprint(LOG_OK, "System struct has been initialized.");

    return sys;
}


void wait_for_ship_data(struct system* sys)
{
    logprint(LOG_NOTE, "Waiting for data from ship...\n");

    while (sys->goat_preference->mode == INIT)
    {
        logprint(LOG_WARN, "placeholder: putting mode into `bunkering`\n");
        sys->goat_preference->mode = BUNKERING;
    }
}


void clear_screen(void)
{
    printf("\033[2J" "\033[H");
}


void print_stats(struct system* sys)
{
    size_t index = sys->goat_preference->mode - 1;
    char* routine_name = subroutines[index].name;
    printf("Ship state:     \t%s\n", routine_name);
    printf("SOC battery:    \t%.1f%%, \t%.1f%%\n",
           sys->battery_soc[0],
           sys->battery_soc[1]);
    printf("Fuel efficiency: \t%.1f%%, \t%.1f%%\n",
           sys->fuel_efficiency[0],
           sys->fuel_efficiency[1]);
    printf("Power DG (kW):   \t%u, \t%u\n",
           sys->power_dg[0],
           sys->power_dg[1]);
    printf("Power battery (kW): \t%d, \t%d\n",
           sys->power_battery[0],
           sys->power_battery[1]);
}
