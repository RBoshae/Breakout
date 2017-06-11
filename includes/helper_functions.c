#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "io.h"

void write_score(unsigned char SCORE)
{
	unsigned char SCORE_HUNDREDS_PLACE = 0x00;
	unsigned char SCORE_TENS_PLACE = 0x00;
	unsigned char SCORE_ONES_PLACE = 0x00;

	for (unsigned char index = 0;  index < SCORE; index++ )
	{
		SCORE_ONES_PLACE++;

		if (SCORE_TENS_PLACE >= 0x0A)
		{
			SCORE_HUNDREDS_PLACE++;
			SCORE_TENS_PLACE = 0;
			SCORE_ONES_PLACE = 0;
		}
		if (SCORE_ONES_PLACE >= 0x0A)
		{
			SCORE_TENS_PLACE++;
			SCORE_ONES_PLACE = 0;
		}
		
		
	}

	LCD_WriteData('0' + SCORE_HUNDREDS_PLACE);
	LCD_WriteData('0' + SCORE_TENS_PLACE);
	LCD_WriteData('0' + SCORE_ONES_PLACE);
	LCD_WriteData('0'); // Make the score a little bigger =P

}

void draw_ball(unsigned char col)
{
	LCD_Cursor(col);
	LCD_WriteData(0x00);
	LCD_WriteCommand(0x40);
	LCD_WriteData(0x00);
	LCD_WriteData(0x0E);
	LCD_WriteData(0x18);
	LCD_WriteData(0x11);
	LCD_WriteData(0x18);
	LCD_WriteData(0x0E);
	LCD_WriteData(0x00);
	LCD_WriteData(0x00);
	LCD_WriteCommand(0x80);

}
#endif