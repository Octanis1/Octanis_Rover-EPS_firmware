/*
 * communication.c
 *
 *  Created on: 11 Mar 2016
 *      Author: raffael, beat
 */

#include <msp430.h>
#include "communication.h"
#include "commands.h"
#include "eps_hal.h"
#include "state_machine.h"

int mainboard_poke_iterate(int *mainboard_poke_counter)
{
	int i;
	if(++(*mainboard_poke_counter) == POKE_COUNTER_LIMIT)
	{
		//poke mainboard
		CLR_PIN(PORT_MB_POKE, PIN_MB_POKE);
		return 1;
	}
	if(++(*mainboard_poke_counter) == 2 * POKE_COUNTER_LIMIT)
	{
		//mainboard did not react to poke, cut power and delay for 5 timer ticks
		//cut mainboard power
		module_set_state(M_M, OFF);

		for(i = 0; i < 5; i++)
		{
			__bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
			__no_operation();                       // Set breakpoint >>here<< and read
		}
		//reset poke pin
		SET_PIN(PORT_MB_POKE, PIN_MB_POKE);
		//power up mainboard
		module_set_state(M_M, ON);
		//reset counter
		*mainboard_poke_counter = 0;
		return -1;
	}
	return 0;
}

int i2c_respond_command() // sets the response.
{
	if(i2c_available())
	{
		switch(i2c_read())
		{
			//poke response
			case ALIVE:
				i2c_send(COMM_OK);
				break;
			//turn off module
			case M3V3_1_OFF:
				i2c_send(COMM_OK);
				module_status[M_331] = TURN_OFF;
				break;
			case M3V3_2_OFF:
				i2c_send(COMM_OK);
				module_status[M_332] = TURN_OFF;
				break;
			case M5V_OFF:
				i2c_send(COMM_OK);
				module_status[M_5] = TURN_OFF;
				break;
			case M11V_OFF:
				i2c_send(COMM_OK);
				module_status[M_11] = TURN_OFF;
				break;
			// turn on module
			case M3V3_1_ON:
				if (eps_status.v_bat > THRESHOLD_10)
				{
					i2c_send(COMM_OK);
					module_status[M_331] = TURN_ON;
				}
				else
				{
					i2c_send(LOW_VOLTAGE);
				}
				break;
			case M3V3_2_ON:
				if (eps_status.v_bat > THRESHOLD_10)
				{
					i2c_send(COMM_OK);
					module_status[M_332] = TURN_ON;
				}
				else
				{
					i2c_send(LOW_VOLTAGE);
				}
				break;
			case M5V_ON:
				if (eps_status.v_bat > THRESHOLD_10)
				{
					i2c_send(COMM_OK);
					module_status[M_5] = TURN_ON;
				}
				else
				{
					i2c_send(LOW_VOLTAGE);
				}
				break;
			case M11V_ON:
				if (eps_status.v_bat > THRESHOLD_10)
				{
					i2c_send(COMM_OK);
					module_status[M_11] = TURN_ON;
				}
				else
				{
					i2c_send(LOW_VOLTAGE);
				}
				break;
			//return analog values
			case V_BAT:
				i2c_send(eps_status.v_bat_8);
				break;
			case V_SC:
				i2c_send(eps_status.v_solar_8);
				break;
			case I_IN:
				i2c_send(eps_status.current_in_8);
				break;
			case I_OUT:
				i2c_send(eps_status.current_out_8);
				break;
			case AEXT1:
				i2c_send(eps_status.analog_ext1_8);
				break;
//			case AEXT2:
//				i2c_send(eps_status.analog_ext2_8);
//				break;
//			case AEXT3:
//				i2c_send(eps_status.analog_ext3_8);
//				break;
//			case AEXT4:
//				i2c_send(eps_status.analog_ext4_8);
//				break;
			case T_BAT:
				i2c_send(eps_status.t_bat_8);
				break;
			//default response to unknown commands
			default: i2c_send(UNKNOWN_COMMAND);break;
		}
		return 1;
	}
	else
	{
		return 0;
	}
}

void execute_i2c_command(unsigned char command)
{
		switch(command)
		{
			default: break;
		}

}
