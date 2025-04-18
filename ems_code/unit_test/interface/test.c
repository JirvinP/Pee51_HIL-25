#include "UARTqueue.h"
#include "ems.h"
#include <check.h>

#include <stdio.h>
#include <stdlib.h>

/* zorg dat libcheck is geinstalleerd
 * how2: cc test.c ems.c UARTqueue.c -lcheck && ./a.out
 */


START_TEST (q_basic) {
    struct queue* q = construct_queue();
    // fi 1
    enqueue(q, "test1");

    // fo 2
    ck_assert_str_eq("test1", (char*)  delet(q));

    // assert no output
    ck_assert_ptr_eq(NULL, (char*)  delet(q));

    destruct_queue(q);
}
END_TEST


START_TEST (q_multiple) {
    struct queue* q = construct_queue();
    // fi 1, 2
    enqueue(q, "test1");
    enqueue(q, "test2");

    // fo 1, 2
    ck_assert_str_eq("test1", (char*) delet(q));
    ck_assert_str_eq("test2", (char*)  delet(q));
    ck_assert_ptr_eq(NULL, delet(q));

    // assume empty, not full
    ck_assert_int_eq(1, is_empty(q));
    ck_assert_int_eq(0, is_full(q));

    // fi 3
    enqueue(q, "test3");

    // assume not full neither empty
    ck_assert_int_eq(0, is_empty(q));
    ck_assert_int_eq(0, is_full(q));

    // fo 3
    ck_assert_str_eq("test3", (char*) delet(q));

    // assume empty, not full
    ck_assert_int_eq(1, is_empty(q));
    ck_assert_int_eq(0, is_full(q));

    // one more check
    ck_assert_ptr_eq(NULL, delet(q));
    ck_assert_int_eq(1, is_empty(q));
    ck_assert_int_eq(0, is_full(q));

    destruct_queue(q);
}
END_TEST


START_TEST (q_limits) {
    struct queue* q = construct_queue();

    // create stringz of content 149 chars
    size_t stringSize = MAX_STRING_SIZE - 1;
    char* longString = malloc(sizeof(char) * stringSize);
    memset(longString, 'a', stringSize);
    longString[stringSize] = '\0';

    // add it one too few times, slightly below capacity
    for (size_t i=0; i < MAX_SIZE-1; ++i)
        enqueue(q, longString);

    ck_assert_int_eq(0, is_full(q));

    // add once more, exhausting capacity
    enqueue(q, longString);
    ck_assert_int_eq(1, is_full(q));

    // add once more, to be sure
    enqueue(q, longString);
    ck_assert_int_eq(1, is_full(q));

    // count we did not add more than is allowed
    size_t stringCount = 0;
    char* output;

    while (! is_empty(q))
    {
        output = (char*) delet(q);  // leaks!
        ++stringCount;
    }

    ck_assert_uint_eq(MAX_SIZE, stringCount);

    // check if max string length is truncated to correct amount
    unsigned long stringLength = strlen(output);
    ck_assert_uint_eq(stringLength, MAX_STRING_SIZE-1);

    destruct_queue(q);
}
END_TEST


