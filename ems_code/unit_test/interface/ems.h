#ifndef _EEP71_EMS_
#define _EEP71_EMS_

// #include "spiQueue.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// forward declaration "spiQueue.h"
// struct structSpiQueue;
// struct structFrame;

#define BATTERY_AMOUNT (2)
#define DG_AMOUNT (2)
#define MODE_AMOUNT (9)

/* Power setpoints */
#define BAG_POWERSETPOINT (2950)
#define IDLE_POWERSETPOINT (2500)
#define DUMP_POWERSETPOINT (2150)
#define RAIN_POWERSETPOINT (3300)
#define SHPUMP_POWERSETPOINT (3950)
#define BUNK_SETPOINT (600)
#define TRAIL_SETPOINT (3150)
#define DEPGEAR_SETPOINT (0)
#define COLLGEAR_SETPOINT (0)

/* Important rule setpoints */
#define MINIMUM_SOC (25)
#define CHARGING_SOC (35)
#define CHARGE_COMPLETE_SOC_35 (35)
#define CHARGE_COMPLETE_SOC_45 (45)
#define MAXIMUM_SOC (70)
#define MAX_POWER_DG (3800)
#define MAX_POWER_BATTERY (2000)
#define MAX_POWER_DG_1 (1900)
#define MAX_BATTERY_BATTERY_1 (1000)

/* arithmetic values */
#define FIFTY_PERCENT (0.5f)
#define TWENTYFIVE_PERCENT (0.25f)
#define SEVENTYFIVE_PERCENT (0.75f)
#define SIXTY_PERCENT (0.6f)
#define FOURTY_PERCENT (0.4f)
#define EIGHTY_PERCENT (0.8f)
#define TWENTY_PERCENT (0.2f)
#define TEN_PERCENT (0.1f)

/* outbound identifiers */
#define SETPOINT_BATTERY1_ID (0xB1)
#define SETPOINT_BATTERY2_ID (0xB2)
#define SETPOINT_DG1_ID (0xB3)
#define SETPOINT_DG2_ID (0xB4)
#define SETPOINT_OVERLOAD_ID (0xB5)
#define TEST_LATENCY_ID (0xA9)

/* inbound identifiers */
#define POWER_BATTERY1_ID (0xC1)
#define POWER_BATTERY2_ID (0xC2)
#define SOC_BATTERY1_ID (0xC3)
#define SOC_BATTERY2_ID (0xC4)
#define POWER_DG1_ID (0xC5)
#define POWER_DG2_ID (0xC6)
#define SFOC_DG1_ID (0xC7)
#define SFOC_DG2_ID (0xC8)
#define CURRENT_MODE_ID (0xC9)
#define TEST_LATENCY_RETURN_ID (0xA9)

/*macro for array size*/
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef enum {
	SAIL_EMPTY = 1,
	DEPLOY_GEAR,
	TRAIL,
	COLLECT_GEAR,
	SAIL_FULL,
	RAINBOW,
	SHORE_PUMP,
	DUMPING,
	BUNKERING,
	INIT = 99,
} SHIP_STAGE_ENUM;

typedef enum {
	INEFFICIENT = 1,
	SOC = 2,
	FUEL_EFFICIENT = 3
} OPTIMIZATION_STRATEGY_ENUM;

typedef enum {
	INEFFICIENT_CALCULATIONS = 0,
	SOC_CALCULATIONS,
	FUEL_EFFICIENCY_CALCULATIONS,
	CHARGE_BATTERY_1,
	CHARGE_BATTERY_2,
	CHARGE_BATTERY_BOTH
} RULE_CHECK_ENUM;

struct data_to_goat {
	SHIP_STAGE_ENUM mode;
	uint32_t dg_power[DG_AMOUNT];
	int32_t battery_power[BATTERY_AMOUNT];
	uint32_t total_power;
};

struct system {
	struct data_to_goat* goat_preference;
	OPTIMIZATION_STRATEGY_ENUM user_setting;
	uint32_t power_dg[DG_AMOUNT];
	double power_battery[BATTERY_AMOUNT];
	float battery_soc[BATTERY_AMOUNT];
	float fuel_efficiency[DG_AMOUNT];
	uint16_t sys_count;
	bool inefficiency_on;
	bool charge_on;
};

struct ship_state_subroutines {
	SHIP_STAGE_ENUM mode;
	char* name;
	void (*subroutine)(struct system* sys);
	uint32_t reference_power;
};

struct system* construct_sys(void);
void destroy_sys(struct system* sys);
void execute_subroutine(struct system* sys);
void test_fill(struct system* sys);
void send_setpoints(struct system* sys);
void parse_simulation_data(struct system* sys);
void rate_limit(void);
int ems_rule_check(struct system *sys);

// extern struct system* sys;
extern struct ship_state_subroutines subroutines[];

/*
 * Function: template, parameters: template
 * ----------------------------
 *   we templating
 *
 *   n1: template template template template
 *   n2: template template template
 *
 *   returns: template template template template template
 */

#endif
