#include <avr/io.h>
#include <avr/sleep.h>
#include "NES_Controller.h"
#include "timer.h"
#include "io.c"

#define ARRAY_SIZE 26



// ====================
// GLOBAL SHARED VARIABLES
// ====================
unsigned volatile char DISPLAY_PORTA[ARRAY_SIZE];
unsigned volatile char DISPLAY_PORTB[ARRAY_SIZE];
unsigned volatile char SCORE = 0; 

unsigned char FLOOR = 0x80;
unsigned char CEILING = 0x01;
unsigned char LEFT_WALL = 0x7F;
unsigned char RIGHT_WALL = 0xFE;

unsigned char NES_SELECT = 0x01 << 2;
unsigned char NES_START = 0x01 << 3;
unsigned char NES_LEFT_DPAD = 0x01 << 6;
unsigned char NES_RIGHT_DPAD = 0x01 << 7;

unsigned char gameInPlay = 0x00;
unsigned char gamePause = 0x00;
unsigned char gameSet = 0x00; 
unsigned char gamePlay = 0x00;
unsigned char gameEndTurn = 0x00;
unsigned char gameOver = 0x00;

// ====================
// TASK DECLARATIONS
// ====================
typedef struct task {
	int state; // Current state of the task
	unsigned long period; // Rate at which the task should tick
	unsigned long elapsedTime; // Time since task's previous tick
	int (*TickFct)(int); // Function to call for task's tick
} task;

task tasks[6];