START_TEST (ems_rule_checks_inefficient)
{
    struct system* sys = construct_sys();
    sys->user_setting = INEFFICIENT;
    float LOW = 24;
    float QUITE_LOW = 34;
    float HIGH = 75;

    // test cases to trigger it to start charging

    // trivial case
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(INEFFICIENT_CALCULATIONS, ems_rule_check(sys));
    ck_assert_int_eq(false, sys->charge_on);

    // charge battery 1
    sys->battery_soc[0] = LOW;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(CHARGE_BATTERY_1, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // charge battery 2
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = LOW;
    ck_assert_int_eq(CHARGE_BATTERY_2, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // charge both batteries
    sys->battery_soc[0] = LOW;
    sys->battery_soc[1] = LOW;
    ck_assert_int_eq(CHARGE_BATTERY_BOTH, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // note: charge_on flag still turned on
    sys->battery_soc[0] = QUITE_LOW;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(CHARGE_BATTERY_1, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = QUITE_LOW;
    ck_assert_int_eq(CHARGE_BATTERY_2, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // finally, get out of charge mode
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(INEFFICIENT_CALCULATIONS, ems_rule_check(sys));
    ck_assert_int_eq(false, sys->charge_on);

    destroy_sys(sys);
}
END_TEST


START_TEST (ems_rule_checks_soc)
{
    struct system* sys = construct_sys();
    sys->user_setting = SOC;
    float LOW = 34;
    float QUITE_LOW = 44;
    float HIGH = 75;

    // test cases to trigger it to start charging

    // trivial case
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(SOC_CALCULATIONS, ems_rule_check(sys));
    ck_assert_int_eq(false, sys->charge_on);

    // charge battery 1
    sys->battery_soc[0] = LOW;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(CHARGE_BATTERY_1, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // charge battery 2
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = LOW;
    ck_assert_int_eq(CHARGE_BATTERY_2, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // charge both batteries
    sys->battery_soc[0] = LOW;
    sys->battery_soc[1] = LOW;
    ck_assert_int_eq(CHARGE_BATTERY_BOTH, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // keep charging both batteries
    sys->battery_soc[0] = QUITE_LOW;
    sys->battery_soc[1] = QUITE_LOW;
    ck_assert_int_eq(CHARGE_BATTERY_BOTH, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);
    sys->battery_soc[0] = QUITE_LOW;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(CHARGE_BATTERY_1, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = QUITE_LOW;
    ck_assert_int_eq(CHARGE_BATTERY_2, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // finally, get out of charge mode
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(SOC_CALCULATIONS, ems_rule_check(sys));
    ck_assert_int_eq(false, sys->charge_on);

    destroy_sys(sys);
}
END_TEST


// identical to test routine for `INEFFICIENT_CALCULATIONS`
START_TEST (ems_rule_checks_fuel_efficiency)
{
    struct system* sys = construct_sys();
    sys->user_setting = FUEL_EFFICIENT;
    float LOW = 24;
    float QUITE_LOW = 34;
    float HIGH = 75;

    // test cases to trigger it to start charging

    // trivial case
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(FUEL_EFFICIENCY_CALCULATIONS, ems_rule_check(sys));
    ck_assert_int_eq(false, sys->charge_on);

    // charge battery 1
    sys->battery_soc[0] = LOW;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(CHARGE_BATTERY_1, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // charge battery 2
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = LOW;
    ck_assert_int_eq(CHARGE_BATTERY_2, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // charge both batteries
    sys->battery_soc[0] = LOW;
    sys->battery_soc[1] = LOW;
    ck_assert_int_eq(CHARGE_BATTERY_BOTH, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // note: charge_on flag still turned on
    sys->battery_soc[0] = QUITE_LOW;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(CHARGE_BATTERY_1, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = QUITE_LOW;
    ck_assert_int_eq(CHARGE_BATTERY_2, ems_rule_check(sys));
    ck_assert_int_eq(true, sys->charge_on);

    // finally, get out of charge mode
    sys->battery_soc[0] = HIGH;
    sys->battery_soc[1] = HIGH;
    ck_assert_int_eq(FUEL_EFFICIENCY_CALCULATIONS, ems_rule_check(sys));
    ck_assert_int_eq(false, sys->charge_on);

    destroy_sys(sys);
}
END_TEST


START_TEST(setpoint_1)
{
    // prepare scenario
    struct system* sys = construct_sys();
    test_fill(sys);
    sys->user_setting = SOC;

    // mode: bunkering
    execute_subroutine(sys);

    // assert correct setpoint
    ck_assert_int_eq(BUNK_SETPOINT, sys->goat_preference->total_power);

    // in bunkering mode, the battery should always charge until full
    // though, the SOC is above the forceful charging threshold
    ck_assert_int_gt(0, sys->goat_preference->battery_power[0]);
    ck_assert_int_gt(0, sys->goat_preference->battery_power[1]);
    ck_assert_int_eq(false, sys->charge_on);

    /* each of the DGs should be running at half the requested total power.
     * however, the batteries are also implicitly charged with 1kW each. thus,
     * we expect the following to hold true.
     */
    int chargingRate = sys->goat_preference->total_power * FIFTY_PERCENT + 1e3;
    for (size_t i=0; i < 2; ++i)
    {
        ck_assert_int_eq(chargingRate, sys->goat_preference->dg_power[i]);
        ck_assert_int_eq(-1e3, sys->goat_preference->battery_power[i]);
    }

    // now imagine the batteries are well charged
    sys->battery_soc[0] = 85;
    sys->battery_soc[1] = 85;
    execute_subroutine(sys);

    // we expect the fifty-fifty rule to hold
    int setpoint = BUNK_SETPOINT * FIFTY_PERCENT;

    for (size_t i=0; i < 2; ++i)
    {
        ck_assert_int_eq(setpoint, sys->goat_preference->dg_power[i]);
        ck_assert_int_eq(0, sys->goat_preference->battery_power[i]);
    }

    // change of operation
    sys->goat_preference->mode = SHORE_PUMP;
    execute_subroutine(sys);
    int setpointDG = SHPUMP_POWERSETPOINT * FOURTY_PERCENT;
    int setpointBattery = SHPUMP_POWERSETPOINT * TEN_PERCENT;

    // this is a heavy operation, so inefficiency is to be expected
    ck_assert_int_eq(true, sys->inefficiency_on);

    for (size_t i=0; i < 2; ++i)
    {
        ck_assert_int_eq(setpointDG, sys->goat_preference->dg_power[i]);
        ck_assert_int_eq(setpointBattery,
                         sys->goat_preference->battery_power[i]);
    }

    // deplete one of the batteries
    sys->battery_soc[0] = 22;
    execute_subroutine(sys);
    ck_assert_int_eq(true, sys->charge_on);
    ck_assert_int_eq(CHARGE_BATTERY_1, ems_rule_check(sys));
}
END_TEST



int main(void) {
    // set up suite
    Suite *suite = suite_create("All");
    SRunner *sr = srunner_create(suite);

    // add tests for queues to suite
    TCase *tcQueues = tcase_create("Queues");
    suite_add_tcase(suite, tcQueues);
    tcase_add_test(tcQueues, q_basic);
    tcase_add_test(tcQueues, q_multiple);
    tcase_add_test(tcQueues, q_limits);

    // add tests for rule checks to suite
    TCase *tcEMSRuleChecks = tcase_create("EMS rule checks");
    suite_add_tcase(suite, tcEMSRuleChecks);
    tcase_add_test(tcEMSRuleChecks, ems_rule_checks_inefficient);
    tcase_add_test(tcEMSRuleChecks, ems_rule_checks_soc);
    tcase_add_test(tcEMSRuleChecks, ems_rule_checks_fuel_efficiency);

    // add tests for setpoints to suite
    TCase *tcSetpoints = tcase_create("Setpoints");
    suite_add_tcase(suite, tcSetpoints);
    tcase_add_test(tcSetpoints, setpoint_1);

    // set up runner and run suite
    int nf;
    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}
