#include "state_machine.h"
#include "eps_hal.h"
#include <stdint.h>

eps_status_t eps_status;
module_status_t module_status[N_MODULES]; //stores the answers to be sent to an eventual i2c request

void eps_update_values()
{
	//TODO: Conversion
	eps_status.v_bat = (uint16_t)(ADC_read(AIN_V_BAT_ADDR) * 5000.0f / 4096.0f);
	eps_status.t_bat = (int16_t)((ADC_read(AIN_A_EXT0_ADDR) - 672.0f) * 5000.0f / 635.0f);
	eps_status.v_solar = (uint16_t)(ADC_read(AIN_V_SC_ADDR) * 8500.0f / 4096.0f);
	eps_status.current_in = (uint16_t)(ADC_read(AIN_I_IN_ADDR) * 5000.0f / 4096.0f);
	eps_status.current_out = (uint16_t)(ADC_read(AIN_I_OUT_ADDR) * 5000.0f / 4096.0f);
	eps_status.analog_ext1 = ADC_read(AIN_A_EXT1_ADDR);
//	eps_status.analog_ext2 = ADC_read(AIN_A_EXT2_ADDR);
//	eps_status.analog_ext3 = ADC_read(AIN_A_EXT3_ADDR);
//	eps_status.analog_ext4 = ADC_read(AIN_A_EXT4_ADDR);

	eps_status.v_bat_8 = (uint8_t)(eps_status.v_bat/100);
	eps_status.t_bat_8 = (int8_t)(eps_status.t_bat/100);
	eps_status.v_solar_8 = (uint8_t)(eps_status.v_solar/100);
	eps_status.current_in_8 = (uint8_t)(eps_status.current_in/100);
	eps_status.current_out_8 = (uint8_t)(eps_status.current_out/100);
	eps_status.analog_ext1_8 = (uint8_t)(eps_status.analog_ext1 >> 8);
//	eps_status.analog_ext2_8 = (uint8_t)(eps_status.analog_ext2 >> 8);
//	eps_status.analog_ext3_8 = (uint8_t)(eps_status.analog_ext3 >> 8);
//	eps_status.analog_ext4_8 = (uint8_t)(eps_status.analog_ext4 >> 8);
}

