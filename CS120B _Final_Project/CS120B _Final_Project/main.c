#include <avr/io.h>
#include "timer.h"
#include "NES_Controller.h"

// === Global Variables ==

// ====================
// PADDLE_TICK: PADDLE MOVEMENT ON LED matrix
// ====================
enum P_States {P_START, P_INIT, P_PAUSE, P_WAIT, P_MOVE_LEFT, P_MOVE_RIGHT, P_BUTTON_HOLD} P_State;

void Paddle_Tick() {
	
	// === Local Variables ===
	static unsigned char bottom_row = 0x80; // bottom row
	static unsigned char paddle_pos = 0xF8; // controls left right movement
	static unsigned char button_hold_count = 0x02;


	unsigned char NES_button = GetNESControllerButton();
	
	// === Transitions ===
	switch (P_State) {
		case P_START:
			P_State = P_INIT;
		break;
		
		case P_PAUSE:
			if (NES_button == (0x01 << 3))
			{
				P_State = P_WAIT;
			}
			else
			{
				P_State = P_PAUSE;
			}
		break;
		case P_INIT:
			P_State = P_WAIT;
		break;

		case P_WAIT:		
			if (NES_button == (0x01 << 6))
			{
				P_State = P_MOVE_LEFT;
			}
			else if (NES_button ==  (0x01 << 7))
			{
				P_State = P_MOVE_RIGHT;
			}
			else 
			{
				P_State = P_WAIT;
			}
		break;

		case P_MOVE_LEFT:
			P_State = P_BUTTON_HOLD;
		break;

		case P_MOVE_RIGHT:
			P_State = P_BUTTON_HOLD;
		break;

		case  P_BUTTON_HOLD:
			if (NES_button)
			{	
				if (button_hold_count < 2)
				{
					P_State = P_BUTTON_HOLD;
					button_hold_count++;
				}
				else if ((button_hold_count >=2) && (NES_button == (0x01 << 6)))
				{
					P_State = P_MOVE_LEFT;
				}
				else if ((button_hold_count >=2) && (NES_button == (0x01 << 7)))
				{
					P_State = P_MOVE_RIGHT;
				}
				
			}
			else
			{
				P_State = P_WAIT;
			}
		break;
		
		default:
			P_State = P_INIT;
		break;
	}
	
	// === Actions ===
	switch (P_State) {
		case P_START:
		break;

		case P_INIT:
			bottom_row = 0x80; // bottom row
			paddle_pos = 0xF8; // controls left right movement
		break;

		case P_PAUSE:
		break;

		case P_WAIT:
			button_hold_count = 0x00;
		break;

		case P_MOVE_LEFT:
			if (paddle_pos == 0X1F)
			{
				// do nothing
			}
			else
			{
				paddle_pos = (paddle_pos << 1) | 0x01;
			}
				
		break;

		case P_MOVE_RIGHT:
			if (paddle_pos ==  0xF8)
			{
				// do nothing
			}
			else
			{
				paddle_pos = (paddle_pos >> 1) | 0x80;
			}
				
		break;

		case  P_BUTTON_HOLD:
		break;
				
		default:
		break;
	}

	//PORTA_OUTPUT = bottom_row;
	//PORTB_OUTPUT = paddle_pos;

	// PORTA = PORTA_OUTPUT; // PORTA displays column pattern
	// PORTB = PORTB_OUTPUT; // PORTB selects column to display pattern

	PORTA = bottom_row;
	PORTB = paddle_pos;
	
};

// ====================
// BALL_TICK: BALL MOVEMENT ON LED matrix
// ====================

enum B_States {B_START, B_INIT,B_PAUSE, B_UP, B_DOWN, B_UP_LEFT, B_UP_RIGHT, B_DOWN_LEFT, B_DOWN_RIGHT} B_State;

