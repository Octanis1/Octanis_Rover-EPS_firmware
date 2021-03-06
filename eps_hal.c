/*
 * eps_hal.c
 *
 *  Created on: 09.05.2016
 *      Author: beat
 */

#include "eps_hal.h"

volatile unsigned char RXData[I2C_BUFFER_SIZE];
volatile unsigned int RXData_ptr_start = 0; //points to oldest, unread char
volatile unsigned int RXData_ptr_length = 0; //points to free spot
volatile unsigned char TXData[I2C_BUFFER_SIZE];
volatile unsigned int TXData_ptr_start = 0; //points to oldest, unread char
volatile unsigned int TXData_ptr_length = 0; //points to free spot

volatile int ADC_summing = 0; //
volatile unsigned int ADC_sum[ANALOG_PORTS];
float ADC_result[ANALOG_PORTS];
//volatile unsigned int TXData_ptr_start = 0; //points to next char to be sent
//volatile unsigned int TXData_ptr_end = 0; //points to end (free spot)

static char wakeup_source = WAKEUP_NONE;
//----------------------------------------------------------------------------------------------------------------------------------------------------
void gpio_init()
{
	//TODO make dependent of definitions
	P1OUT = 0x00;
	P1REN = 0x00;
	//P1OUT = 0xC0; //uncomment if internal i2c pullup required
	//P1REN = 0xC0;
	P1DIR = 0x00; //all inputs
	P1SEL0 = 0x3f; //p1.0-5 a0-a5, p1.6-7 i2c
	P1SEL1 = 0xff;

	P2OUT = BIT4; //external digital, user leds (P2.0,1,3,5,6), user button (P2.4: in, pull-up), rest pulldown (2.2, 2.7)
	P2REN = 0xff; //is dont care for outputs...
	P2DIR = BIT0 + BIT1 + BIT3 + BIT5 + BIT6;
	P2SEL0 = 0x00; //standard gpio
	P2SEL1 = 0x00;
	//Falling edge interrupt enable on button pin:
	P2IES |= PIN_BUTTON;
	P2IV = 0; //the previous register set may cause an interrupt.

	P3OUT = PIN_DIRECT_EN + PIN_3V3_M_EN; // module pins p3.0-6 output, 3.7 pulldown, everything off (main & direct are active low), SOLAR ON (active low)
	P3REN = 0xe0;
	P3DIR = 0x7f;
	P3SEL0 = 0x00; //standard gpio
	P3SEL1 = 0x00;

	P4OUT = PIN_MB_RESET_N; // module pin p4.7, A8-A11 used as digital GPIOs p4.0-3 (of which p4.2 (A10) is boot_state pin,
																// p4.3 (A11) is shutdown pin, p4.0 (A8) is MB interrupt,
																// p4.1 (A9) is MB reset), rest pulldown
	P4REN = 0xff; // all pulldowns, except MB reset is pull-up
	P4DIR = BIT7 + PIN_SHUTDOWN + PIN_MB_INT; // module pin 4.7, shutdown pin 4.3, mainboard poke 4.0, mainboard reset 4.1: by default high-Z with pull-up
	P4SEL0 = 0x00; // standard gpio
	P4SEL1 = 0x00;

	PJOUT = 0x00; //heater pins pj.0-2, pulldown on pj.3
	PJREN = 0x08;
	PJDIR = 0x07;
	PJSEL0 = 0x00; //standard gpio
	PJSEL1 = 0x00;
}

// Timer0 A interrupt service routine CC1-2, OV
// Timer interrupt currently not in use
#pragma vector=PORT2_VECTOR
__interrupt void port2_isr()
{
	P2IV = 0;

	//check if button is still switched on.
	if((PORT_DIGITAL_IN & PIN_BUTTON) == 0)
	{
		wakeup_source = WAKEUP_FROM_BUTTON;
		DIGITAL_IE &= ~PIN_BUTTON;
		//restart previously stopped timer:
		timer0_A_start();
		LPM4_EXIT;
	}
}
//----------------------------------------------------------------------------------------------------------------------------------------------------

