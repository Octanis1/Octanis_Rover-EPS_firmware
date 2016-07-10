#include "state_machine.h"
#include "eps_hal.h"

eps_status_t eps_status;
module_status_t module_status[N_MODULES]; //stores the answers to be sent to an eventual i2c request

void eps_update_values()
{
	//TODO: Conversion
	eps_status.v_bat = ADC_read(AIN_V_BAT_ADDR);
	eps_status.t_bat = ADC_read(AIN_A_EXT0_ADDR);
	eps_status.v_solar = ADC_read(AIN_V_SC_ADDR);
	eps_status.current_in = ADC_read(AIN_I_IN_ADDR);
	eps_status.current_out = ADC_read(AIN_I_OUT_ADDR);
	eps_status.analog_ext1 = ADC_read(AIN_A_EXT1_ADDR);
	eps_status.analog_ext2 = ADC_read(AIN_A_EXT2_ADDR);
	eps_status.analog_ext3 = ADC_read(AIN_A_EXT3_ADDR);
	eps_status.analog_ext4 = ADC_read(AIN_A_EXT4_ADDR);

	eps_status.v_bat_8 = eps_status.v_bat/100;
	eps_status.t_bat_8 = eps_status.t_bat/100;
	eps_status.v_solar_8 = eps_status.v_solar/100;
	eps_status.current_in_8 = eps_status.current_in/100;
	eps_status.current_out_8 = eps_status.current_out/100;
	eps_status.analog_ext1_8 = eps_status.analog_ext1 >> 8;
	eps_status.analog_ext2_8 = eps_status.analog_ext2 >> 8;
	eps_status.analog_ext3_8 = eps_status.analog_ext3 >> 8;
	eps_status.analog_ext4_8 = eps_status.analog_ext4 >> 8;
}

void eps_update_states()
{
	int i;
	for(i = 0; i < N_MODULES; i++)
	{
		if(module_status[i] == TURN_OFF)
		{
			//turn off module
			module_set_state(i, 0);
			module_status[i] = MODULE_OFF;
		}
		else if(module_status[i] == TURN_ON)
		{
			//turn on module
			module_set_state(i, 1);
			module_status[i] = MODULE_ON;
		}
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
	else if(eps_status.v_bat < THRESHOLD_10) //low bat voltage threshold
	{
		//turn off all modules except mb
		for(i = 0; i < N_MODULES; i++)
		{
			if(i != M_M)
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
			module_set_state(H_T1, 1);
			module_status[H_T1] = MODULE_ON;
			module_set_state(H_T2, 1);
			module_status[H_T2] = MODULE_ON;
			module_set_state(H_T3, 1);
			module_status[H_T3] = MODULE_ON;
		}
		else if(eps_status.t_bat > -500)
		{
			//turn off heater
			module_set_state(H_T1, 0);
			module_status[H_T1] = MODULE_OFF;
			module_set_state(H_T2, 0);
			module_status[H_T2] = MODULE_OFF;
			module_set_state(H_T3, 0);
			module_status[H_T3] = MODULE_OFF;
		}
	}
	else
	{
		if(eps_status.t_bat < -500)
		{
			//turn on heater
			module_set_state(H_T1, 1);
			module_status[H_T1] = MODULE_ON;
			module_set_state(H_T2, 1);
			module_status[H_T2] = MODULE_ON;
			module_set_state(H_T3, 1);
			module_status[H_T3] = MODULE_ON;
		}
		else if(eps_status.t_bat > 0)
		{
			//turn off heater
			module_set_state(H_T1, 0);
			module_status[H_T1] = MODULE_OFF;
			module_set_state(H_T2, 0);
			module_status[H_T2] = MODULE_OFF;
			module_set_state(H_T3, 0);
			module_status[H_T3] = MODULE_OFF;
		}
	}
}
