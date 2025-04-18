#include "ems.h"
#include <stdio.h>
#include <unistd.h>

void sail_subroutine(struct system* sys);
void deploy_subroutine(struct system* sys);
void trail_subroutine(struct system* sys);
void collect_subroutine(struct system* sys);
void rainbow_subroutine(struct system* sys);
void shpump_subroutine(struct system* sys);
void dump_subroutine(struct system* sys);
void bunk_subroutine(struct system* sys);

// clang-format off
struct ship_state_subroutines subroutines[] = {
		{.mode = SAIL_EMPTY,	.subroutine = sail_subroutine,		.reference_power = IDLE_POWERSETPOINT,		.name = "sailing empty"},
		{.mode = DEPLOY_GEAR,	.subroutine = deploy_subroutine,	.reference_power = DEPGEAR_SETPOINT,		.name = "deploying gear"},
		{.mode = TRAIL,			.subroutine = trail_subroutine,		.reference_power = TRAIL_SETPOINT,			.name = "trailing"},
		{.mode = COLLECT_GEAR,	.subroutine = collect_subroutine,	.reference_power = COLLGEAR_SETPOINT,		.name = "collecting gear"},
		{.mode = SAIL_FULL,		.subroutine = sail_subroutine,		.reference_power = IDLE_POWERSETPOINT,		.name = "sailing full"},
		{.mode = RAINBOW,		.subroutine = rainbow_subroutine,	.reference_power = RAIN_POWERSETPOINT,		.name = "rainbowing"},
		{.mode = SHORE_PUMP,	.subroutine = shpump_subroutine,	.reference_power = SHPUMP_POWERSETPOINT,	.name = "shore pumping"},
		{.mode = DUMPING,		.subroutine = dump_subroutine,		.reference_power = DUMP_POWERSETPOINT,		.name = "dumping"},
		{.mode = BUNKERING,		.subroutine = bunk_subroutine,		.reference_power = BUNK_SETPOINT,			.name = "bunkering"}
};
// clang-format on

void execute_subroutine(struct system* sys) {
	if (sys->goat_preference->mode > BUNKERING || sys->goat_preference->mode <= 0 ) {
		return;
	}
	size_t index = sys->goat_preference->mode - 1;
	subroutines[index].subroutine(sys);
}

struct system* construct_sys(void) {
	struct system* sys = malloc(sizeof(struct system));
	sys->goat_preference = malloc(sizeof(struct data_to_goat));

	sys->user_setting = INEFFICIENT;
	sys->power_dg[0] = 0;
	sys->power_dg[1] = 0;
	sys->power_battery[0] = 0.0;
	sys->power_battery[1] = 0.0;
	sys->goat_preference->total_power = 0;
	sys->battery_soc[0] = 0.0;
	sys->battery_soc[1] = 0.0;
	sys->fuel_efficiency[0] = 0.0;
	sys->fuel_efficiency[1] = 0.0;
	sys->sys_count = 0;
	sys->inefficiency_on = true;
	sys->charge_on = false;
	sys->goat_preference->mode = INIT;
	sys->goat_preference->dg_power[0] = 0.0;
	sys->goat_preference->dg_power[1] = 0.0;
	sys->goat_preference->battery_power[0] = 0.0;
	sys->goat_preference->battery_power[1] = 0.0;
	return sys;
}

void check_user_setting(struct system* sys) {
	if (sys->user_setting != INEFFICIENT) {
		sys->inefficiency_on = false;
	} else {
		sys->inefficiency_on = true;
	}
}

void destroy_sys(struct system* sys) {
	free(sys->goat_preference);
	free(sys);
}

/*
 * Function: ems_rule_check, parameters: sys
 * ----------------------------
 *   Checks with
 *
 *   sys: system struct with all needed info
 *
 *   returns: 0 -> for inefficient calcs, 1 -> for soc calcs, 2 -> for fuel efficiency calcs, 3 -> charge batt1, 4 -> charge batt2, 5 -> charge both
 */

