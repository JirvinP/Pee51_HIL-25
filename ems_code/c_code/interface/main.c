#include "ems.h"
#include "ui.h"

#include <stdio.h>

struct system* sys;

void schedule_setpoints(void)
{
    rate_limit();

    while (true)
    {
        clear_screen();
        print_stats(sys);
        parse_simulation_data();
        execute_subroutine(sys);
        rate_limit();
    }
}


int main(void){
    printf("EMS Interfacinator\n");
    sys = initialize_sys();
    parse_user_optimization_strategy(sys);
    wait_for_ship_data(sys);
    schedule_setpoints();

    return 0;
}
