#include "state_machine.h"
#include "eps_hal.h"
#include <stdint.h>

static char system_on_off = 0;
eps_status_t eps_status;
module_status_t module_status[N_MODULES]; //stores the answers to be sent to an eventual i2c request

void turn_off_all_modules(char lowbat); //if lowbat==1, this forces everything to turn off immediately and enables comparator before deepsleep
void force_turn_on_mainboard();

#ifndef FIRMWARE_BASE_STATION
static char button_state = 0;
static char button_prev_state = 0;
#endif

void eps_update_values()
{
	float A0 = (int16_t)(ADC_read(AIN_A_EXT0_ADDR));
	//TODO: Conversion
	eps_status.analog_ext1 = ADC_read(AIN_A_EXT1_ADDR);
	eps_status.v_bat = (uint16_t)(ADC_read(AIN_V_BAT_ADDR) * 5000.0f / 4096.0f);
	eps_status.t_bat = (uint16_t)(A0 * 9100.0f / (2.0f*eps_status.analog_ext1 - A0));
	eps_status.v_solar = (uint16_t)(ADC_read(AIN_V_SC_ADDR) * 8250.0f / 4096.0f);
	eps_status.current_in = (uint16_t)(ADC_read(AIN_I_IN_ADDR) * 2500.0f / 4096.0f); // 20 mOhm resistor
	eps_status.current_out = (uint16_t)(ADC_read(AIN_I_OUT_ADDR) * 5000.0f / 4096.0f); // 10mOhm resistor

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
	static char buzzer_state = 0;
	if(eps_status.v_bat < BUZZER_THRESHOLD && eps_status.v_bat > ALL_OFF_THRESHOLD + THRESHOLD_BUZZER_HYS && system_on_off == 1)
		buzzer_state = 1;
	else if(eps_status.v_bat > BUZZER_THRESHOLD+THRESHOLD_BUZZER_HYS || eps_status.v_bat < ALL_OFF_THRESHOLD)
		buzzer_state = 0;

	if(buzzer_state == 1)
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
		buzzer_state = 0;
	}

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
					module_set_state(BUZZER, 1); //confirmation
					timer_delay100(6);
					module_set_state(BUZZER, 0);
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
			// wait till boot complete before changing internal state
			if(i==M_5_RPI)
			{
				module_set_state(M_5_RPI, 1);
				if(module_update_shutdown_signal(M_5_RPI, START_BOOT) == SYSTEM_ON)
				{
					module_status[M_5_RPI] = MODULE_ON;
					module_set_state(BUZZER, 1); //confirmation
					timer_delay100(3);
					module_set_state(BUZZER, 0);
				}
			}
#else
			// wait till boot complete before changing internal state
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

	if(eps_status.v_bat < THRESHOLD_0) //emergency off
	{
		turn_off_all_modules(1);
	}
	else if(eps_status.v_bat < ALL_OFF_THRESHOLD) //low bat voltage threshold
	{
		turn_off_all_modules(0);
	}
	else if(eps_status.v_bat < SYSTEMS_THRESHOLD) //turn off all modules except mb
	{
		for(i = 0; i < N_MODULES; i++)
		{
			if(i != M_M && i!= BUZZER)
			{
				//turn off module
				if(module_status[i] == MODULE_ON)
					module_status[i] = TURN_OFF;
			}
		}
	}

//	else if(eps_status.v_bat < THRESHOLD_40) // rather low bat voltage threshold -> reduce internal temp
//	{
//		if(eps_status.t_bat < -1000)
//		{
//			//turn on heater
//			module_set_state(H_T3, 1);
//			module_status[H_T3] = MODULE_ON;
//			module_set_state(H_T2, 1);
//			module_status[H_T2] = MODULE_ON;
//		}
//		else if(eps_status.t_bat > -500)
//		{
//			//turn off heater
//			module_set_state(H_T3, 0);
//			module_status[H_T3] = MODULE_OFF;
//			module_set_state(H_T2, 0);
//			module_status[H_T2] = MODULE_OFF;
//		}
//	}
//	else
//	{
//		if(eps_status.t_bat < -500)
//		{
//			//turn on heater
//			module_set_state(H_T3, 1);
//			module_status[H_T3] = MODULE_ON;
//			module_set_state(H_T2, 1);
//			module_status[H_T2] = MODULE_ON;
//		}
//		else if(eps_status.t_bat > 0)
//		{
//			//turn off heater
//			module_set_state(H_T3, 0);
//			module_status[H_T3] = MODULE_OFF;
//			module_set_state(H_T2, 0);
//			module_status[H_T2] = MODULE_OFF;
//		}
//	}

	// TURN ON mainboard if battery is again sufficiently charged.
	// function is also needed to turn on current measurement, so keep it also for FBS code!!
	if(module_status[M_M] == MODULE_OFF
			&& eps_status.v_bat > ALL_OFF_THRESHOLD + THRESHOLD_MODULE_HYS
			&& system_on_off == 1)
	{
		force_turn_on_mainboard();
	}