int ems_rule_check(struct system* sys) {
	if(sys->charge_on == true){
		if(sys->user_setting == SOC){
			if (sys->battery_soc[0] < CHARGE_COMPLETE_SOC_45 && sys->battery_soc[1] < CHARGE_COMPLETE_SOC_45) {
				return CHARGE_BATTERY_BOTH;
			}

			if (sys->battery_soc[0] < CHARGE_COMPLETE_SOC_45) {
				return CHARGE_BATTERY_1;
			} else if (sys->battery_soc[1] < CHARGE_COMPLETE_SOC_45) {
				return CHARGE_BATTERY_2;
			} else {
				sys->charge_on = false;
			}
		} else{
			if (sys->battery_soc[0] < CHARGE_COMPLETE_SOC_35 && sys->battery_soc[1] < CHARGE_COMPLETE_SOC_35) {
				return CHARGE_BATTERY_BOTH;
			}

			if (sys->battery_soc[0] < CHARGE_COMPLETE_SOC_35) {
				return CHARGE_BATTERY_1;
			} else if (sys->battery_soc[1] < CHARGE_COMPLETE_SOC_35) {
				return CHARGE_BATTERY_2;
			} else {
				sys->charge_on = false;
			}
		}
	}


	if (sys->user_setting == INEFFICIENT) {
		if (sys->battery_soc[0] < MINIMUM_SOC && sys->battery_soc[1] < MINIMUM_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_BOTH;
		}

		if (sys->battery_soc[0] < MINIMUM_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_1;
		} else if (sys->battery_soc[1] < MINIMUM_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_2;
		}

		return INEFFICIENT_CALCULATIONS;
	}

	if (sys->user_setting == SOC) {
		if (sys->battery_soc[0] < CHARGING_SOC && sys->battery_soc[1] < CHARGING_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_BOTH;
		}

		if (sys->battery_soc[0] < CHARGING_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_1;
		} else if (sys->battery_soc[1] < CHARGING_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_2;
		}

		return SOC_CALCULATIONS;
	}

	if (sys->user_setting == FUEL_EFFICIENT) {
		if (sys->battery_soc[0] < MINIMUM_SOC && sys->battery_soc[1] < MINIMUM_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_BOTH;
		}
		if (sys->battery_soc[0] < MINIMUM_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_1;
		} else if (sys->battery_soc[1] < MINIMUM_SOC) {
			sys->charge_on = true;
			return CHARGE_BATTERY_2;
		}
		return FUEL_EFFICIENCY_CALCULATIONS;
	}

	return INEFFICIENT_CALCULATIONS;
}

void rate_limit(void) {
	sleep(1);
}

void parse_simulation_data(struct system* sys, struct structFrame* dataframe) {

	/*stm spi functions*/
	switch (dataframe->identifier) {
	case POWER_BATTERY1_ID: {
		sys->power_battery[0] = dataframe->payload.frac64;
		break;
	}
	case POWER_BATTERY2_ID: {
		sys->power_battery[1] = dataframe->payload.frac64;
		break;
	}
	case SOC_BATTERY1_ID: {
		sys->battery_soc[0] = dataframe->payload.frac32;
		break;
	}
	case SOC_BATTERY2_ID: {
		sys->battery_soc[1] = dataframe->payload.frac32;
		break;
	}
	case POWER_DG1_ID: {
		sys->power_dg[0] = dataframe->payload.uint32;
		break;
	}
	case POWER_DG2_ID: {
		sys->power_dg[1] = dataframe->payload.uint32;
		break;
	}
	case SFOC_DG1_ID: {
		sys->fuel_efficiency[0] = dataframe->payload.frac32;
		break;
	}
	case SFOC_DG2_ID: {
		sys->fuel_efficiency[1] = dataframe->payload.frac32;
		break;
	}
	case CURRENT_MODE_ID: {
		sys->goat_preference->mode = dataframe->payload.uint8[0];
		break;
	}
	}
}

