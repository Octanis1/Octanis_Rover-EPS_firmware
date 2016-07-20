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

volatile int mainboard_poke_counter = 0;

int mainboard_poke_iterate()
{
	int i;
	if(++mainboard_poke_counter == POKE_COUNTER_LIMIT)
	{
		//poke mainboard
		SET_PIN(PORT_MB_POKE, PIN_MB_POKE);
		return 1;
	}
	if(mainboard_poke_counter == 2 * POKE_COUNTER_LIMIT)
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
		CLR_PIN(PORT_MB_POKE, PIN_MB_POKE);
		//power up mainboard
		module_set_state(M_M, ON);
		//reset counter
		mainboard_poke_counter = 0;
		return -1;
	}
	return 0;
}

int mainboard_poke_get_counter()
{
	return mainboard_poke_counter;
}

void mainboard_poke_reset_counter()
{
	//reset counter
	mainboard_poke_counter = 0;
	//reset poke pin if already activated
	CLR_PIN(PORT_MB_POKE, PIN_MB_POKE);
}

void i2c_receive_callback()
{
	switch(i2c_read())
	{
		//poke response
		case ALIVE:
			i2c_send_byte(COMM_OK, 0);
			break;
		//turn off module
		case M3V3_1_OFF:
			i2c_send_byte(COMM_OK, 0);
			module_status[M_331] = TURN_OFF;
			break;
		case M3V3_2_OFF:
			i2c_send_byte(COMM_OK, 0);
			module_status[M_332] = TURN_OFF;
			break;
		case M5V_OFF:
			i2c_send_byte(COMM_OK, 0);
			module_status[M_5] = TURN_OFF;
			break;
		case M11V_OFF:
			i2c_send_byte(COMM_OK, 0);
			module_status[M_11] = TURN_OFF;
			break;
		// turn on module
		case M3V3_1_ON:
			if (eps_status.v_bat > THRESHOLD_10)
			{
				i2c_send_byte(COMM_OK, 0);
				module_status[M_331] = TURN_ON;
			}
			else
			{
				i2c_send_byte(LOW_VOLTAGE, 0);
			}
			break;
		case M3V3_2_ON:
			if (eps_status.v_bat > THRESHOLD_10)
			{
				i2c_send_byte(COMM_OK, 0);
				module_status[M_332] = TURN_ON;
			}
			else
			{
				i2c_send_byte(LOW_VOLTAGE, 0);
			}
			break;
		case M5V_ON:
			if (eps_status.v_bat > THRESHOLD_10)
			{
				i2c_send_byte(COMM_OK, 0);
				module_status[M_5] = TURN_ON;
			}
			else
			{
				i2c_send_byte(LOW_VOLTAGE, 0);
			}
			break;
		case M11V_ON:
			if (eps_status.v_bat > THRESHOLD_10)
			{
				i2c_send_byte(COMM_OK, 0);
				module_status[M_11] = TURN_ON;
			}
			else
			{
				i2c_send_byte(LOW_VOLTAGE, 0);
			}
			break;
		//return analog values
		case V_BAT:
			i2c_send_word(eps_status.v_bat, 0);
			break;
		case V_SC:
			i2c_send_word(eps_status.v_solar, 0);
			break;
		case I_IN:
			i2c_send_word(eps_status.current_in, 0);
			break;
		case I_OUT:
			i2c_send_word(eps_status.current_out, 0);
			break;
		case AEXT1:
			i2c_send_word(eps_status.analog_ext1, 0);
			break;
//			case AEXT2:
//				i2c_send_word(eps_status.analog_ext2, 0);
//				break;
//			case AEXT3:
//				i2c_send_word(eps_status.analog_ext3, 0);
//				break;
//			case AEXT4:
//				i2c_send_word(eps_status.analog_ext4, 0);
//				break;
		case T_BAT:
			i2c_send_word(eps_status.t_bat, 0);
			break;
		//default response to unknown commands
		default: i2c_send_byte(UNKNOWN_COMMAND, 0);break;
	}
	mainboard_poke_reset_counter();
}