#ifndef FIRMWARE_BASE_STATION

	// TURN ON all systems if battery is again sufficiently charged:
	if(module_status[M_5_OLIMEX] == MODULE_OFF
			&& eps_status.v_bat > SYSTEMS_THRESHOLD + THRESHOLD_MODULE_HYS
			&& system_on_off == 1)
	{
		turn_on_all_rover_modules();
	}

#else
	// Workaround for ESD problem (if Raspi shuts down by accident)
	static int rpi_off_timeout = 0;

	if(module_status[M_5_RPI] == MODULE_ON && module_check_boot_state() == SHUTDOWN_COMPLETE)
	{
		rpi_off_timeout++;
		if(rpi_off_timeout > 40) // wait at least 20 s in case rpi is already rebooting...
		{
			module_set_state(M_5_RPI, 0);	// turn system off for reset; while GPS keeps running.
			module_status[M_5_RPI] = TURN_ON; // set state such that it gets turned on in the next loop.
			rpi_off_timeout = 0;
		}
	}
	else
		rpi_off_timeout = 0;
#endif


}



#define SHORT_BUTTON_TIME 	2
#define LONG_BUTTON_TIME		6
void eps_update_user_interface()
{
#ifdef FIRMWARE_BASE_STATION
	////////////////////////////  check button: //////////////////////////////////

	static int i;
	static uint8_t time_button_pressed = 0;
	if(PORT_DIGITAL_IN & PIN_DIGITAL_6)
	{//only check how long button was pressed if released for the first threshold

		if(time_button_pressed > LONG_BUTTON_TIME)
		{ // short button pressing; only turn off or on raspi. If GPS would have been off, it would also turn on.
			if(module_status[M_5_RPI] == MODULE_ON)
			{
				//long button pressing: turn off all systems to be able to charge battery.
				module_status[M_5_RPI] = TURN_OFF;
				module_status[M_5_GPS] = TURN_OFF;
			}
			else if(module_status[M_5_RPI] == MODULE_OFF)
			{
				module_status[M_5_GPS] = TURN_OFF;
			}
		}
		else if(time_button_pressed > SHORT_BUTTON_TIME) //at least 2 seconds
		{
			// short button pressing; only turn off or on raspi. If GPS would have been off, it would also turn on.
			if(module_status[M_5_RPI] == MODULE_ON)
			{
				module_status[M_5_RPI] = TURN_OFF;
			}
			else if(module_status[M_5_RPI] == MODULE_OFF)
			{
				if(eps_status.v_bat > BOOT_THRESHOLD)
				{
					module_status[M_5_RPI] = TURN_ON;
					module_status[M_5_GPS] = TURN_ON;
				}
			}
		}

		time_button_pressed = 0; //reset counter because button is released.
	}
	else
	{
		//directly give error sound if button action not allowed:
		if(module_status[M_5_RPI] == TURN_ON || module_status[M_5_RPI] == TURN_OFF)
		{
			for(i=3;i>0;i--) //error tone: 3 very short beeps.
			{
				module_set_state(BUZZER, 1);
				timer_delay50(1);
				module_set_state(BUZZER, 0);
				timer_delay50(1);
			}
		}
		else
		{
			if(time_button_pressed == SHORT_BUTTON_TIME) //at least 1.5 seconds
			{
				module_set_state(BUZZER, 1); //confirmation
				timer_delay100(1);
				module_set_state(BUZZER, 0);
			}
			else if(time_button_pressed == LONG_BUTTON_TIME)
			{
				module_set_state(BUZZER, 1); //confirmation
				timer_delay100(1);
				module_set_state(BUZZER, 0);
				timer_delay100(1);
				module_set_state(BUZZER, 1);
				timer_delay100(1);
				module_set_state(BUZZER, 0);
				timer_delay100(1);
			}

			time_button_pressed += 1;
		}
	}
	// LEDs:
	// battery full:
	if(eps_status.v_bat > THRESHOLD_95 && (eps_status.current_in-eps_status.current_out) < 100) //TODO: test this value in practice.
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_1);
	else if(eps_status.v_bat < THRESHOLD_95-THRESHOLD_LED_HYS)
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_1);

	// battery good:
	if(eps_status.v_bat > BOOT_THRESHOLD)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_2);
	else if(eps_status.v_bat < THRESHOLD_40-THRESHOLD_LED_HYS)
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_2);

	// EPS on / battery > 0%:
	if(eps_status.v_bat > THRESHOLD_0+THRESHOLD_LED_HYS)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_3);
	else if(eps_status.v_bat < THRESHOLD_0)
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_3);

	// RasPi on:
	if((module_status[M_5_RPI] == MODULE_ON) || (module_status[M_5_RPI] == TURN_OFF))
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_4);
	else
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_4);

	// GPS on:
	if(module_status[M_5_GPS] == MODULE_ON || module_status[M_5_GPS] == TURN_ON)
		SET_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_5);
	else
		CLR_PIN(PORT_DIGITAL_OUT, PIN_DIGITAL_5);