void ready_setpoint(struct system* sys, int ems_state) {
	if (ems_state == CHARGE_BATTERY_1 || ems_state == CHARGE_BATTERY_BOTH || ems_state == CHARGE_BATTERY_2) {
		if (sys->goat_preference->total_power > 3000) {
			sys->inefficiency_on = true;
		} else {
			check_user_setting(sys);
		}
	}

	switch (ems_state) {
	case INEFFICIENT_CALCULATIONS: {
		sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * TWENTYFIVE_PERCENT);
		sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * TWENTYFIVE_PERCENT);
		sys->goat_preference->battery_power[0] = (sys->goat_preference->total_power * TWENTYFIVE_PERCENT);
		sys->goat_preference->battery_power[1] = (sys->goat_preference->total_power * TWENTYFIVE_PERCENT);
		break;
	}
	case SOC_CALCULATIONS: {
		sys->goat_preference->battery_power[0] = 0;
		sys->goat_preference->battery_power[1] = 0;
		if (sys->goat_preference->total_power == BUNK_SETPOINT) {
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FIFTY_PERCENT);
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FIFTY_PERCENT);
			if (sys->battery_soc[0] < MAXIMUM_SOC) {
				sys->goat_preference->battery_power[0] = -1000;
				sys->goat_preference->dg_power[0] += 1000;
			}
			if (sys->battery_soc[1] < MAXIMUM_SOC) {
				sys->goat_preference->battery_power[1] = -1000;
				sys->goat_preference->dg_power[1] += 1000;
			}
		}

		if (sys->goat_preference->total_power == DUMP_POWERSETPOINT) {
			sys->goat_preference->dg_power[0] = sys->goat_preference->total_power * FIFTY_PERCENT;
			sys->goat_preference->dg_power[1] = sys->goat_preference->total_power * FIFTY_PERCENT;

			if (sys->battery_soc[0] < MAXIMUM_SOC) {
				sys->goat_preference->dg_power[0] += 500;
				sys->goat_preference->battery_power[0] = -500;
			}

			if (sys->battery_soc[1] < MAXIMUM_SOC) {
				sys->goat_preference->dg_power[0] += 500;
				sys->goat_preference->battery_power[1] = -500;
			}
		}

		if (sys->goat_preference->total_power >= IDLE_POWERSETPOINT) {
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FOURTY_PERCENT);
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FOURTY_PERCENT);
			sys->goat_preference->battery_power[0] = (sys->goat_preference->total_power * TEN_PERCENT);
			sys->goat_preference->battery_power[1] = (sys->goat_preference->total_power * TEN_PERCENT);
		}

		break;
	}
	case FUEL_EFFICIENCY_CALCULATIONS: {
		if (sys->goat_preference->total_power == BUNK_SETPOINT || sys->goat_preference->total_power == DUMP_POWERSETPOINT) {
			sys->goat_preference->dg_power[0] = 0;
			sys->goat_preference->dg_power[1] = 0;
			sys->goat_preference->battery_power[0] = sys->goat_preference->total_power * FIFTY_PERCENT;
			sys->goat_preference->battery_power[1] = sys->goat_preference->total_power * FIFTY_PERCENT;
		}

		if (sys->goat_preference->total_power == SHPUMP_POWERSETPOINT) {
			sys->goat_preference->dg_power[0] = sys->goat_preference->total_power * FIFTY_PERCENT;
			sys->goat_preference->dg_power[1] = 0;
			sys->goat_preference->battery_power[0] = sys->goat_preference->total_power * TWENTYFIVE_PERCENT;
			sys->goat_preference->battery_power[1] = sys->goat_preference->total_power * TWENTYFIVE_PERCENT;
		}

		if (sys->goat_preference->total_power >= IDLE_POWERSETPOINT && sys->goat_preference->total_power < SHPUMP_POWERSETPOINT) {
			sys->goat_preference->dg_power[0] = MAX_POWER_DG_1;
			sys->goat_preference->dg_power[1] = 0;
			sys->goat_preference->battery_power[0] = (sys->goat_preference->total_power - 1900) / 2;
			sys->goat_preference->battery_power[1] = (sys->goat_preference->total_power - 1900) / 2;
		}

		break;
	}
	case CHARGE_BATTERY_1: {
		sys->goat_preference->battery_power[0] = 0;
		sys->goat_preference->battery_power[1] = 0;
		if (sys->inefficiency_on) {
			if(sys->goat_preference->total_power > 3000){
				sys->goat_preference->dg_power[0] = sys->goat_preference->total_power * FIFTY_PERCENT;
				sys->goat_preference->dg_power[1] = sys->goat_preference->total_power * FIFTY_PERCENT;
				break;
			}
		}
		unsigned int remaining_available_power = MAX_POWER_DG - sys->goat_preference->total_power;
		if (remaining_available_power > 1000) {
			sys->goat_preference->battery_power[0] = -1000;
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FIFTY_PERCENT) + 500;
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FIFTY_PERCENT) + 500;
		} else {
			sys->goat_preference->battery_power[0] -= remaining_available_power;
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FIFTY_PERCENT) + (remaining_available_power / 2);
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FIFTY_PERCENT) + (remaining_available_power / 2);
		}
		break;
	}
	case CHARGE_BATTERY_2: {
		sys->goat_preference->battery_power[0] = 0;
		sys->goat_preference->battery_power[1] = 0;
		if (sys->inefficiency_on) {
			if(sys->goat_preference->total_power > 3000){
				sys->goat_preference->dg_power[0] = sys->goat_preference->total_power * FIFTY_PERCENT;
				sys->goat_preference->dg_power[1] = sys->goat_preference->total_power * FIFTY_PERCENT;
				break;
			}
		}
		float remaining_available_power = MAX_POWER_DG - sys->goat_preference->total_power;
		if (remaining_available_power > 1000) {
			sys->goat_preference->battery_power[1] = -1000;
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FIFTY_PERCENT) + 500;
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FIFTY_PERCENT) + 500;
		} else {
			sys->goat_preference->battery_power[1] -= remaining_available_power;
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FIFTY_PERCENT) + (remaining_available_power / 2);
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FIFTY_PERCENT) + (remaining_available_power / 2);
		}
		break;
	}
	case CHARGE_BATTERY_BOTH: {
		sys->goat_preference->battery_power[0] = 0;
		sys->goat_preference->battery_power[1] = 0;
		float remaining_available_power = MAX_POWER_DG - sys->goat_preference->total_power;
		if (sys->inefficiency_on) {
			if(sys->goat_preference->total_power > 3000){
				sys->goat_preference->dg_power[0] = sys->goat_preference->total_power * FIFTY_PERCENT;
				sys->goat_preference->dg_power[1] = sys->goat_preference->total_power * FIFTY_PERCENT;
				break;
			}
		}


		if (remaining_available_power > 2000) {
			sys->goat_preference->battery_power[0] = -1000;
			sys->goat_preference->battery_power[1] = -1000;
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FIFTY_PERCENT) + 1000;
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FIFTY_PERCENT) + 1000;
		} else {
			sys->goat_preference->battery_power[0] -= (remaining_available_power / 2);
			sys->goat_preference->battery_power[1] -= (remaining_available_power / 2);
			sys->goat_preference->dg_power[0] = (sys->goat_preference->total_power * FIFTY_PERCENT) + (remaining_available_power / 2);
			sys->goat_preference->dg_power[1] = (sys->goat_preference->total_power * FIFTY_PERCENT) + (remaining_available_power / 2);
		}
		break;
	}
	}
}

