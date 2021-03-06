/*
 * eps_hal.h
 *
 *  Created on: 09.05.2016
 *      Author: beat
 */

#ifndef EPS_HAL_H_
#define EPS_HAL_H_

//#define FIRMWARE_BASE_STATION 	1
#define FIRMWARE_BALLOON

#include <msp430fr5969.h>

//---------------------------------------------------------------
// definitions

#define I2C_BUFFER_SIZE		8

#define N_MODULES	11
//indexes:
#define M_M			0 	//active low
#define M_SC			1	//active low
#define M_331		2
#define M_332		3
#define M_5			4
#define M_52			5
#define M_11			6
#ifdef FIRMWARE_BASE_STATION
	#define M_5_GPS			M_5
	#define M_5_RPI			M_11
#else
#ifndef FIRMWARE_BALLOON
	#define M_5_OLIMEX		M_5
#else
	//dont treat any of the outputs special with boot sequence etc.
#endif
#endif
#define M_DIRECT		7
#ifndef FIRMWARE_BALLOON
	#define BUZZER		8
#else
	#define BUZZER 		10 //Buzzer NOT USED!!!
	#define HOT_WIRE 	8
#endif
#define H_T2			9
#define 	H_T3			10

#define ON		1
// turning a module off is always allowed
#define OFF		0

#define PORT_3V3_M_EN		P3OUT
#define PIN_3V3_M_EN			BIT2

#define PORT_3V3_1_EN		P3OUT
#define PIN_3V3_1_EN			BIT3

#define PORT_3V3_2_EN		P4OUT
#define PIN_3V3_2_EN			BIT7

#define PORT_5V_EN			P3OUT
#define PIN_5V_EN			BIT1

#define PORT_5V2_EN			P3OUT
#define PIN_5V2_EN			BIT6

#define PORT_11V_EN			P3OUT
#define PIN_11V_EN			BIT0

#define PORT_DIRECT_EN		P3OUT
#define PIN_DIRECT_EN		BIT5

#define PORT_SC_EN			P3OUT
#define PIN_SC_EN			BIT4

#define PORT_HEATER_1_EN	PJOUT
#define PIN_HEATER_1_EN		BIT0

#define PORT_HEATER_2_EN	PJOUT
#define PIN_HEATER_2_EN		BIT1

#define PORT_HEATER_3_EN	PJOUT
#define PIN_HEATER_3_EN		BIT2

#define PORT_DIGITAL_OUT		P2OUT
#define PORT_DIGITAL_IN		P2IN
#define PIN_DIGITAL_1		BIT5		// LED battery full (95% or more)
#define PIN_DIGITAL_2		BIT6		// LED battery good (40% or more)
#define PIN_DIGITAL_3		BIT0		// LED EPS on (0% bat or more). On rover EPS, this is the INA213 supply pin
#define PIN_DIGITAL_4		BIT1		// LED rPI on
#define PIN_DIGITAL_5		BIT3		// LED dGPS on
#define PIN_DIGITAL_6		BIT4		// user button; use pull-up
//more meaningfull names:
#define PIN_INA_VCC			PIN_DIGITAL_3
#define PIN_BUTTON			PIN_DIGITAL_6
#define DIGITAL_IE			P2IE

//ANALOG PORTS USED AS DIGITAL GPIOs
#define PORT_ANALOG_OUT		P4OUT
#define PORT_ANALOG_DIR		P4DIR
#define PORT_ANALOG_IN		P4IN
#define PIN_A11				BIT3
#define PIN_A10				BIT2
#define PIN_A9				BIT1
#define PIN_A8				BIT0

#define PORT_MB_OUT			PORT_ANALOG_OUT
#define DIR_MB				PORT_ANALOG_DIR
#define PIN_MB_INT			PIN_A8
#define PIN_MB_RESET_N		PIN_A9

#define PORT_SHUTDOWN		PORT_ANALOG_OUT
#define PIN_SHUTDOWN			PIN_A11
#define PORT_BOOT_STATE		PORT_ANALOG_IN
#define PIN_BOOT_STATE		PIN_A10

#define TIMER0_A0_DELAY		0x4000
#define TIMER0_A1_ENABLE		1
#define TIMER0_A1_DELAY		0x0660 //ca. 100ms

#define POKE_COUNTER_LIMIT	121 // if no messages during N cycles, toggle the mainboard reset pin
#define RESET_COUNTER_LIMIT	10 // if reset still necessary after N cycles shut down all systems and reboot.

#define ANALOG_PORTS			6 //10
#define ANALOG_NUM_AVG		16

#define AIN_I_OUT_CH			0
#define AIN_I_IN_CH			1
#define AIN_V_SC_CH			2
#define AIN_V_BAT_CH			3
#define AIN_A_EXT0_CH		4
#define AIN_A_EXT1_CH		5
//#define AIN_A_EXT2_CH		8
//#define AIN_A_EXT3_CH		9
//#define AIN_A_EXT4_CH		10
//#define AIN_A_EXT5_CH		11

