#include "spiQueue.h"
#include "ems.h"

#include <check.h>
#include <stdio.h>
#include <stdlib.h>

// zorg dat libcheck is geinstalleerd
// build met cmake

START_TEST (spi_q) {
    struct system* sys = construct_sys();
    sys->user_setting = SOC;

    // set up buffers
    struct structSpiQueue* spiQueueTransmit = NULL;
    struct structSpiQueue* spiQueueReceive = NULL;
    spiQueueTransmit = spiQueueCreate(20);
    spiQueueReceive = spiQueueCreate(20);
    ck_assert_ptr_nonnull(spiQueueTransmit);
    ck_assert_ptr_nonnull(spiQueueReceive);

    // populate rx buffer with arbitrary data
    spiQueuePostDec(spiQueueReceive, POWER_BATTERY1_ID, 69);        // double
    spiQueuePostDec(spiQueueReceive, POWER_BATTERY2_ID, 420);
    spiQueuePostDec(spiQueueReceive, SOC_BATTERY1_ID, 27);          // float
    spiQueuePostDec(spiQueueReceive, SOC_BATTERY2_ID, 27);
    spiQueuePostInt(spiQueueReceive, POWER_DG1_ID, 666);            // uint32
    spiQueuePostInt(spiQueueReceive, POWER_DG2_ID, 555);
    spiQueuePostDec(spiQueueReceive, SFOC_DG1_ID, 99);              // float
    spiQueuePostDec(spiQueueReceive, SFOC_DG2_ID, 11);
    spiQueuePostInt(spiQueueReceive, CURRENT_MODE_ID, SAIL_FULL);   // uint8

    // parse data from buffer into sys
    while (spiQueueReceive->sizeCurrent > 0) {
        parse_simulation_data(sys, spiQueueReceive->headFramePtr);
        spiQueueFrameRemove(spiQueueReceive);
    }

    // validate sys struct against data input into rx buffer
    ck_assert_double_eq(sys->power_battery[0], 69.f);
    ck_assert_double_eq(sys->power_battery[1], 420.f);
    ck_assert_float_eq(sys->battery_soc[0], 27.f);
    ck_assert_float_eq(sys->battery_soc[1], 27.f);
    ck_assert_uint_eq(sys->power_dg[0], 666);
    ck_assert_uint_eq(sys->power_dg[1], 555);
    ck_assert_float_eq(sys->fuel_efficiency[0], 99.f);
    ck_assert_float_eq(sys->fuel_efficiency[1], 11.f);
    ck_assert_uint_eq(sys->goat_preference->mode, SAIL_FULL);

    // calucalate setpoints
    execute_subroutine(sys);
    send_setpoints(sys, spiQueueTransmit);

    // validata data inside tx buffer against output from calculated data
    struct structFrame* idx = spiQueueTransmit->headFramePtr;
    ck_assert_int_eq(idx->identifier, SETPOINT_BATTERY1_ID);
    ck_assert_int_eq(idx->payload.sint32, -650);
    idx = idx->nextFramePtr;
    ck_assert_int_eq(idx->identifier, SETPOINT_BATTERY2_ID);
    ck_assert_int_eq(idx->payload.sint32, -650);
    idx = idx->nextFramePtr;
    ck_assert_int_eq(idx->identifier, SETPOINT_DG1_ID);
    ck_assert_uint_eq(idx->payload.sint32, 1900);
    idx = idx->nextFramePtr;
    ck_assert_int_eq(idx->identifier, SETPOINT_DG2_ID);
    ck_assert_uint_eq(idx->payload.sint32, 1900);

    // cleanup
    spiQueueRemove(spiQueueReceive);
    spiQueueRemove(spiQueueTransmit);
}


int main(void) {
    // set up suite
    Suite *suite = suite_create("All");
    SRunner *sr = srunner_create(suite);

    // add tests for spi queues to suite
    TCase *tcSPIQueues = tcase_create("SPIQueues");
    suite_add_tcase(suite, tcSPIQueues);
    tcase_add_test(tcSPIQueues, spi_q);

    // set up runner and run suite
    int nf;
    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}
