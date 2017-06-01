#include <avr/io.h>
#include "timer.h"
#include "NES_Controller.h"

// ====================
// SM1: DEMO LED matrix
// ====================
enum SM1_States {sm1_display};

int SM1_Tick(int state) {
	
	// === Local Variables ===
	static unsigned char column_val = 0x01; // sets the pattern displayed on columns
	static unsigned char column_sel = 0x7F; // grounds column to display pattern
	
	// === Transitions ===
	switch (state) {       
			case sm1_display:    
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
	DDRD = 0xFF; PORTD = 0x00; // Initialize to output to PORT A
	DDRC = 0x03; PORTC = 0x04;

	unsigned char button_info = 0x00;
	TimerSet(200);
	TimerOn();
	
	int c_state = 0;
	while(1) {
		
		button_info = GetNESControllerButton();
		
		//c_state = SM1_Tick(c_state);
		while (!TimerFlag);
			TimerFlag = 0;

		PORTA = 0x01; // display far left column
		PORTB = 0x7F << button_info; // pattern illuminates top row
		PORTD = button_info;
	
	}
}