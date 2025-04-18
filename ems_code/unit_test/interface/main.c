#include "ems.h"
#include "ui.h"
#include "UARTqueue.h"

#include <stdio.h>

struct system* sys;
struct queue* qu;

void schedule_setpoints(void)
{
    rate_limit();

    while (true)
    {
        clear_screen(qu);
        print_stats(sys, qu);
        parse_simulation_data(sys);
        execute_subroutine(sys);

        if (!is_full(qu))
            printf("%s", delet(qu));

        rate_limit();
    }
}


int main(void){
    printf("EMS Interfacinator\n");
    qu = construct_queue();
    sys = initialize_sys(qu);
    parse_user_optimization_strategy(sys, "0");
    wait_for_ship_data(sys, qu);
    schedule_setpoints();

    return 0;
}