int i2c_send_byte(unsigned char data, int append)
{
	int temp_ptr;
	//append to buffer
	if(append)
	{
		//check buffer space, return 0 if full
		if(TXData_ptr_length >= I2C_BUFFER_SIZE)
		{
			return 0;
		}

		//get end of buffer and postincrement length
		temp_ptr = TXData_ptr_start + TXData_ptr_length++;
		while(temp_ptr >= I2C_BUFFER_SIZE)
			temp_ptr = temp_ptr - I2C_BUFFER_SIZE;

		TXData[temp_ptr] = data;
		return 1;
	}
	else //empty and reset buffer and fill first position
	{
		TXData[0] = data;
		TXData_ptr_start = 0;
		TXData_ptr_length = 1;
		return 1;
	}
}


int i2c_send_word(unsigned short data, int append)
{
	//Send MSB first, pass on append flag
	if(i2c_send_byte((unsigned char)(data), append))
	{
		//If MSB sent successfully, append LSB
		if(i2c_send_byte((unsigned char)(data>>8), 1))
		{
			return 1;
		}
	}
	return 0;
}

void i2c_clear()
{
	//reset send and receive buffer
	RXData_ptr_start = 0;
	RXData_ptr_length = 0;
	TXData_ptr_start = 0;
	TXData_ptr_length = 0;
}

void i2c_init()
{
	//TODO verify interrupts
	UCB0CTLW0 = UCSWRST; // eUSCI_B in reset state
	UCB0CTLW0 |= UCMODE_3 | UCSYNC; // I2C slave mode, sync mode
	UCB0I2COA0 = 0x48 | UCOAEN; // own address is 48hex
	UCB0CTLW0 &= ~UCSWRST; // eUSCI_B in operational state

	UCB0IE |= UCTXIE0 + UCRXIE0 + UCSTPIE; // enable TX&RX-interrupt
}

int i2c_read()
{
	char temp = RXData[RXData_ptr_start];
	if(RXData_ptr_length > 0) //data available
	{
		//increment pointer and decrement length as char was read;
		RXData_ptr_start++;
		RXData_ptr_length--;
		while(RXData_ptr_start >= I2C_BUFFER_SIZE)
			RXData_ptr_start = RXData_ptr_start - I2C_BUFFER_SIZE;
		//return char
		return temp;
	}
	//return -1 if no char read, unblocking call
	return -1;
}

int i2c_glimpse()
{
	if(RXData_ptr_length > 0) //data available
	{
		//if bufffer not empty, return char
		return RXData[RXData_ptr_start];
	}
	//return -1 if no char read, unblocking call
	return -1;
}

int i2c_available()
{
	return RXData_ptr_length;
}