void Ball_Tick() {
	
	// === Local Variables ===
	static unsigned char ball_row = 0x40; // ball row
	static unsigned char ball_column = 0xFD; // controls ball movement

	unsigned char FLOOR = 0x40;
	unsigned char CEILING = 0x01;
	unsigned char LEFT_WALL = 0x7F;
	unsigned char RIGHT_WALL = 0xFE; 
	
	unsigned char NES_button = GetNESControllerButton();
	// === Transitions ===
	switch (B_State) {
		case B_START:
			B_State = B_INIT;
		break;

		case B_INIT:
			B_State = B_PAUSE;
		break;

		case B_PAUSE:
			if (NES_button == (0x01 << 3))
			{
				B_State = B_UP_LEFT;
			}
			else
			{
				B_State = B_PAUSE;
			}
		break;

		case B_UP_LEFT:
			if (ball_column == LEFT_WALL)
			{
				B_State = B_UP_RIGHT;
			}
			else if (ball_row == CEILING)
			{
				B_State = B_DOWN_LEFT;
			}
		break;

		case B_UP_RIGHT:
			if ((ball_column == RIGHT_WALL) && (ball_row != CEILING))
			{
				B_State = B_UP_LEFT;
			}
			else if ((ball_column == RIGHT_WALL) && (ball_row == CEILING))
			{
				B_State = B_DOWN_LEFT;
			}
			else if ((ball_row == CEILING) && (ball_column != RIGHT_WALL))
			{
				B_State = B_DOWN_RIGHT;
			}
		break;

		case  B_DOWN_LEFT:
			if (ball_column == LEFT_WALL)
			{
				B_State = B_DOWN_RIGHT;
			}
			else if (ball_row == FLOOR)
			{
				B_State = B_UP_LEFT;
			}
		break;

		case  B_DOWN_RIGHT:
			if ((ball_column == RIGHT_WALL) && !(ball_row == FLOOR))
			{
				B_State = B_DOWN_LEFT;
			}
			else if ((ball_column == RIGHT_WALL) && (ball_row == FLOOR))
			{
				B_State = B_UP_LEFT;
			}
			else if (!(ball_column == RIGHT_WALL) && (ball_row == FLOOR))
			{
				B_State = B_UP_RIGHT;
			}
		break;
		
		default:
			B_State = B_INIT;
		break;
	}
	
	// === Actions ===
	switch (B_State) {
		case B_START:
		break;

		case B_INIT:
			ball_row = 0x40; // bottom row
			ball_column = 0xFB; // controls left right movement
		break;

		case B_PAUSE:
		break;

		case B_UP_LEFT:
			ball_row = ball_row >> 1;
			ball_column = (ball_column << 1) | 0x01;
		break;

		case B_DOWN_LEFT:
			ball_row = ball_row << 1;
			ball_column = (ball_column << 1) | 0x01;
		break;

		case  B_UP_RIGHT:
			ball_row = ball_row >> 1;
			ball_column = (ball_column >> 1) | 0x80;
		break;

		case B_DOWN_RIGHT:
			ball_row = ball_row << 1;
			ball_column = (ball_column >> 1) | 0x80;
		break;
		
		default:
		break;
	}
	
	PORTA =  ball_row; // PORTA displays column pattern
	PORTB =  ball_column; // PORTB selects column to display pattern

	//PORTA =  ball_row | PORTA_OUTPUT; // PORTA displays column pattern
	//PORTB =  ball_column |PORTB_OUTPUT; // PORTB selects column to display pattern
	
};

int main(void) {

	DDRB = 0xFF; PORTB = 0x00;
	DDRA = 0xFF; PORTA = 0x00; // Initialize to output to PORT A

	DDRC = 0x03; PORTC = 0x04;

	
	//TimerSet(20);
	TimerSet(100);
	TimerOn();

	while(1) {
		
		Paddle_Tick();
		Ball_Tick();
		while (!TimerFlag);
		TimerFlag = 0;
		
		
	}
}