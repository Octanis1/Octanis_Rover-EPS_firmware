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
static char all_systems_rebooting = 0;

char are_all_systems_rebooting()
{
	return all_systems_rebooting;
}

int mainboard_poke_iterate()
{
#ifndef FIRMWARE_BASE_STATION
	static int mainboard_reset_counter = 0;

	if(module_status[M_M] == MODULE_ON)
	{
		mainboard_poke_counter++;
		if(mainboard_poke_counter > POKE_COUNTER_LIMIT)
		{
			//mainboard did not react to poke --> reset it
			mainboard_reset();
			//reset counter
			mainboard_reset_counter++;
			mainboard_poke_counter = 0;
			if(mainboard_reset_counter > RESET_COUNTER_LIMIT)
			{
				// reset didnt help --> shut down all systems and restart.
				// WARNING BEEP
				int i;
				for(i=20;i>0;i--)
				{
					module_set_state(BUZZER, 1);
					timer_delay100(1);
					module_set_state(BUZZER, 0);
					timer_delay100(1);
				}
				turn_off_all_modules(0);
				timer_delay100(20);
				mainboard_reset_counter = 0;
				all_systems_rebooting = 1;
			}
			return -1;
		}
	}
	else // dont reset mainboard while it's off, so we can flash it.
	{
		mainboard_poke_counter = 0;
	}

	if(all_systems_rebooting) //check if we can turn them on again (everything must be completely off
								//in order to avoid pin input leakage current through GPIOS
	{
		if(module_status[M_5_OLIMEX] == MODULE_OFF)
		{
			turn_on_all_rover_modules();
			mainboard_poke_counter = 0;
			mainboard_reset_counter = 0; //just to be sure
			all_systems_rebooting = 0;
		}
		else if(module_status[M_5_OLIMEX] == MODULE_ON) //if it was not fully on before, we have to switch it off now!
		{
			module_status[M_5_OLIMEX] = TURN_OFF;
		}
	}

#endif

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
//	CLR_PIN(PORT_MB_POKE, PIN_MB_POKE) --> not needed anymore
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
		case M5V2_OFF:
			i2c_send_byte(COMM_OK, 0);
			module_status[M_52] = TURN_OFF;
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
		case M5V2_ON:
			if (eps_status.v_bat > THRESHOLD_10)
			{
				i2c_send_byte(COMM_OK, 0);
				module_status[M_52] = TURN_ON;
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
