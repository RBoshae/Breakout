/*controller_serial_data
 * NES_Controller_Test.c
 *
 * Created: 5/27/2017 2:33:19 AM
 * Author : Rick Boshae
 */ 

#include <avr/io.h>
#include "timer.h"

// Pins
#define CLOCK   PORTB0 // replaces with 0  -- Red Wire
#define LATCH   PORTB1 // replaces with 1  -- Green Wire
#define DATA    PORTB2 // replaces with 2  -- Yellow Wire

//button bit order 0-7 --> A, B, SELECT, START, UP, DOWN, LEFT, RIGHT

unsigned char controller_serial_data = 0x00; // 8 bits of controller data
unsigned char serial_data_index = 0x00;
const unsigned char  NUMBER_OF_BUTTONS = 0x07;

enum NESCont_States {NESCont_Start, INIT, PULL_LATCH_UP, PULL_LATCH_DOWN, CLOCK_RISE, READ_DATA, CLOCK_FALL} NESCont_State;
void NESCont_Tick()
{
	switch(NESCont_State) { // Transitions

		case (NESCont_Start) :
			NESCont_State  = INIT;
		break;

		case (INIT) :
			NESCont_State = PULL_LATCH_UP;
		break;

		case (PULL_LATCH_UP) :
			NESCont_State = PULL_LATCH_DOWN;
		break;

		case (PULL_LATCH_DOWN) :
			NESCont_State = READ_DATA;
			serial_data_index = 0x00; 
		break;

		case (READ_DATA) :
			if (serial_data_index <= NUMBER_OF_BUTTONS)
			{
				NESCont_State = CLOCK_RISE;
			}
			else
			{
				NESCont_State = PULL_LATCH_UP;
				serial_data_index = 0x00;
			}
		break;

		case (CLOCK_RISE) :
				NESCont_State = CLOCK_FALL;
		break;

		case (CLOCK_FALL) :
			NESCont_State = READ_DATA;
		break;

		default :
			NESCont_State = INIT;
		break;
	}

	switch(NESCont_State) { // Actions

		case (INIT) :
			controller_serial_data = 0x00;
		break;

		case (PULL_LATCH_UP) :
			controller_serial_data = 0x00;
			//PORTB = (1 << LATCH);
			PORTB = 0x02;
		break;

		case (PULL_LATCH_DOWN) :
			//PORTB = (0 << LATCH);
			PORTB = 0x00;
		break;

		case (READ_DATA) :
			controller_serial_data = (((~PINB & 0x04) >> 2 ) << serial_data_index) | controller_serial_data;
			//controller_serial_data = (~PINB & 0x04) << serial_data_index | controller_serial_data;
		break;
		
		case (CLOCK_RISE) :
			//PORTB = (1 << CLOCK);
			PORTB = 0x01;
		break;

		case (CLOCK_FALL) :
			//PORTB = (0 << CLOCK);
			PORTB = 0x00;
			serial_data_index++;
		break;

		default :
			PORTB = 0x00;
		break;
	}
	
}

int main(void) {

	//DDRB = 0x06; PORTB = 0x01;
	
	//unsigned char PORTB_SETTING = ~(1 << LATCH) | ~(1 << CLOCK) | (1 << DATA); //sets latch and clock low
	//PORTB = PORTB_SETTING; DDRB = ~PORTB_SETTING;

	DDRB = 0xFB; PORTB = 0x04; 
	DDRC = 0xFF; PORTC = 0x00; // Initialize to output to PORT C

	TimerOn();
	//TimerSet(1);

	while(1) {
		//GetNESControllerData();
		NESCont_Tick();		
		while (!TimerFlag)
		{
			TimerFlag = 0;
		}
		
		//Display results here!
		PORTC = controller_serial_data;
		
	}
}