#include <avr/io.h>
#include "timer.h"
#include "NES_Controller.h"

// ====================
//PADDLE_TICK: PADDLE MOVEMENT ON LED matrix
// ====================
enum P_States {P_START, P_INIT, P_WAIT, P_MOVE_LEFT, P_MOVE_RIGHT, P_BUTTON_HOLD} P_State;

int Paddle_Tick(int P_State, unsigned char controller_data) {
	
	// === Local Variables ===
	static unsigned char column_val = 0x80; // sets the pattern displayed on columns
	static unsigned char column_sel = 0x1F; // grounds column to display pattern
	
	// === Transitions ===
	switch (P_State) {
		case P_START:
			P_State = P_INIT;
		break;

		case P_INIT:
			P_State = P_WAIT;
		break;

		case P_WAIT:
			
			if ()
			{
				P_State = P_INIT;
			}
			
		break;
		
		default:
		state = sm1_display;
		break;
	}
	
	// === Actions ===
	switch (state) {
		case sm1_display:   // If illuminated LED in bottom right corner
		if (column_sel == 0xFE && column_val == 0x80)
		{
			column_sel = 0x7F; // display far left column
			column_val = 0x01; // pattern illuminates top row
		}
		// else if far right column was last to display (grounded)
		else if (column_sel == 0xFE)
		{
			column_sel = 0x7F; // resets display column to far left column
			column_val = column_val << 1; // shift down illuminated LED one row
		}
		// else Shift displayed column one to the right
		else
		{
			column_sel = (column_sel >> 1) | 0x80;
		}
		break;
		
		default:
		break;
	}
	
	PORTA = column_val; // PORTA displays column pattern
	PORTB = column_sel; // PORTB selects column to display pattern
	
	return state;
};

int main(void) {

	DDRB = 0xFF; PORTB = 0x00;
	DDRA = 0xFF; PORTA = 0x00; // Initialize to output to PORT A

	
	TimerSet(100);
	TimerOn();
	
	int c_state = 0;
	while(1) {

		PORTA = 0x80; // display far left column
		PORTB = 0x1F; // pattern illuminates top row
		
		//c_state = SM1_Tick(c_state);
		while (!TimerFlag);
		TimerFlag = 0;
		
		
	}
}