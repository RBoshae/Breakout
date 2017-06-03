// Created by Rick Boshae
//
// Permission to copy is granted provided that this header remains intact.
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////

// Returns 0 if no key pressed, else returns char 0, 1, ... 30, 31, ...
// If multiple keys pressed, returns
// Keypad must be connected to port C
// Keypad arrangement
//        Px4 Px5 Px6 Px7
//	  col 1   2   3   4
//  row  ______________
//Px0 1	| 1 | 2 | 3 | A
//Px1 2	| 4 | 5 | 6 | B
//Px2 3	| 7 | 8 | 9 | C
//Px3 4	| * | 0 | # | D

#ifndef NES_CONTROLLER_H
#define NES_CONTROLLER_H

#include <bit.h>

// NES Controller Setup Values
#define PORT    PORTC  // select Port
#define PIN     PINC   // select Pin
#define CLOCK   PORTC0 // replaces with 0  -- Red Wire
#define LATCH   PORTC1 // replaces with 1  -- Green Wire
#define DATA    PINC2  // replaces with 2  -- Yellow Wire

// NES Controller Constant Values
const unsigned char  NUMBER_OF_BUTTONS = 0x07;

////////////////////////////////////////////////////////////////////////////////
//Functionality - Gets input from a keypad via time-multiplexing
//Parameter: None
//Returns: A NES controller button press else '0'
unsigned char GetNESControllerButton() {
	
	unsigned char controller_serial_data = 0x00;  // Stores controller data

	PORT |= (1 << LATCH); // Latch Rise
	//PORTC = 0X02;
	PORT &= ~(1 << LATCH); // Latch Fall
	//PORTC = 0X00;

	for (unsigned char cont_index = 0x00; cont_index <= NUMBER_OF_BUTTONS; cont_index++)
	{
		controller_serial_data |= (((~PIN >> DATA) & 0x01) << cont_index);  // Read Data
		//controller_serial_data = (((~PINC & 0x04) >> 2 ) << cont_index) | controller_serial_data;

		PORT |= (1 << CLOCK);  // Clock Rise
		//PORTC = 0x01;
		PORT &= ~(1 << CLOCK);  //Clock Fall
		//PORTC = 0x00;
	}
	
	return controller_serial_data;

}

#endif //NES_CONTROLLER_H
