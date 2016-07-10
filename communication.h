/*
 * communication.h
 *
 *  Created on: 09.05.2016
 *      Author: beat
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

//void i2c_callback(char *buffer);
int i2c_respond_command(); // reacts to i2c commands, updating eps state
int mainboard_poke_iterate(int *mainboard_poke_counter);

//offer callback function
//implement all the highlevel communication without care of low level protocol.

#endif /* COMMUNICATION_H_ */