#else

	////////////////////////////  check button: //////////////////////////////////


	button_prev_state = button_state;

	if((PORT_DIGITAL_IN & PIN_DIGITAL_6) == 0)
	{
		button_state = 1;
		if(button_prev_state == 0 || get_and_clear_wakeup_source() == WAKEUP_FROM_COMPARATOR) //swich has been turned on.
		{
			//to avoid accidental turn on-off, check multiple times.
			timer_delay50(1);
			if((PORT_DIGITAL_IN & PIN_DIGITAL_6) == 0)
			{
				//turn on all systems:
				turn_on_all_rover_modules();
			}
			else
				button_state = 0;
		}

	}
	else
	{
		button_state = 0;
		if(button_prev_state == 1) //shut down all systems.
		{
			//to avoid accidental turn on-off, check multiple times.
			timer_delay50(1);
			if((PORT_DIGITAL_IN & PIN_DIGITAL_6) > 0)
			{
				system_on_off = 0;
				turn_off_all_modules(0);
			}
			else
				button_state = 1;
		}
	}

	//check that Olimex is in correct state (might not be because wait for boot not respected):
	if(system_on_off == 1 && module_status[M_5_OLIMEX] == MODULE_OFF && eps_status.v_bat > SYSTEMS_THRESHOLD + THRESHOLD_MODULE_HYS)
		module_status[M_5_OLIMEX] = TURN_ON;
	else if(system_on_off == 0 && module_status[M_5_OLIMEX] == MODULE_ON)
		module_status[M_5_OLIMEX] = TURN_OFF;

	//check if everyting is off and switch is on position off -> go to sleep!
	if(system_on_off == 0 && (PORT_DIGITAL_IN & PIN_BUTTON) == 1)
	{
		int n_sys_on = 0;
		int i;
		for(i = 0; i < N_MODULES; i++)
		{
			//turn off module
			if(module_status[i] != MODULE_OFF)
				n_sys_on++;
		}
		if(n_sys_on==0)
		{
			goto_deepsleep(0);
		}
	}

#endif
}

void turn_off_all_modules(char lowbat)
{
	int i;
	//turn off all modules (inc. Mainboard)
	for(i = 0; i < N_MODULES; i++)
	{
		if(lowbat)
		{
			module_set_state(i, 0);
			module_status[i] = MODULE_OFF;
		}
		else
		{
		//turn off module
		if(module_status[i] == MODULE_ON)
			module_status[i] = TURN_OFF;
		}
	}
	// Also turn off current measurement
	CLR_PIN(PORT_DIGITAL_OUT, PIN_INA_VCC);

	if(lowbat) //
		goto_deepsleep(lowbat);
}

#ifndef FIRMWARE_BASE_STATION

void force_turn_on_mainboard()
{
	//system can be turned on:
	system_on_off = 1;
	//enable current measurements:
	SET_PIN(PORT_DIGITAL_OUT, PIN_INA_VCC);
	//enable mainboard
	module_set_state(M_M, 1);
	module_status[M_M] = MODULE_ON;
}

void turn_on_all_rover_modules()
{

	//check for sufficient battery
	ADC_update(); //get ADC values
	eps_update_values();
	//Buzzer confirmation #1:
	module_set_state(BUZZER, 1);
	timer_delay100(1);
	module_set_state(BUZZER, 0);
	timer_delay100(1);

	if(eps_status.v_bat > ALL_OFF_THRESHOLD + THRESHOLD_MODULE_HYS
			&& (PORT_DIGITAL_IN & PIN_BUTTON) == 0) // switch is position: on
	{
		//needed at startup!
		button_state = 1;
		button_prev_state = 1;
		//Buzzer confirmation #2:
		module_set_state(BUZZER, 1);
		timer_delay100(1);
		module_set_state(BUZZER, 0);
		timer_delay100(1);
		force_turn_on_mainboard();
	}

	if(eps_status.v_bat > SYSTEMS_THRESHOLD + THRESHOLD_MODULE_HYS  && (PORT_DIGITAL_IN & PIN_BUTTON) == 0) // switch is position: on
	{
		//Buzzer confirmation #3:
		module_set_state(BUZZER, 1);
		timer_delay100(1);
		module_set_state(BUZZER, 0);
		timer_delay100(1);

		//enable GPS and rockblock
		module_set_state(M_52, 1);
		module_status[M_52] = MODULE_ON;

		timer_delay100(2);

		//enable Olimex
		module_set_state(M_5_OLIMEX, 1);
		module_status[M_5_OLIMEX] = TURN_ON;
		//enable Lidar logic
		module_set_state(M_331, 1);
		module_status[M_331] = MODULE_ON;

		timer_delay100(2);
		//enable Lidar motor
		module_set_state(M_11, 1);
		module_status[M_11] = MODULE_ON;
	}


}
#endif