// USCI_B0 Data ISR
#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
	if(UCB0IFG & UCSTPIFG) //Reset after stop condition,TODO: not sure if required
	{
		UCB0IFG &= ~UCSTPIFG;
	}
	else if(UCB0IFG & UCTXIFG0) //TX
	{
		//hold byte to send in temp
		char temp = TXData[TXData_ptr_start];
		if(TXData_ptr_length > 0) //data available, send 0x00 otherwise
		{
			//increment pointer and decrement length as char was read;
			TXData_ptr_start++;
			TXData_ptr_length--;
			while(TXData_ptr_start >= I2C_BUFFER_SIZE)
				TXData_ptr_start = TXData_ptr_start - I2C_BUFFER_SIZE;
			//return char
			UCB0TXBUF = temp;
		}
		else
		{
			UCB0TXBUF = 0x00;
		}
	} else if(UCB0IFG & UCRXIFG0) //RX
	{

		//get end of buffer and postincrement length
		int temp_ptr = RXData_ptr_start + RXData_ptr_length++;
		while(temp_ptr >= I2C_BUFFER_SIZE)
			temp_ptr = temp_ptr - I2C_BUFFER_SIZE;

		RXData[temp_ptr] = UCB0RXBUF;	  //read command

		if(RXData_ptr_length > I2C_BUFFER_SIZE)
		{
			RXData_ptr_length--;
			RXData_ptr_start++;
			while(RXData_ptr_start >= I2C_BUFFER_SIZE)
				RXData_ptr_start = RXData_ptr_start - I2C_BUFFER_SIZE;
		}

		i2c_receive_callback();
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

void timer0_A_init()
{
	TA0R = 0x0000;                       // Reset timer counter
	TA0CCR0 = TIMER0_A0_DELAY;           // Set timing offset of A0
	TA0CCR1 = TIMER0_A1_DELAY;           // Set timing offset of A1
	TA0CCTL0 = CCIE;                     // TA0CCR0 interrupt enabled

	TA0EX0 = TAIDEX_7;					 // Prescaler: divide by 8
	TA0CTL = TASSEL_2 + MC__CONTINUOUS + ID_3;     // activate timer, SMCLK, contmode, prescaler 1:8
}

void timer0_A_start()
{
	TA0CTL = TASSEL_2 + MC__CONTINUOUS + ID_3;     // activate timer, SMCLK, contmode, prescaler 1:8
}

void timer0_A_stop()
{
	TA0CTL = TASSEL_2 + MC__STOP + ID_3;     // activate timer, SMCLK, contmode, prescaler 1:8
}

//multiple of 100ms delay
void timer_delay100(int t100)
{
#if TIMER0_A1_ENABLE
	TA0CCR1 = TA0R + TIMER0_A1_DELAY;
	TA0CCTL1 = CCIE;                     // TA0CCR1 interrupt enabled
#endif
	for(; t100>0; t100 = t100-1)
	{
		TA0CCR0 += TIMER0_A1_DELAY; //also delay the other timer such that it doesnt wake up before!
		__bis_SR_register(CPUOFF + GIE);        // wait for timer interrupt
		__no_operation();                       // Set breakpoint >>here<< and read

	}
#if TIMER0_A1_ENABLE
	TA0CCTL1 &= ~CCIE;                     // TA0CCR1 interrupt disabled
#endif
}

//multiple of 100ms delay
void timer_delay50(int t50)
{
#if TIMER0_A1_ENABLE
	TA0CCR1 = TA0R + TIMER0_A1_DELAY/2;
	TA0CCTL1 = CCIE;                     // TA0CCR1 interrupt enabled
#endif
	for(; t50>0; t50 = t50-1)
	{
		TA0CCR0 += TIMER0_A1_DELAY/2; //also delay the other timer such that it doesnt wake up before!
		__bis_SR_register(CPUOFF + GIE);        // wait for timer interrupt
		__no_operation();                       // Set breakpoint >>here<< and read

	}
#if TIMER0_A1_ENABLE
	TA0CCTL1 &= ~CCIE;                     // TA0CCR1 interrupt disabled
#endif
}


// Timer0 A interrupt service routine CC0
// Wake up processor to spin main loop once
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0 ()
{
	TA0CCR0 += TIMER0_A0_DELAY;
	__bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

// Timer0 A interrupt service routine CC1-2, OV
// Timer interrupt currently not in use
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer0_A1 ()
{
	TA0CCR1 += TIMER0_A1_DELAY;
	TA0CCTL1 &= ~CCIFG;
	__bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

//----------------------------------------------------------------------------------------------------------------------------------------------------

//enum adc_status_{
//	IDLE, 	//not in use, and not to be triggered
//	ADC_BUSY, 	//wait for it to finish
//	START, 	//the periodic timer interrupt requires a new measurement, therefore start a new ADC conversion
//	DONE		//ADC conversion is done, read values to status variable
//} adc_status;

void ADC_init()
{

	//adc_status = IDLE;
	//reference voltage (2.5V)
	REFCTL0 = REFVSEL_2;
	//4 adclock sh, msc set, adcon;
	ADC12CTL0 = ADC12SHT1_0 + ADC12SHT1_0 + ADC12MSC + ADC12ON;
	//prediv 1:4, adcshp, adc12div 1:8, SMCLK, sequence
	ADC12CTL1 = ADC12PDIV_1 + ADC12SHP + ADC12DIV_7 + ADC12SSEL_3 + ADC12CONSEQ_1;
	//12bit res, low power mode (max 50ksps)
	ADC12CTL2 = ADC12RES_2 + ADC12PWRMD;
	//
	ADC12CTL3 = 0x00;

	//Vref as reference
	ADC12MCTL0 = ADC12VRSEL_1 + AIN_I_OUT_CH;
	ADC12MCTL1 = ADC12VRSEL_1 + AIN_I_IN_CH;
	ADC12MCTL2 = ADC12VRSEL_1 + AIN_V_SC_CH;
	ADC12MCTL3 = ADC12VRSEL_1 + AIN_V_BAT_CH;
	ADC12MCTL4 = ADC12VRSEL_1 + AIN_A_EXT0_CH;
	ADC12MCTL5 = ADC12VRSEL_1 + AIN_A_EXT1_CH + ADC12EOS;
//	ADC12MCTL6 = ADC12VRSEL_1 + AIN_A_EXT2_CH;
//	ADC12MCTL7 = ADC12VRSEL_1 + AIN_A_EXT3_CH;
//	ADC12MCTL8 = ADC12VRSEL_1 + AIN_A_EXT4_CH;
//	ADC12MCTL9 = ADC12VRSEL_1 + AIN_A_EXT5_CH + ADC12EOS;

	// interrupt
	ADC12IER0 = ADC12IE5; //interrupt generated after conversion of last value (i.e. ADC channel 5)
}

void ADC_update()
{
	int i;
	//reset buffer
	ADC_summing = 0;
	for(i = 0; i < ANALOG_PORTS; i++)
	{
		ADC_sum[i] = 0;
	}

	//start ADC
	ADC12CTL0 |= ADC12ENC + ADC12SC;

	//wait for ADC to finish
	while(ADC_summing < ANALOG_NUM_AVG)
	{
		__bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
		__no_operation();                       // Set breakpoint >>here<< and read
	}

	//TODO: calculate result for right units
	for(i = 0; i < ANALOG_PORTS; i++)
	{
		ADC_result[i] = ADC_sum[i] / ANALOG_NUM_AVG;
	}
}

float ADC_read(int port)
{
	if(port >= 0 && port < ANALOG_PORTS)
		return ADC_result[port];
	return 0;
}

// ADC interrupt after all values are read.
#pragma vector=ADC12_VECTOR
__interrupt void ADC_ISR ()
{
	//add new values to the buffer
	ADC_sum[0] += ADC12MEM0;
	ADC_sum[1] += ADC12MEM1;
	ADC_sum[2] += ADC12MEM2;
	ADC_sum[3] += ADC12MEM3;
	ADC_sum[4] += ADC12MEM4;
	ADC_sum[5] += ADC12MEM5;
//	ADC_sum[6] += ADC12MEM6;
//	ADC_sum[7] += ADC12MEM7;
//	ADC_sum[8] += ADC12MEM8;
//	ADC_sum[9] += ADC12MEM9;

	if(++ADC_summing < ANALOG_NUM_AVG)
	{
		//start another conversion series
		ADC12CTL0 |= ADC12ENC + ADC12SC;
	}
	else
	{
		//stop ADC (automatic)
		ADC12CTL0 &= ~ADC12ENC;
		//wake up CPU
		__bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
	}
}

void module_set_state(int module_number, char state)
{
	switch(module_number)
	{
		case M_M: //ACTIVE LOW
			if(state) CLR_PIN(PORT_3V3_M_EN, PIN_3V3_M_EN);
			else SET_PIN(PORT_3V3_M_EN, PIN_3V3_M_EN);
			break;
		case M_331:
			if(state) SET_PIN(PORT_3V3_1_EN, PIN_3V3_1_EN);
			else CLR_PIN(PORT_3V3_1_EN, PIN_3V3_1_EN);
			break;
		case M_332:
			if(state) SET_PIN(PORT_3V3_2_EN, PIN_3V3_2_EN);
			else CLR_PIN(PORT_3V3_2_EN, PIN_3V3_2_EN);
			break;
		case M_5:
			if(state) SET_PIN(PORT_5V_EN, PIN_5V_EN);
			else CLR_PIN(PORT_5V_EN, PIN_5V_EN);
			break;
		case M_52:
			if(state) SET_PIN(PORT_5V2_EN, PIN_5V2_EN);
			else CLR_PIN(PORT_5V2_EN, PIN_5V2_EN);
			break;
		case M_11:
			if(state) SET_PIN(PORT_11V_EN, PIN_11V_EN);
			else CLR_PIN(PORT_11V_EN, PIN_11V_EN);
			break;
		case M_DIRECT: //ACTIVE LOW
			if(state) CLR_PIN(PORT_DIRECT_EN, PIN_DIRECT_EN);
			else SET_PIN(PORT_DIRECT_EN, PIN_DIRECT_EN);
			break;
		case M_SC: //ACTIVE LOW
			if(state) CLR_PIN(PORT_SC_EN, PIN_SC_EN);
			else SET_PIN(PORT_SC_EN, PIN_SC_EN);
			break;
#ifdef FIRMWARE_BALLOON
		case HOT_WIRE:
#else
		case BUZZER:
#endif
			if(state) SET_PIN(PORT_HEATER_1_EN, PIN_HEATER_1_EN);
			else CLR_PIN(PORT_HEATER_1_EN, PIN_HEATER_1_EN);
			break;
		case H_T2:
			if(state) SET_PIN(PORT_HEATER_2_EN, PIN_HEATER_2_EN);
			else CLR_PIN(PORT_HEATER_2_EN, PIN_HEATER_2_EN);
			break;
		case H_T3:
			if(state) SET_PIN(PORT_HEATER_3_EN, PIN_HEATER_3_EN);
			else CLR_PIN(PORT_HEATER_3_EN, PIN_HEATER_3_EN);
			break;
	}
}

int module_update_shutdown_signal(int module_number, char state){
	static int power_off_counter = 0; //add some extra delay after the GPIO is low.
	static int force_change_state_counter = 0;
	if(state == START_SHUTDOWN)
	{
		SET_PIN(PORT_SHUTDOWN, PIN_SHUTDOWN);
		if((PORT_BOOT_STATE & PIN_BOOT_STATE) == 0)
			power_off_counter++;
		else if(power_off_counter<3) //to avoid accidental change of state.
			power_off_counter = 0;

		force_change_state_counter++;

		if(((power_off_counter>3) && (force_change_state_counter>90)) || force_change_state_counter > 180) //about 100 sec more delay. Also force power off if not able to shut down during 3 minutes
		{
			CLR_PIN(PORT_SHUTDOWN, PIN_SHUTDOWN); //!! to avoid leakage into GPIO while off.

			force_change_state_counter = 0;
			power_off_counter = 0;
			return SHUTDOWN_COMPLETE;
		}

		return START_SHUTDOWN;
	}
	else if(state == START_BOOT)
	{
		force_change_state_counter++;
		CLR_PIN(PORT_SHUTDOWN, PIN_SHUTDOWN);
		if((PORT_BOOT_STATE & PIN_BOOT_STATE) || force_change_state_counter > 180)
		{
			force_change_state_counter = 0;
			return SYSTEM_ON;
		}
		else
			return START_BOOT;
	}
	else
		return UNKNOWN_STATE;
}

int module_check_boot_state()
{
	if((PORT_BOOT_STATE & PIN_BOOT_STATE) == 0)
		return SHUTDOWN_COMPLETE;
	else
		return SYSTEM_ON;

}

void mainboard_reset()
{
	int i;

	// set reset pin as low output:
	CLR_PIN(PORT_MB_OUT, PIN_MB_RESET_N);
	SET_PIN(DIR_MB, PIN_MB_RESET_N);

	for(i = 100; i > 0; i--);
//	{
//		__bis_SR_register(CPUOFF + GIE);        // Enter LPM0 w/ interrupts
//		__no_operation();                       // Set breakpoint >>here<< and read
//	}

	// set reset pin high-Z with pull-up:
	SET_PIN(PORT_MB_OUT, PIN_MB_RESET_N);
	CLR_PIN(DIR_MB, PIN_MB_RESET_N);
}

void goto_deepsleep(char lowbat)
{
	DIGITAL_IE |= PIN_BUTTON;
	timer0_A_stop(); //stop timer to make it possible to turn off SMCLK.

	if(lowbat) //reason was low battery --> enable comperator to wake up
	{
		// configure comparator:
		// enable C3 = VBAT (+) and C5 = 1/2*VCC resistive divider (-) inputs
		CECTL0 = CEIPEN + CEIPSEL_3 + CEIMEN + CEIMSEL_5;
		CECTL1 = CEON + CEPWRMD_2;      // Enable comp; ultra-low power md; rising edge; don't use reference.
		CEINT = CEIE;                   // Enable interrupt for Comparator and clear them all.
	}
	__bis_SR_register(LPM4_bits + GIE);        // Enter LPM0 w/ interrupts
}

char get_and_clear_wakeup_source()
{
	char retval = wakeup_source;
	wakeup_source = WAKEUP_NONE;
	return retval;
}


// Comp_A interrupt service routine -- toggles LED
#pragma vector=COMP_E_VECTOR
__interrupt void Comp_A_ISR (void)
{
	CEINT &= ~(CEIFG + CEIE); // Clear Interrupt flag and disable interrupt

	//check if button is still switched on.
	if((PORT_DIGITAL_IN & PIN_BUTTON) == 0)
	{
		wakeup_source = WAKEUP_FROM_COMPARATOR;

		//restart previously stopped timer:
		timer0_A_start();
		LPM4_EXIT;
	}
}