#define AIN_I_OUT_ADDR		0
#define AIN_I_IN_ADDR		1
#define AIN_V_SC_ADDR		2
#define AIN_V_BAT_ADDR		3
#define AIN_A_EXT0_ADDR		4
#define AIN_A_EXT1_ADDR		5
//#define AIN_A_EXT2_ADDR		6
//#define AIN_A_EXT3_ADDR		7
//#define AIN_A_EXT4_ADDR		8
//#define AIN_A_EXT5_ADDR		9
//-----------------------------------------------------------
//battery voltage threshold levels (0%: 3V [for testing at ambient temperature]; 100% = 4.1V)
#define BAT_FULL		4100
#define BAT_EMPTY	2700

#define BAT_FS		(BAT_FULL-BAT_EMPTY)

#define THRESHOLD_95		(uint16_t)(BAT_EMPTY+0.95*BAT_FS) 		//95% of charge (100% is 4.1V & 636 adc counts)
#define THRESHOLD_80		(uint16_t)(BAT_EMPTY+0.80*BAT_FS)	 	//80% of charge (100% is 4.1V & 636 adc counts)
#define THRESHOLD_60		(uint16_t)(BAT_EMPTY+0.60*BAT_FS)		//60% of charge
#define THRESHOLD_40		(uint16_t)(BAT_EMPTY+0.40*BAT_FS)		//40% of charge
#define THRESHOLD_30		(uint16_t)(BAT_EMPTY+0.30*BAT_FS)		//20% of charge
#define THRESHOLD_20		(uint16_t)(BAT_EMPTY+0.20*BAT_FS)		//20% of charge
#define THRESHOLD_15		(uint16_t)(BAT_EMPTY+0.15*BAT_FS)		//15% of charge
#define THRESHOLD_10		(uint16_t)(BAT_EMPTY+0.10*BAT_FS)		//10% of charge
#define THRESHOLD_5		(uint16_t)(BAT_EMPTY+0.05*BAT_FS)		//5% of charge
#define THRESHOLD_0		(uint16_t)(BAT_EMPTY)					//0% of charge

#define ALL_OFF_THRESHOLD	THRESHOLD_10 	//retain some charge for emergency
#define BUZZER_THRESHOLD		THRESHOLD_15
#ifdef FIRMWARE_BASE_STATION
	#define SYSTEMS_THRESHOLD	ALL_OFF_THRESHOLD
#else
	#define SYSTEMS_THRESHOLD	THRESHOLD_15 //switch off other systems than mainboard.
#endif

#define THRESHOLD_MODULE_HYS		(uint16_t)(BAT_FS*0.09)			//9% hysteresis when turning on modules again
#define THRESHOLD_BUZZER_HYS		(uint16_t)(BAT_FS*0.04)			//4% hysteresis for buzzer on/off
#define THRESHOLD_LED_HYS		(uint16_t)(BAT_FS*0.03)			//3% of hysteresis when turning on/off LEDs

//number of main loop cycles that have to pass with low battery voltage measurements before turn-off action is performed
#define EMERGENCY_OFF_N_CYCLES	100
#define ALL_OFF_N_CYCLES			50
#define SYSTEMS_OFF_N_CYCLES		50

//same for mainboard turn-on:
#define MAINBOARD_ON_N_CYCLES	20

//#define THRESHOLD_80	3800	 	//80% of charge (100% is 4.2V & 636 adc counts)
//#define THRESHOLD_60	3250		//60% of charge
//#define THRESHOLD_40	3000		//40% of charge
//#define THRESHOLD_20	2800		//20% of charge (0% is 2.5V)
//#define THRESHOLD_10	2700		//20% of charge (0% is 2.5V)
//#define THRESHOLD_0		2600		//0% of charge
//battery temperature thresholds in ADC counts
#define COLD_20			385		//too cold for charging
#define COLD_0			672		//ok for charging
#define HOT_30			1020	//too hot, shut lower the PID duty cycle
#define T_BAT_OK		800
//-----------------------------------------------------------

#define SET_PIN(port, pin) port|=pin
#define CLR_PIN(port, pin) port&=~(pin)

//-----------------------------------------------------------

void i2c_init();
int i2c_read();
int i2c_glimpse();
int i2c_available();
int i2c_send_byte(unsigned char data, int append);
int i2c_send_word(unsigned short data, int append);
void i2c_receive_callback(); //has to be implemented, is called by interrupt

void gpio_init();

void timer0_A_init();
void timer0_A_start();
void timer0_A_stop();
void timer_delay100(int t10);
void timer_delay50(int t50);


void ADC_init();
void ADC_update();
float ADC_read(int channel);

void module_set_state(int module_number, char state);

#define SHUTDOWN_COMPLETE	1
#define SYSTEM_ON			0
#define START_SHUTDOWN		2
#define START_BOOT			3
#define UNKNOWN_STATE		-1

int module_update_shutdown_signal(int module_number, char state);
int module_check_boot_state();

void mainboard_reset();

#define WAKEUP_NONE				0
#define WAKEUP_FROM_COMPARATOR	1
#define WAKEUP_FROM_BUTTON		2
char get_and_clear_wakeup_source();
void goto_deepsleep(char lowbat); //if lowbat == 1, deepsleep is due to low battery and comparator should be set to wake up!

//TODO s:
//module interface

#endif /* EPS_HAL_H_ */
