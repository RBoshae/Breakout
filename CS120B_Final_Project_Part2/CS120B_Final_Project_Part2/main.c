#include <avr/io.h>
#include <avr/sleep.h>
#include "NES_Controller.h"
#include "timer.h"


// ====================
// GLOBAL SHARED VARIABLES
// ====================
unsigned volatile char PADDLE_PORTA;
unsigned volatile char PADDLE_PORTB;

unsigned volatile char BALL_PORTA;
unsigned volatile char BALL_PORTB;




typedef struct task {
	int state; // Current state of the task
	unsigned long period; // Rate at which the task should tick
	unsigned long elapsedTime; // Time since task's previous tick
	int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[2];

const unsigned char tasksNum = 3;
const unsigned long tasksPeriodGCD  = 100;
const unsigned long periodBall = 200;
const unsigned long periodPaddle = 100;
const unsigned long periodOutput = 100;

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

	PADDLE_PORTA = bottom_row;
	PADDLE_PORTB = paddle_pos;
	
	return state;
};

// ====================
// BALL_TICK: BALL MOVEMENT ON LED matrix
// ====================

enum B_States {B_START, B_INIT,B_PAUSE, B_UP, B_DOWN, B_UP_LEFT, B_UP_RIGHT, B_DOWN_LEFT, B_DOWN_RIGHT} state;
int Ball_Tick(int state) {
	
	// === Local Variables ===
	static unsigned char ball_row = 0x40; // ball row
	static unsigned char ball_column = 0xFD; // controls ball movement

	unsigned char FLOOR = 0x40;
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
			state = B_UP_LEFT;
		}
		else
		{
			state = B_PAUSE;
		}
		break;

		case B_UP_LEFT:
		if (ball_column == LEFT_WALL)
		{
			state = B_UP_RIGHT;
		}
		else if (ball_row == CEILING)
		{
			state = B_DOWN_LEFT;
		}
		break;

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
		break;

		case  B_DOWN_LEFT:
		if (ball_column == LEFT_WALL)
		{
			state = B_DOWN_RIGHT;
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
	
	BALL_PORTA =  ball_row; // PORTA displays column pattern
	BALL_PORTB =  ball_column; // PORTB selects column to display pattern

	//PORTA =  ball_row | PORTA_OUTPUT; // PORTA displays column pattern
	//PORTB =  ball_column |PORTB_OUTPUT; // PORTB selects column to display pattern

	return state;
	
};


// ====================
// OUTPUT_TICK:OUTPUT TO LED matrix
// ====================
enum O_States {O_START, O_INIT, O_PADDLE_OUTPUT, O_BALL_OUTPUT};
int Output_Tick(int state) {

	// === Local Variables ===
		static unsigned char OUTPUT_A; // ball row
		static unsigned char OUTPUT_B; // controls ball movement
	
	// === Transitions ===
	switch (state) {
		case O_START:
			state = O_INIT;
		break;
		
		case O_INIT:
			state = O_PADDLE_OUTPUT;
		break;

		case O_PADDLE_OUTPUT:
			state = O_BALL_OUTPUT;
		break;

		case O_BALL_OUTPUT:
			state = O_PADDLE_OUTPUT;
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

		case O_PADDLE_OUTPUT:
			OUTPUT_A = PADDLE_PORTA;
			OUTPUT_B = PADDLE_PORTB;
		break;

		case O_BALL_OUTPUT:
			OUTPUT_A = BALL_PORTA;
			OUTPUT_B = BALL_PORTB;
		break;
			
		default:
			OUTPUT_A = 0x01;
			OUTPUT_B = 0xF8;
		break;
	}

	PORTA = OUTPUT_A;
	PORTB = OUTPUT_B;
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
	tasks[i].state = P_INIT;
	tasks[i].period = periodPaddle;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &Paddle_Tick;
	++i;
	tasks[i].state = B_INIT;
	tasks[i].period = periodBall;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &Ball_Tick;
	++i;
	tasks[i].state = O_INIT;
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