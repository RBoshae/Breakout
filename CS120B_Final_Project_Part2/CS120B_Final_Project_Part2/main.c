#include <avr/io.h>
#include <avr/sleep.h>
#include "NES_Controller.h"
#include "timer.h"

#define ARRAY_SIZE 18

// ====================
// GLOBAL SHARED VARIABLES
// ====================
// unsigned volatile char PADDLE_PORTA;
// unsigned volatile char PADDLE_PORTB;
// 
// unsigned volatile char BALL_PORTA;
// unsigned volatile char BALL_PORTB;
// 
// unsigned volatile char BRICK_PORTA;
// unsigned volatile char BRICK_PORTB;


unsigned volatile char DISPLAY_PORTA[ARRAY_SIZE];
unsigned volatile char DISPLAY_PORTB[ARRAY_SIZE];




typedef struct task {
	int state; // Current state of the task
	unsigned long period; // Rate at which the task should tick
	unsigned long elapsedTime; // Time since task's previous tick
	int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[4];

const unsigned char tasksNum = 4;
const unsigned long tasksPeriodGCD  = 1;
const unsigned long periodBall = 500;
const unsigned long periodPaddle = 50;
const unsigned long periodOutput = 1;
const unsigned long periodBrick = 50;

// ====================
// PADDLE_TICK: PADDLE MOVEMENT ON LED matrix
// ====================
enum P_States {P_START, P_INIT, P_PAUSE, P_WAIT, P_MOVE_LEFT, P_MOVE_RIGHT, P_BUTTON_HOLD};
int Paddle_Tick(int state) {
	
	// === Local Variables ===
	static unsigned char bottom_row = 0x80; // bottom row
	static unsigned char paddle_pos = 0xF8; // controls left right movement
	static unsigned char button_hold_count = 0x02;


	unsigned char NES_button = GetNESControllerButton();
	
	// === Transitions ===
	switch (state) {
		case P_START:
		state = P_INIT;
		break;
		
		case P_PAUSE:
		if (NES_button == (0x01 << 3))
		{
			state = P_WAIT;
		}
		else
		{
			state = P_PAUSE;
		}
		break;
		case P_INIT:
			state = P_WAIT;
		break;

		case P_WAIT:
		if (NES_button == (0x01 << 6))
		{
			state = P_MOVE_LEFT;
		}
		else if (NES_button ==  (0x01 << 7))
		{
			state = P_MOVE_RIGHT;
		}
		else
		{
			state = P_WAIT;
		}
		break;

		case P_MOVE_LEFT:
			state = P_BUTTON_HOLD;
		break;

		case P_MOVE_RIGHT:
			state = P_BUTTON_HOLD;
		break;

		case  P_BUTTON_HOLD:
			if (NES_button)
			{
				if (button_hold_count < 2)
				{
					state = P_BUTTON_HOLD;
					button_hold_count++;
				}
				else if ((button_hold_count >=2) && (NES_button == (0x01 << 6)))
				{
					state = P_MOVE_LEFT;
				}
				else if ((button_hold_count >=2) && (NES_button == (0x01 << 7)))
				{
					state = P_MOVE_RIGHT;
				}
			
			}
			else
			{
				state = P_WAIT;
			}
		break;
		
		default:
			state = P_INIT;
		break;
	}
	
	// === Actions ===
	switch (state) {
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

	//PADDLE_PORTA = bottom_row;
	//PADDLE_PORTB = paddle_pos;
	DISPLAY_PORTA[0] = bottom_row;
	DISPLAY_PORTB[0] = paddle_pos;
	return state;
};

// ====================
// BALL_TICK: BALL MOVEMENT ON LED matrix
// ====================

enum B_States {B_START, B_INIT,B_PAUSE, B_UP, B_DOWN, B_UP_LEFT, B_UP_RIGHT, B_DOWN_LEFT, B_DOWN_RIGHT} state;
int Ball_Tick(int state) {
	
	// === Local Variables ===
	static unsigned char ball_row = 0x40; // ball row
	static unsigned char ball_column = 0xFB; // controls ball movement
	unsigned char ball_collision = 0x00;

	unsigned char FLOOR = 0x80;
	unsigned char CEILING = 0x01;
	unsigned char LEFT_WALL = 0x7F;
	unsigned char RIGHT_WALL = 0xFE;
	
	unsigned char NES_button = GetNESControllerButton();
	// === Transitions ===
	switch (state) {
		case B_START:
			state = B_INIT;
		break;

		case B_INIT:
			state = B_PAUSE;
		break;

		case B_PAUSE:
			if (NES_button == (0x01 << 3))
			{
				//state = B_UP_LEFT;
				state = B_UP_LEFT;
			}
			else
			{
				state = B_PAUSE;
			}
		break;

		case B_UP_LEFT:
			if ((ball_column == LEFT_WALL) && (ball_row != CEILING))
			{
				state = B_UP_RIGHT;
			}
			else if ((ball_column == LEFT_WALL) && (ball_row == CEILING))
			{
				state = B_DOWN_LEFT;
			}
			else if ((ball_row == CEILING) && (ball_column != LEFT_WALL))
			{
				state = B_DOWN_LEFT;
			}

			// UP LEFT Collision Detection
			for (int i = 2; i < ARRAY_SIZE; i++)
			{
				//Brick Directly Above
				if ((DISPLAY_PORTA[i] & (ball_row >> 1)) && ((DISPLAY_PORTB[i] & ~ball_column) == 0))
				{
					if (ball_column == 0x7F)
					{
						state = B_DOWN_RIGHT;	
					}
					else
					{
						state = B_DOWN_LEFT;
					}
					DISPLAY_PORTB[i] = 0xFF;
					ball_collision = 0x01;
					break;
				}
			}
			if (ball_collision == 0x00)
			{
				for (int i = 2; i < ARRAY_SIZE; i++)
				{
					// Brick Above Left, Not Directly Above
					if ((DISPLAY_PORTA[i] & (ball_row >> 1)) && ((DISPLAY_PORTB[i] & (~ball_column)<<1) == 0))
					{
						state = B_DOWN_RIGHT;
						DISPLAY_PORTB[i] = 0xFF;
						break;
					}
				}
				break;
			}
			

		case B_UP_RIGHT:
			if ((ball_column == RIGHT_WALL) && (ball_row != CEILING))
			{
				state = B_UP_LEFT;
			}
			else if ((ball_column == RIGHT_WALL) && (ball_row == CEILING))
			{
				state = B_DOWN_LEFT;
			}
			else if ((ball_row == CEILING) && (ball_column != RIGHT_WALL))
			{
				state = B_DOWN_RIGHT;
			}
			
			// UP Right Collision Detection
			for (int i = 2; i < ARRAY_SIZE; i++)
			{
				// Brick Directly Above
				if ((DISPLAY_PORTA[i] & (ball_row >> 1)) && ((DISPLAY_PORTB[i] & ~ball_column) == 0))
				{
					if (ball_column == 0xFE)
					{
						state = B_DOWN_LEFT;
					}
					else
					{
						state = B_DOWN_RIGHT;
					}
					DISPLAY_PORTB[i] = 0xFF;
					ball_collision = 0x01;
					break;
				}
			}
			if (ball_collision == 0x00)
			{
				for (int k = 2; k < ARRAY_SIZE; k++)
				{
					// Brick Above to the Right, not directly above
					if ((DISPLAY_PORTA[k] & (ball_row >> 1)) && ((DISPLAY_PORTB[k] & (~ball_column)>>1) == 0))
					{
						state = B_DOWN_LEFT;
						DISPLAY_PORTB[k] = 0xFF;
						break;
					}
				}
			}
		break;

		case  B_DOWN_LEFT:
			if ((ball_row != FLOOR) && (ball_column == LEFT_WALL))
			{
				state = B_DOWN_RIGHT;
			}
			else if ((ball_row == FLOOR) && (ball_column == LEFT_WALL))
			{
				state = B_UP_RIGHT;
			}
			else if (ball_row == FLOOR)
			{
				state = B_UP_LEFT;
			}
		break;

		case  B_DOWN_RIGHT:
		if ((ball_column == RIGHT_WALL) && !(ball_row == FLOOR))
		{
			state = B_DOWN_LEFT;
		}
		else if ((ball_column == RIGHT_WALL) && (ball_row == FLOOR))
		{
			state = B_UP_LEFT;
		}
		else if (!(ball_column == RIGHT_WALL) && (ball_row == FLOOR))
		{
			state = B_UP_RIGHT;
		}
		break;
		
		default:
		state = B_INIT;
		break;
	}
	
	// === Actions ===
	switch (state) {
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

	DISPLAY_PORTA[1] =  ball_row; // PORTA displays column pattern
	DISPLAY_PORTB[1] =  ball_column; // PORTB selects column to display pattern
	
	//BALL_PORTA =  ball_row; // PORTA displays column pattern
	//BALL_PORTB =  ball_column; // PORTB selects column to display pattern

	//PORTA =  ball_row | PORTA_OUTPUT; // PORTA displays column pattern
	//PORTB =  ball_column |PORTB_OUTPUT; // PORTB selects column to display pattern

	return state;
	
};


// ====================
// BRICK_TICK: BALL MOVEMENT ON LED matrix
// ====================

enum Brick_States {BRICK_START, BRICK_INIT,BRICK_PAUSE, BRICK_DISPLAY};
int Brick_Tick(int state) {
	
	// === Local Variables ===
	//static unsigned char brick_row = 0x07; // ball row
	//static unsigned char brick_column = 0x00; // controls ball movement

	unsigned char NES_button = GetNESControllerButton();
	
	// === Transitions ===
	switch (state) {
		case BRICK_START:
			state = BRICK_INIT;
		break;

		case BRICK_INIT:
			state = BRICK_PAUSE;
		break;

		case BRICK_PAUSE:
			if (NES_button == (0x01 << 3))
			{
				state = BRICK_DISPLAY;
			}
			else
			{
				state = BRICK_PAUSE;
			}
		break;

		case  BRICK_DISPLAY:
		break;
		
		default:
			state = B_INIT;
		break;
	}
	
	// === Actions ===
	switch (state) {
		case BRICK_START:
		break;

		case BRICK_INIT:
			for (int i = 2; i < ARRAY_SIZE; i++)
			{
				if (i < 10)
				{
					DISPLAY_PORTA[i] = 0x01;
					DISPLAY_PORTB[i] = (0x7F >> (i-2)) | 0xFF<<(8-(i-2));  //Acts as a roll shift
					

					
				}
				else if ((i >= 10) && (i < 18))
				{
					DISPLAY_PORTA[i] = 0x02;
					DISPLAY_PORTB[i] = (0x7F >> (i - 10)) | 0xFF<<(8-(i-10)); //Acts as a roll shift
				}
				else if ((i >= 18) && (i < 26))
				{
					DISPLAY_PORTA[i] = 0x04;
					DISPLAY_PORTB[i] = (0x7F >> (i - 18)) | 0xFF<<(8-(i-18));
					
				}

			}
		break;

		case BRICK_PAUSE:
		break;

		case BRICK_DISPLAY:
		break;

		default:
		break;
	}

// 	for (unsigned char i = 2; i < 16; i++)
// 	{
// 		DISPLAY_PORTA[i] = brick_row;
// 		DISPLAY_PORTB[i] = brick_column;
// 
// 	}
// 		
	// 	BRICK_PORTA =  brick_row; // PORTA displays column pattern
	// 	BRICK_PORTB =  brick_column; // PORTB selects column to display pattern

	//PORTA =  ball_row | PORTA_OUTPUT; // PORTA displays column pattern
	//PORTB =  ball_column |PORTB_OUTPUT; // PORTB selects column to display pattern

	return state;
	
};


// ====================
// OUTPUT_TICK:OUTPUT TO LED matrix
// ====================
enum O_States {O_START, O_INIT, O_DISPLAY_OUTPUT};
int Output_Tick(int state) {

	// === Local Variables ===
		//static unsigned char OUTPUT_A; // ball row
		//static unsigned char OUTPUT_B; // controls ball movement
		static int index = 0;
	
	// === Transitions ===
	switch (state) {
		case O_START:
			state = O_INIT;
		break;
		
		case O_INIT:
			state = O_DISPLAY_OUTPUT;
		break;

		case O_DISPLAY_OUTPUT:
			state = O_DISPLAY_OUTPUT;
		break;
		
		default:
			state = O_INIT;
		break;
	}
	
	// === Actions ===
	switch (state) 
	{
		case O_START:
		break;
			
		case O_INIT:
				//PORTA = 0x00;
				//PORTB = 0xFF;
		break;

		case O_DISPLAY_OUTPUT:

				PORTA = DISPLAY_PORTA[index];
				PORTB = DISPLAY_PORTB[index];

				if (index == (ARRAY_SIZE - 1))
				{
					index = 0;
				} 
				else
				{
					index++;
				}
							
		break;
			
		default:
		break;
	}
	return state;
};


void TimerISR() {
	unsigned char i;
	for(i = 0; i < tasksNum; ++i) { // Heart of the scheduler code
		if (tasks[i].elapsedTime >= tasks[i].period)
		{ // Ready
			tasks[i].state = tasks[i].TickFct(tasks[i].state);
			tasks[i].elapsedTime = 0;
		}
		tasks[i].elapsedTime += tasksPeriodGCD;

	}
	TimerFlag = 1;
}


int main() {

	DDRB = 0xFF; PORTB = 0x00;
	DDRA = 0xFF; PORTA = 0x00; // Initialize to output to PORT A
	DDRC = 0x03; PORTC = 0x04;

	unsigned char i = 0;
	tasks[i].state = P_START;
	tasks[i].period = periodPaddle;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &Paddle_Tick;
	++i;
	tasks[i].state = B_START;
	tasks[i].period = periodBall;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &Ball_Tick;
	++i;
	tasks[i].state = BRICK_START;
	tasks[i].period = periodBrick;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &Brick_Tick;
	++i;
	tasks[i].state = O_START;
	tasks[i].period = periodOutput;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &Output_Tick;

	

 	TimerSet(tasksPeriodGCD);
 	TimerOn();
	
	while(1)
	{
		while (!TimerFlag);
		TimerFlag = 0;
			
		
	}
}