void eps_update_states()
{
	int i;
	for(i = 0; i < N_MODULES; i++)
	{
		if(module_status[i] == TURN_OFF)
		{
#ifdef FIRMWARE_BASE_STATION
			// wait for shutdown and turn off module
			if(i==M_5_RPI)
			{
				if(module_update_shutdown_signal(M_5_RPI, START_SHUTDOWN) == SHUTDOWN_COMPLETE)
				{
					module_set_state(M_5_RPI, 0);
					module_status[M_5_RPI] = MODULE_OFF;
				}
			}
#else
			// wait for shutdown and turn off module
			if(i==M_5_OLIMEX)
			{
				if(module_update_shutdown_signal(M_5_OLIMEX, START_SHUTDOWN) == SHUTDOWN_COMPLETE)
				{
					module_set_state(M_5_OLIMEX, 0);
					module_status[M_5_OLIMEX] = MODULE_OFF;
				}
			}
#endif
			else
			{
				//turn off module
				module_set_state(i, 0);
				module_status[i] = MODULE_OFF;
			}
		}
		else if(module_status[i] == TURN_ON)
		{
#ifdef FIRMWARE_BASE_STATION
			// wait for shutdown and turn off module
			if(i==M_5_RPI)
			{
				module_set_state(M_5_RPI, 1);
				if(module_update_shutdown_signal(M_5_RPI, START_BOOT) == SYSTEM_ON)
				{
					module_status[M_5_RPI] = MODULE_ON;
				}
			}
#else
			// wait for shutdown and turn off module
			if(i==M_5_OLIMEX)
			{
				module_set_state(M_5_OLIMEX, 1);
				if(module_update_shutdown_signal(M_5_OLIMEX, START_BOOT) == SYSTEM_ON)
				{
					module_status[M_5_OLIMEX] = MODULE_ON;
				}
			}
#endif
			else
			{
				//turn on module
				module_set_state(i, 1);
				module_status[i] = MODULE_ON;
			}
		}
	}

	if(eps_status.v_bat < BUZZER_THRESHOLD && eps_status.v_bat > ALL_OFF_THRESHOLD)
	{
		//Start buzzing
		if(module_status[BUZZER] == MODULE_OFF)
		{
			module_set_state(BUZZER, 1);
			module_status[BUZZER] = MODULE_ON;
		}
		else
		{
			module_set_state(BUZZER, 0);
			module_status[BUZZER] = MODULE_OFF;
		}
	}
	else
	{
		module_set_state(BUZZER, 0);
		module_status[BUZZER] = MODULE_OFF;
		module_set_state(M_M, 1);
		module_status[M_M] = MODULE_ON;
	}

	if(eps_status.v_bat < THRESHOLD_0) //low bat voltage threshold
	{
		//turn off all modules (inc. Mainboard)
		for(i = 0; i < N_MODULES; i++)
		{
			//turn off module
			module_set_state(i, 0);
			module_status[i] = MODULE_OFF;
		}
	}
	else if(eps_status.v_bat < ALL_OFF_THRESHOLD) //low bat voltage threshold
	{
		//turn off all modules except mb
		for(i = 0; i < N_MODULES; i++)
		{
			if(i != M_M && i!= BUZZER)
			{
				//turn off module
				module_set_state(i, 0);
				module_status[i] = MODULE_OFF;
			}
		}
	}
	else if(eps_status.v_bat < THRESHOLD_40) // rather low bat voltage threshold -> reduce internal temp
	{
		if(eps_status.t_bat < -1000)
		{
			//turn on heater
			module_set_state(H_T3, 1);
			module_status[H_T3] = MODULE_ON;
			module_set_state(H_T2, 1);
			module_status[H_T2] = MODULE_ON;
		}
		else if(eps_status.t_bat > -500)
		{
			//turn off heater
			module_set_state(H_T3, 0);
			module_status[H_T3] = MODULE_OFF;
			module_set_state(H_T2, 0);
			module_status[H_T2] = MODULE_OFF;
		}
	}
	else
	{
		if(eps_status.t_bat < -500)
		{
			//turn on heater
			module_set_state(H_T3, 1);
			module_status[H_T3] = MODULE_ON;
			module_set_state(H_T2, 1);
			module_status[H_T2] = MODULE_ON;
		}
		else if(eps_status.t_bat > 0)
		{
			//turn off heater
			module_set_state(H_T3, 0);
			module_status[H_T3] = MODULE_OFF;
			module_set_state(H_T2, 0);
			module_status[H_T2] = MODULE_OFF;
		}
	}
}


void eps_update_user_interface()
{
#ifdef FIRMWARE_BASE_STATION

	//check button:
	static uint8_t time_button_pressed = 0;
	if(PORT_DIGITAL_IN && PIN_DIGITAL_6)
		time_button_pressed = 0;
	else
		time_button_pressed += 1;

	if(time_button_pressed > 2) //at least 3 seconds
	{
		//always assume GPS and RPI have the same state! GPS waits for RPI to turn on/off completely untill it can change state again.
		if(module_status[M_5_RPI] == MODULE_ON)
		{
			module_status[M_5_RPI] = TURN_OFF;
			module_status[M_5_GPS] = TURN_OFF;
		}
		else if(module_status[M_5_RPI] == MODULE_OFF)
		{
			if(eps_status.v_bat > BOOT_THRESHOLD)
			{
				module_status[M_5_RPI] = TURN_ON;
				module_status[M_5_GPS] = TURN_ON;
			}
		}

		time_button_pressed = 0;
	}


	// LEDs:
	// battery full:
	if(eps_status.v_bat > THRESHOLD_95)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_1);
	else if(eps_status.v_bat < THRESHOLD_95-THRESHOLD_LED_HYS)
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_1);

	// battery good:
	if(eps_status.v_bat > THRESHOLD_40)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_2);
	else if(eps_status.v_bat < THRESHOLD_40-THRESHOLD_LED_HYS)
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_2);

	// EPS on / battery > 0%:
	if(eps_status.v_bat > THRESHOLD_0+THRESHOLD_LED_HYS)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_3);
	else if(eps_status.v_bat < THRESHOLD_0)
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_3);

	// RasPi on:
	if(module_status[M_5_RPI] == MODULE_ON)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_4);
	else
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_4);

	// EPS on / battery > 0%:
	if(module_status[M_5_GPS] == MODULE_ON)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_5);
	else
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_5);



#endif
}