void send_setpoints(struct system* sys, struct structSpiQueue* tx_buffer) {
	spiQueuePostDec(tx_buffer, SETPOINT_BATTERY1_ID, sys->goat_preference->battery_power[0]);

	double test = 0.0;

	if (test > 0  && test < 0.1){
		HAL_Delay(1);
	}


	if (tx_buffer->tailFramePtr->payload.frac64 > 0  && tx_buffer->tailFramePtr->payload.frac64 < 0.1) {
		spiQueueFrameRemove(tx_buffer);
	}

	spiQueuePostDec(tx_buffer, SETPOINT_BATTERY2_ID, sys->goat_preference->battery_power[1]);
	if (tx_buffer->tailFramePtr->payload.frac64 > 0  && tx_buffer->tailFramePtr->payload.frac64 < 0.1) {
		spiQueueFrameRemove(tx_buffer);
	}

	spiQueuePostDec(tx_buffer, SETPOINT_DG1_ID, sys->goat_preference->dg_power[0]);
	if (tx_buffer->tailFramePtr->payload.frac64 > 0  && tx_buffer->tailFramePtr->payload.frac64 < 0.1) {
		spiQueueFrameRemove(tx_buffer);
	}

	spiQueuePostDec(tx_buffer, SETPOINT_DG2_ID, sys->goat_preference->dg_power[1]);
	if (tx_buffer->tailFramePtr->payload.frac64 > 0  && tx_buffer->tailFramePtr->payload.frac64 < 0.1) {
		spiQueueFrameRemove(tx_buffer);
	}
}

void sail_subroutine(struct system* sys) {
	sys->goat_preference->total_power = IDLE_POWERSETPOINT;
	int res = ems_rule_check(sys);
	ready_setpoint(sys, res);
}

void deploy_subroutine(struct system* sys) {
}

void trail_subroutine(struct system* sys) {
	sys->goat_preference->total_power = TRAIL_SETPOINT;
	int res = ems_rule_check(sys);
	ready_setpoint(sys, res);
}

void collect_subroutine(struct system* sys) {
}

void rainbow_subroutine(struct system* sys) {
	sys->goat_preference->total_power = RAIN_POWERSETPOINT;
	int res = ems_rule_check(sys);
	ready_setpoint(sys, res);
}

void shpump_subroutine(struct system* sys) {
	sys->goat_preference->total_power = SHPUMP_POWERSETPOINT;
	int res = ems_rule_check(sys);
	ready_setpoint(sys, res);
}

void dump_subroutine(struct system* sys) {
	sys->goat_preference->total_power = DUMP_POWERSETPOINT;
	int res = ems_rule_check(sys);
	ready_setpoint(sys, res);
}

void bunk_subroutine(struct system* sys) {
	sys->goat_preference->total_power = BUNK_SETPOINT;
	int res = ems_rule_check(sys);
	ready_setpoint(sys, res);
}

void test_fill(struct system* sys) {
	sys->user_setting = SOC;
	sys->power_dg[0] = 0;
	sys->power_dg[1] = 0;
	sys->power_battery[0] = 0;
	sys->power_battery[1] = 0;
	sys->battery_soc[0] = 60.0;
	sys->battery_soc[1] = 60.0;
	sys->fuel_efficiency[0] = 0.0;
	sys->fuel_efficiency[1] = 0.0;
	sys->sys_count = 0;
	sys->goat_preference->mode = BUNKERING;
}
