/*
 * communication.h
 *
 *  Created on: 09.05.2016
 *      Author: beat
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

char are_all_systems_rebooting();

//void i2c_callback(char *buffer);
int mainboard_poke_iterate();
void mainboard_poke_reset_counter();
int mainboard_poke_get_counter();

//offer callback function
//implement all the highlevel communication without care of low level protocol.

#endif /* COMMUNICATION_H_ */