const unsigned char tasksNum = 6;
const unsigned long tasksPeriodGCD  = 1;
const unsigned long periodBall = 200;
const unsigned long periodPaddle = 50;
const unsigned long periodOutput = 1;
const unsigned long periodBrick = 50;
const unsigned long periodLCDOutput = 800; // 4x ball period
const unsigned long periodGame = 50;


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
	
		case P_INIT:
			state = P_WAIT;
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
	static unsigned char ball_row = 0x40;    // ball row
	static unsigned char ball_column = 0xFB; // controls ball movement

	unsigned char ball_collision = 0x00;


	
	unsigned char NES_button = GetNESControllerButton();
	
	// === Local Functions ===
	void turn_off_LED(int LED_index){
		if ((LED_index >=0) && (LED_index < ARRAY_SIZE))
		{
			DISPLAY_PORTA[LED_index] = 0x00;
			DISPLAY_PORTB[LED_index] = 0x00;
		}

	}

	void paddle_collision_detection(){
		
		// ball is in the second row
		if (ball_row == (FLOOR>>1))
		{
			unsigned char ballBit;
			unsigned char DISPLAY_PORTB_BIT;
			unsigned char collision_point = 0x00;


			for (int i = 0; i < 8; i++)
			{
				ballBit = (ball_column >> i) & 0x01;
				DISPLAY_PORTB_BIT = (DISPLAY_PORTB[0] >> i) & 0x01;

				if ((ballBit == 0x01) && (DISPLAY_PORTB_BIT == 0x00))
				{
					collision_point++; 	
					
				}
				// Ball is either above left or above right from the paddle. 
				else if ((ballBit == 0x00) && (DISPLAY_PORTB_BIT == 0x01))
				{
					// Ball is above right of paddle
					if (( /*((ball_column>>(i+1)) == 0x01 ) &&*/((DISPLAY_PORTB[0]>>(i+1))& 0x01)) == 0x00 )
					{
						if (state == B_DOWN_LEFT)
						{
							if (ball_column == RIGHT_WALL)
							{
								state = B_UP_LEFT;
							}
							else
							{
								state = B_UP_RIGHT;
								collision_point = 0x08;
								break;
							}
						}
						else
						{
							collision_point = 0x09;
							break;
						}
					}
					// Ball is above left of paddle
					if (( /*((ball_column>>(i+1)) == 0x01 ) &&*/((DISPLAY_PORTB[0]>>(i-1)) & 0x01)) == 0x00 )
					{
						if (state == B_DOWN_RIGHT)
						{
							if (ball_column == LEFT_WALL)
							{
								state = B_UP_LEFT;
							}
							state = B_UP_LEFT;
							collision_point = 0x08;
							break;
						}
						else
						{
							collision_point = 0x09;
							break;
						}
					}

				}
				else if ((ballBit == 0x00) && (DISPLAY_PORTB_BIT == 0x00))
				{
					break;
				}
			}

			// ===  Paddle Collision Conditions  ===
			// Ball hits right side of the paddle
			if (collision_point == 0x00)
			{	
				if (ball_column == RIGHT_WALL)
				{
					state = B_UP_LEFT;
				}
				else 
				{
					state = B_UP_RIGHT;
				}
			}
			else if (collision_point == 0x01)
			{
				if (state == B_DOWN_RIGHT)
				{
					state = B_UP_RIGHT;
				}
				if (state == B_DOWN_LEFT)
				{
					state = B_UP_LEFT;
				}
			}
			else if (collision_point == 0x02) 
			{
				state = B_UP_LEFT;
			}
			else if (collision_point == 0x08)
			{
				//do nothing
			}
			else
			{
				state = B_PAUSE;
				gameEndTurn--;
			}
		}
	}

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

		case B_UP:
			if (ball_row == CEILING)
			{
				state = B_DOWN;
			}
			// === UP Brick Collision Detection ===
			for (int i = 2; i < ARRAY_SIZE; i++)
			{
				//Brick Directly Above -- works
				if ((DISPLAY_PORTA[i] & (ball_row >> 1)) && ((DISPLAY_PORTB[i] & ~ball_column) == 0))
				{
					state = B_DOWN;
					turn_off_LED(i);
					ball_collision = 0x01;
					break;
				}
			}
		break;

		case B_DOWN:
			if (ball_row == FLOOR)
			{
				state = B_UP;
			}
 			// === DOWN PADDLE Collision Detection ===
			paddle_collision_detection(); 
		break;

		case B_UP_LEFT:
			if ((ball_column == LEFT_WALL) && (ball_row == CEILING))
			{
				state = B_DOWN_LEFT;
			}
			else if ((ball_column == LEFT_WALL) && (ball_row != CEILING))
			{
				state = B_UP_RIGHT;
			}
			else if ((ball_column != LEFT_WALL) && (ball_row == CEILING))
			{
				state = B_DOWN_LEFT;
			}
			else if ((ball_column != LEFT_WALL) && (ball_row != CEILING))
			{
				state = B_UP_LEFT;
			}
			else
			{
				state = B_DOWN;  // Debugging
			}

			// === UP-LEFT Brick Collision Detection ===
			for (int i = 2; i < ARRAY_SIZE; i++)
			{
				// Brick Directly Above
				//	Example of what should evaluate to TRUE
				//	DISPLAY_PORTB[h] -> XXXX X0XX  // X - don't care
				//	ball_column      -> 1111 1011  // 1 - LED is OFF
				//	                               // 0 - LED is ON
				//
				//	Example of what should evaluate to FALSE
				//	DISPLAY_PORTB[h] -> XXXX X1XX  // X - don't care
				//	ball_column      -> 1111 1011  // 1 - LED is OFF
				//
				if ((DISPLAY_PORTA[i] & (ball_row >> 1)) && ((DISPLAY_PORTB[i] & ~ball_column) == 0))
				{
					if (ball_column == LEFT_WALL)
					{
						state = B_DOWN_RIGHT;	
					}
					else
					{
						state = B_DOWN_LEFT;
					}
					// Turn hit LED off
					turn_off_LED(i);
					ball_collision = 0x01;
					SCORE++;
					break;
				}
			}
			
			// Brick Above Left, Not Directly Above
			if(ball_collision == 0x00)
			{
				for (int h = 2; h < ARRAY_SIZE; h++)
				{
					// Example of what should evaluate to TRUE
					// DISPLAY_PORTB[h] -> XXXX 0XXX  // X - don't care 
					// ball_column      -> 1111 1011  // 1 - LED is OFF
					//                                // 0 - LED is ON
					//
					// Example of what should evaluate to FALSE
					// DISPLAY_PORTB[h] -> XXXX 11XX  // X - don't care
					// ball_column      -> 1111 1011  // 1 - LED is OFF
					//                                // 0 - LED is ON

					// Edge Cases -- Out of bounds issue. If the ball is in the corner it should bounce back. 
					if ((ball_row == CEILING) && (ball_column == LEFT_WALL))
					{
						state = B_DOWN_RIGHT;
					}
					else if ((ball_row != CEILING) && (ball_column == LEFT_WALL))
					{
						state = B_UP_RIGHT;
					}
					// Expected Cases
					else if ((DISPLAY_PORTA[h] & (ball_row >> 1)) && ((DISPLAY_PORTB[h] & ((~ball_column)<<1)) == 0))
					{
						state = B_DOWN_RIGHT;
						turn_off_LED(h);
						break;
						SCORE++;
					}
				}
			}
		break;	

		case B_UP_RIGHT:
			if ((ball_column == RIGHT_WALL) && (ball_row == CEILING))
			{
				state = B_DOWN_LEFT;
			}
			else if ((ball_column == RIGHT_WALL) && (ball_row != CEILING))
			{
				state = B_UP_LEFT;
			}
			else if ((ball_column != RIGHT_WALL) && (ball_row == CEILING))
			{
				state = B_DOWN_RIGHT;
			}
			
			// === UP-RIGHT BRICK Collision Detection ===
			for (int i = 2; i < ARRAY_SIZE; i++)
			{
				// Brick Directly Above
				if ((DISPLAY_PORTA[i] & (ball_row >> 1)) && ((DISPLAY_PORTB[i] & ~ball_column) == 0))
				{
					if (ball_column == RIGHT_WALL)
					{
						state = B_DOWN_LEFT;
					}
					else
					{
						state = B_DOWN_RIGHT;
					}

					turn_off_LED(i);
					ball_collision = 0x01;
					SCORE++;
					break;
				}
			}
			// Brick Above to the Right, not directly above
			if (ball_collision == 0x00)
			{
				for (int k = 2; k < ARRAY_SIZE; k++)
				{
					
					// Example of what should evaluate to TRUE
					// DISPLAY_PORTB[k] -> XXXX X0XX  // X - don't care
					// ball_column      -> 1111 0111  // 1 - LED is OFF
					//                                // 0 - LED is ON
					//
					// Example of what should evaluate to FALSE
					// DISPLAY_PORTB[k] -> XXXX XX1X  // X - don't care
					// ball_column      -> 1111 1011  // 1 - LED is OFF
					//                                // 0 - LED is ON

					// Edge Cases
					if ((ball_row == CEILING) && (ball_column == RIGHT_WALL))
					{
						state = B_DOWN_LEFT;
					}
					else if ((ball_row != CEILING) && (ball_column == RIGHT_WALL))
					{
						state = B_UP_LEFT;
					}

					// Expected Cases
					//else if ((DISPLAY_PORTA[k] & (ball_row >> 1)) && ((((ball_column>>1) | 0x80) & DISPLAY_PORTB[k]) == DISPLAY_PORTB[k] ))
					else if ((DISPLAY_PORTA[k] & (ball_row >> 1)) && ((DISPLAY_PORTB[k] & ~((ball_column>>1)|0x80)) == 0x00))
					{
						state = B_DOWN_LEFT;
						turn_off_LED(k);
						SCORE++;
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


 			// === DOWN LEFT PADDLE Collision Detection ===
			paddle_collision_detection();

		break;

		case  B_DOWN_RIGHT:
			if ((ball_row == FLOOR) && (ball_column == RIGHT_WALL))
			{
				state = B_UP_LEFT;
			}
			else if ((ball_row == FLOOR) && (ball_column != RIGHT_WALL))
			{
				state = B_UP_RIGHT;
			}
			else if ((ball_row != FLOOR) && (ball_column == RIGHT_WALL))
			{
				state = B_DOWN_LEFT;
			}
		
 			// === DOWN RIGHT PADDLE Collision Detection ===
			paddle_collision_detection();
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

		case B_UP:
			ball_row = ball_row >> 1; // bottom row
		break;

		case B_DOWN:
			ball_row = ball_row<<1; // bottom row
		break;
		
		case B_UP_LEFT:
			ball_row = ball_row >> 1;                 // Ex. 0100 0000 -> 0010 0000
			ball_column = (ball_column << 1) | 0x01;  // Ex. 1111 1011 -> 1111 0111
		break;

		case B_DOWN_LEFT:
			ball_row = ball_row << 1;
			ball_column = (ball_column << 1) | 0x01;
		break;

		case  B_UP_RIGHT:
			ball_row = ball_row >> 1;                // Ex. 0100 0000 -> 0010 0000
			ball_column = (ball_column >> 1) | 0x80; // Ex. 1111 1011 -> 1111 1101
		break;

		case B_DOWN_RIGHT:
			ball_row = ball_row << 1;
			ball_column = (ball_column >> 1) | 0x80;
		break;
		
		default:
		break;
	}

	DISPLAY_PORTA[1] =  ball_row;    // PORTA displays column pattern
	DISPLAY_PORTB[1] =  ball_column; // PORTB selects column to display pattern
	
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

	return state;
	
};


// ====================
// LED_MATRIX_OUTPUT_TICK:OUTPUT TO LED matrix
// ====================
enum O_States {O_START, O_INIT, O_DISPLAY_OUTPUT};
int LED_MATRIX_OUTPUT_Tick(int state) {

	// === Local Variables ===
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

// ====================
// LCD_OUTPUT_TICK:OUTPUT TO LED matrix
// ====================
enum LCD_O_States {LCD_O_START, LCD_O_INIT, LCD_O_DISPLAY_MENU, LCD_O_POST_DISPLAY_MENU, LCD_O_DISPLAY_SCORE, LCD_O_DISPLAY_GAME_OVER, LCD_O_WAIT};
int LCD_OUTPUT_Tick(int state) {

	// === Local Variables ===
	unsigned static PREV_SCORE = 0x00;
	unsigned static char SCORE_HUNDREDS_PLACE = 0x00;
	unsigned static char SCORE_TENS_PLACE = 0x00;
	unsigned static char SCORE_ONES_PLACE = 0x00;
	unsigned static char count = 0x01;
	// === Transitions ===
	switch (state) {
		case LCD_O_START:
			state = LCD_O_INIT;
		break;
		
		case LCD_O_INIT:
			state = LCD_O_DISPLAY_MENU;
		break;

		case  LCD_O_DISPLAY_MENU:
			if (gameInPlay == 0x01)
			{
				state = LCD_O_DISPLAY_SCORE;
			}
			else
			{
				state = LCD_O_POST_DISPLAY_MENU;
			}
		break;

		case LCD_O_POST_DISPLAY_MENU:
			if (gameInPlay != 0x01)
			{
				state = LCD_O_POST_DISPLAY_MENU;
			}
			else
			{
				state = LCD_O_DISPLAY_SCORE;
			}
		break;

		case LCD_O_DISPLAY_SCORE:
			if ((gameInPlay == 0x00) && (gameOver==0x00))
			{	
				state = LCD_O_DISPLAY_MENU;
			}
			else if ((gameInPlay == 0x01) && (gameOver==0x00))
			{
				state = LCD_O_DISPLAY_SCORE;
			}
			else if ((gameInPlay == 0x01) && (gameOver==0x01))
			{
				state = LCD_O_DISPLAY_GAME_OVER;
			}
		break;
		
		default:
			state = LCD_O_INIT;
		break;
	}
	
	// === Actions ===
	switch (state)
	{
		case LCD_O_START:
		break;
		
		case LCD_O_INIT:
			LCD_ClearScreen();
		break;

		case LCD_O_DISPLAY_MENU:
			LCD_Cursor(0x01);
			LCD_DisplayString(1, "BRICK BREAKER!  PRESS START");
			//LCD_DisplayString(0x10, "START TO PLAY");
		break;

		case LCD_O_DISPLAY_SCORE:
			// Express SCORE by factors of 10
			if (SCORE != PREV_SCORE)
			{
					count++;
					if (SCORE_TENS_PLACE == 0x0A)
					{
						SCORE_HUNDREDS_PLACE++;
						SCORE_TENS_PLACE = 0;
					}
					if (SCORE_ONES_PLACE == 0x0A)
					{
						SCORE_TENS_PLACE++;
						SCORE_ONES_PLACE = 0;
					}
					SCORE_ONES_PLACE++;
				LCD_Cursor(0x01);
				LCD_DisplayString(1, "SCORE:");
				LCD_WriteData('0' + SCORE_HUNDREDS_PLACE);
				LCD_WriteData('0' + SCORE_TENS_PLACE);
				LCD_WriteData('0' + SCORE_ONES_PLACE);
				LCD_WriteData('0'); // Make the score a little bigger =P
				LCD_Cursor(0x30);

				SCORE = PREV_SCORE;
			}	
		break;

		case LCD_O_DISPLAY_GAME_OVER:
			LCD_Cursor(0x01);
			LCD_DisplayString(1, "GAME OVER!      ");
			//LCD_DisplayString(0x10, "START TO PLAY");
		break;
		
		default:
		break;
	}
	return state;
};

// ====================
// GAME_TICK:OUTPUT TO LED matrix
// ====================
enum G_States {G_START, G_INIT, G_MENU, G_PRE_PLAY_WAIT, G_PLAY, G_POST_PLAY_WAIT, G_PAUSE, G_POST_PAUSE_WAIT, G_SET, G_RESET, G_PRE_RESET, G_GAME_OVER};
int GAME_Tick(int state) {

	// === Local Variables ===
	unsigned char button = GetNESControllerButton();
	unsigned static char NumberOfTurns = 0x03;
	unsigned static char game_over_count = 0x00; 
	unsigned char game_over_display_time = 0x05;

	// === Transitions ===
	switch (state) {
		case G_START:
		state = G_INIT;
		break;
		
		case G_INIT:
			state = G_MENU;
		break;

		case G_MENU:
			if ((button == NES_START ) && ~gameInPlay)
			{
				gameInPlay = 0x01;
				state = G_PRE_PLAY_WAIT;
			}
		break;
		
		case G_PRE_PLAY_WAIT:
			if (button == NES_START)
			{
				state = G_PRE_PLAY_WAIT;
			}
			else
			{
				state = G_PLAY;
			}
		break;

		case G_PLAY:
			state = G_POST_PLAY_WAIT;
		break;

		case G_POST_PLAY_WAIT:
			if ((button == NES_START) && (gameEndTurn != 0x01))
			{
				gamePause = 0x01;
				state = G_PAUSE;
			}
			else if (gameEndTurn == 0x01)
			{
				gameEndTurn = 0x00;
				NumberOfTurns--;
				if (NumberOfTurns != 0x00)
				{
					state = G_SET;
				}
				else
				{
					state = G_GAME_OVER;
				}
			}
			else
			{
				 state = G_POST_PLAY_WAIT;
			}
		break;

		case G_GAME_OVER:
			if (game_over_count >= game_over_display_time)
			{
				state = G_MENU;
				gameOver = 0x00;
			}
			else 
			{
				state = G_GAME_OVER;
			}
		break;
		
		case G_PAUSE:
			if(button == NES_START)
			{
				state = G_PAUSE;
			}
			else
			{
				state = G_POST_PAUSE_WAIT;
			}
		break;

		case G_POST_PAUSE_WAIT:
			if (button == NES_START)
			{
				gamePause = 0x00;
				state = G_PRE_PLAY_WAIT;
			}
		break;

		case G_SET:
			if (button == NES_START)
			{
				state = G_PRE_PLAY_WAIT;
				gamePlay = 0x01;
				gameSet = 0x00;
				gameEndTurn = 0x00;
			}
		break;

		case G_PRE_RESET:
			if (button == (NES_START|NES_SELECT))
			{
				state = G_PRE_RESET;
			}
			else
			{
				state = G_RESET;
			}
		break;

		case G_RESET:
			if (button == (NES_START|NES_SELECT))
			{
				state = G_PRE_RESET;
			}
			else
			{
				state = G_MENU;
				gamePlay = 0;
			}
		break;
		
		default:
			state = G_INIT;
		break;
	}
	
	// === Actions ===
	switch (state) {
		case G_START:
		break;
		
		case G_INIT:
			gameInPlay = 0x00;
			gamePause = 0x00;
			gameSet = 0x00;
			gamePlay = 0x00;
			gameEndTurn = 0x00;
		break;

		case G_MENU:
		break;
		
		case G_PRE_PLAY_WAIT:
		break;

		case G_PLAY:
		break;

		case G_POST_PLAY_WAIT:
		break;
		
		case G_GAME_OVER:
			gameOver = 0x01;
		break;

		case G_PAUSE:
		break;

		case G_SET:
		break;

		case G_PRE_RESET:
		break;

		case G_RESET:
		break;
		
		default:
		break;
	}
	return state;
};


// == TimerISR() ==
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

	DDRA = 0xFF; PORTA = 0x00; // Initialize to output to PORT A
	DDRB = 0xFF; PORTB = 0x00;
	//DDRC = 0x03; PORTC = 0x04; // Before LCD implementation
	DDRC = 0xFB; PORTC = 0x04; // LCD control lines on Pin 4 and Pin 5. NES Outputs on Pin 1 and Pin 2. NES Read on Pin 3
	DDRD = 0xFF; PORTD = 0x00; // LCD data lines

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
	tasks[i].TickFct= &LED_MATRIX_OUTPUT_Tick;
	++i;
	tasks[i].state = LCD_O_START;
	tasks[i].period = periodLCDOutput;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &LCD_OUTPUT_Tick;
	++i;
	tasks[i].state = G_START;
	tasks[i].period = periodGame;
	tasks[i].elapsedTime = tasks[i].period;
	tasks[i].TickFct= &GAME_Tick;

	

 	TimerSet(tasksPeriodGCD);
 	TimerOn();
	
	   // Initializes the LCD display
	   	LCD_init();
	   	LCD_Cursor(0x01);
		//LCD_DisplayString(1, "Systems Online.");
		LCD_DisplayString(1, "LOADING...");

	while(1)
	{
		while (!TimerFlag);
		TimerFlag = 0;			
	}
}