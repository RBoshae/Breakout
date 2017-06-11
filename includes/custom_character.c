#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "io.h"

void write_score(unsigned char score)
{

}

void draw_ball()
{
	
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