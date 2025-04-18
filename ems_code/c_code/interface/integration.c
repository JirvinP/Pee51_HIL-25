#include <stdio.h>
#include "ems.h"
#include "interface.h"

struct system* sys = NULL;
struct structBuffer* rx_buffer = NULL;



int main(){

    sys = construct_sys();

    test_fill(sys);

    execute_subroutine(sys);


    printf("ems mode: %u\n", sys->user_setting);
    printf("ship mode: %u\n", sys->goat_preference->mode);

    sys->battery_soc[0] = 25.0;

    execute_subroutine(sys);

    sys->battery_soc[1] = 25.0;

    execute_subroutine(sys);

    sys->goat_preference->mode = RAINBOW;

    execute_subroutine(sys);

    sys->goat_preference->mode = SAIL_EMPTY;

    execute_subroutine(sys);

    rx_buffer = bufferCreate(100);

    send_setpoints(sys, rx_buffer);

    return 0;
}
