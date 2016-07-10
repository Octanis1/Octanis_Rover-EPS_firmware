/*
 * state_machine.h
 *
 *  Created on: 09.05.2016
 *      Author: beat
 */

#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include "eps_hal.h"
#include <stdint.h>

typedef struct _eps_status { //stores the answers to be sent to an eventual i2c request
	uint16_t v_bat; //mV
	int16_t t_bat; //0.01°C
	uint16_t v_solar; //mV
	uint16_t current_in; //mA
	uint16_t current_out; //mA
	uint16_t analog_ext1; //units
	uint16_t analog_ext2; //units
	uint16_t analog_ext3; //units
	uint16_t analog_ext4; //units

	uint8_t v_bat_8; //.1V
	int8_t t_bat_8; //°C
	uint8_t v_solar_8; //.1V
	uint8_t current_in_8; //0.1A
	uint8_t current_out_8; //0.1A
	uint8_t analog_ext1_8; //units/256
	uint8_t analog_ext2_8; //units/256
	uint8_t analog_ext3_8; //units/256
	uint8_t analog_ext4_8; //units/256
} eps_status_t;

typedef enum _module_status{
	MODULE_OFF=0,
	TURN_OFF,
	TURN_ON,
	MODULE_ON,
	FAULT
} module_status_t;

extern eps_status_t eps_status;
extern module_status_t module_status[N_MODULES]; //stores the answers to be sent to an eventual i2c request

void eps_update_values();
void eps_update_states();

#endif /* STATE_MACHINE_H_ */
