// Copyright (c) 2015-19, Joe Krachey
// All rights reserved.
//
// Redistribution and use in source or binary form, with or without modification, 
// are permitted provided that the following conditions are met:
//
// 1. Redistributions in source form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in 
//    the documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// By:
// Kunal Waghray
// Christian Henke

#include "main.h"
// for randomization
#include <stdlib.h>

// for EEPROM	
#define addrGamemode 			256+0
#define addrPlayer1Score 	256+4
#define addrPlayer2Score 	256+8
#define addrRandomSeed 		256+12

// joystick variables
volatile uint32_t direction;
bool movingJoystick;

// global variables
bool menu;
uint8_t gamemode;
uint8_t player1score;
uint8_t player2score;
uint8_t randomSeed;
volatile int ballWait;
volatile int waitLoser;
volatile bool heisenbergBlinking;
volatile bool AlertTimerDelay;
volatile bool AlertGameTick;
volatile bool ioButtonLeft = false;
volatile bool ioButtonRight = false;
// sw1 button debounce
volatile bool AlertTimer4;


// this is mostly for info, hence const
const int GAMEMODES = 5;
const int maxSpeed = 10;
const int ballMaxSpeed = 5;
const int gameSpeed = 20;
const int winScore = 10;
const int maxScoreWidth = 30;
const int maxScoreHeight = 20;
const int heisenbergBlinkTime = 30; // in ms
const int accelSensitivity = 7000;
const int noupdown = 10;

// x and y of the center of the object
int player1x;
int player1y;
int player2x;
int player2y;
int ballx;
int bally;
int ballChangex; // speed
int ballChangey; // speed

// used for many calculations, initalized in main using images.c values
int playerWidth;
int playerHeight; 
int ballWidth; 
int ballHeight;


// Debounce the SW1 button using the timer4 ISR
bool sw1_debounce(void)
{
  // this function is constantly called
	bool pin_logic_level;
	static int debounceCount = 0;
	
	// every 20ms
	if (AlertTimer4) {
		// read the SW1 button
		pin_logic_level = lp_io_read_pin(SW1_BIT);
		// if unpressed
		if (pin_logic_level) {
				debounceCount = 0; // reset
		}
		else { // active low
				// count to debounce
				if (debounceCount<10)
					debounceCount++;
				
		}
		// acknowledge interrupt
		AlertTimer4 = false;
		// guarantee 1 debounced button signal
		return debounceCount==7;
	}
	else 
		return false;
}

// this function called constantly
void delayWaitFunction(void) 
{
	// local variables with only one initialization
	static int i = 0;
	static int j = 0;
	static bool ballWaiting = false;
	static bool waitingLoser = false;
	static int blinking = 0;
	
	// every 10ms
	if (AlertTimerDelay){
			// delay after a point
			if (ballWait && !ballWaiting){
					i = 0;
					ballWaiting = true;
			}
			if (ballWaiting) {
					i++;
					// a second has been waited
					if (i>100){
						ballWait = 0;
						i = 0;
						ballWaiting = false;
					}
			}
			
			// delay after game won 
			if (waitLoser && !waitingLoser) {
				j = 0;
				waitingLoser = true;
			}
			if (waitingLoser){
				j++;
				if (j>100) {
					j = 0;
					waitLoser = 0;
					waitingLoser = false;
				}
			}
			// every 0.5 seconds switch
			blinking++;
			if (blinking>2*heisenbergBlinkTime) blinking = 0;
			heisenbergBlinking = blinking<heisenbergBlinkTime;
			
			// acknowledge interrupt
			AlertTimerDelay = false;
	}
}

// this function sets the game speed
bool gameTick(void) 
{
	if (AlertGameTick){
			// acknowledge interrupt
			AlertGameTick = false;
			return true;
	}
	return false;
}


// based on the global variables, draw the menu
void drawMenu(void)
{
	// hide old menu choice
	lcd_clear_screen(LCD_COLOR_BLACK);
	
	// draw menu choices:
	switch(gamemode) {
		case 0: 
			// middle (current option)
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode0WidthPixels,   // Image Horizontal Width
											(ROWS/2),                 // Y Pos
											mode0HeightPixels,  // Image Vertical Height
											mode0Bitmaps,       // Image
											LCD_COLOR_GREEN,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// above option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode5WidthPixels,   // Image Horizontal Width
											((ROWS/2)/2),                 // Y Pos
											mode5HeightPixels,  // Image Vertical Height
											mode5Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// below option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode1WidthPixels,   // Image Horizontal Width
											(3*(ROWS/2)/2),                 // Y Pos
											mode1HeightPixels,  // Image Vertical Height
											mode1Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			break;
		case 1: 
			// middle (current option)
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode1WidthPixels,   // Image Horizontal Width
											(ROWS/2),                 // Y Pos
											mode1HeightPixels,  // Image Vertical Height
											mode1Bitmaps,       // Image
											LCD_COLOR_GREEN,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// above option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode0WidthPixels,   // Image Horizontal Width
											((ROWS/2)/2),                 // Y Pos
											mode0HeightPixels,  // Image Vertical Height
											mode0Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// below option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode2WidthPixels,   // Image Horizontal Width
											(3*(ROWS/2)/2),                 // Y Pos
											mode2HeightPixels,  // Image Vertical Height
											mode2Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			break;
		case 2: 
			// middle (current option)
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode2WidthPixels,   // Image Horizontal Width
											(ROWS/2),                 // Y Pos
											mode2HeightPixels,  // Image Vertical Height
											mode2Bitmaps,       // Image
											LCD_COLOR_GREEN,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// above option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode1WidthPixels,   // Image Horizontal Width
											((ROWS/2)/2),                 // Y Pos
											mode1HeightPixels,  // Image Vertical Height
											mode1Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// below option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode3WidthPixels,   // Image Horizontal Width
											(3*(ROWS/2)/2),                 // Y Pos
											mode3HeightPixels,  // Image Vertical Height
											mode3Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			break;
		case 3: 
			// middle (current option)
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode3WidthPixels,   // Image Horizontal Width
											(ROWS/2),                 // Y Pos
											mode3HeightPixels,  // Image Vertical Height
											mode3Bitmaps,       // Image
											LCD_COLOR_GREEN,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// above option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode2WidthPixels,   // Image Horizontal Width
											((ROWS/2)/2),                 // Y Pos
											mode2HeightPixels,  // Image Vertical Height
											mode2Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// below option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode4WidthPixels,   // Image Horizontal Width
											(3*(ROWS/2)/2),                 // Y Pos
											mode4HeightPixels,  // Image Vertical Height
											mode4Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			break;
		case 4: 
			// middle (current option)
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode4WidthPixels,   // Image Horizontal Width
											(ROWS/2),                 // Y Pos
											mode4HeightPixels,  // Image Vertical Height
											mode4Bitmaps,       // Image
											LCD_COLOR_GREEN,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// above option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode3WidthPixels,   // Image Horizontal Width
											((ROWS/2)/2),                 // Y Pos
											mode3HeightPixels,  // Image Vertical Height
											mode3Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// below option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode5WidthPixels,   // Image Horizontal Width
											(3*(ROWS/2)/2),                 // Y Pos
											mode5HeightPixels,  // Image Vertical Height
											mode5Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			break;
		case 5: 
			// middle (current option)
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode5WidthPixels,   // Image Horizontal Width
											(ROWS/2),                 // Y Pos
											mode5HeightPixels,  // Image Vertical Height
											mode5Bitmaps,       // Image
											LCD_COLOR_GREEN,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// above option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode4WidthPixels,   // Image Horizontal Width
											((ROWS/2)/2),                 // Y Pos
											mode4HeightPixels,  // Image Vertical Height
											mode4Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			// below option
			lcd_draw_image(
											(COLS/2),                 // X Pos
											mode0WidthPixels,   // Image Horizontal Width
											(3*(ROWS/2)/2),                 // Y Pos
											mode0HeightPixels,  // Image Vertical Height
											mode0Bitmaps,       // Image
											LCD_COLOR_GREEN2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
			break;
		default:
			// error
			lcd_clear_screen(LCD_COLOR_GREEN2);
			break;
	}

		/* 
		Drawing notes:
		Top left pixel is 0,0
		Screen width is COLS
		Screen height is ROWS
		images are drawn from the CENTER!!!
		draw_rectangle_centered draws rectangles from the center
		*/
}
// draw the scores on the side
void drawScore(void){
	// max score we can draw
	if (player1score<10 && player2score<10) {
		// hide old score
		lcd_draw_image(				(COLS-5)-maxScoreWidth/2,                 // X Pos
													maxScoreWidth,   // Image Horizontal Width
													(ROWS/2) - 1 - maxScoreHeight/2,                 // Y Pos
													maxScoreHeight,  // Image Vertical Height
													num0Bitmaps,       // Image
													LCD_COLOR_BLACK,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
		lcd_draw_image(				(COLS-5)-maxScoreWidth/2,                 // X Pos
													maxScoreWidth,   // Image Horizontal Width
													(ROWS/2) + 1 + maxScoreHeight/2,                 // Y Pos
													maxScoreHeight,  // Image Vertical Height
													num0Bitmaps,       // Image
													LCD_COLOR_BLACK,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
												
		switch (player1score) {
			case 0:
				lcd_draw_image(		(COLS-5)-num0Width/2,                 // X Pos
													num0Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num0Height/2,                 // Y Pos
													num0Height,  // Image Vertical Height
													num0Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 1:
				lcd_draw_image(		(COLS-5)-num1Width/2,                 // X Pos
													num1Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num1Height/2,                 // Y Pos
													num1Height,  // Image Vertical Height
													num1Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 2:
				lcd_draw_image(		(COLS-5)-num2Width/2,                 // X Pos
													num2Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num2Height/2,                 // Y Pos
													num2Height,  // Image Vertical Height
													num2Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 3:
				lcd_draw_image(		(COLS-5)-num3Width/2,                 // X Pos
													num1Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num3Height/2,                 // Y Pos
													num3Height,  // Image Vertical Height
													num3Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 4:
				lcd_draw_image(		(COLS-5)-num4Width/2,                 // X Pos
													num4Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num4Height/2,                 // Y Pos
													num4Height,  // Image Vertical Height
													num4Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 5:
				lcd_draw_image(		(COLS-5)-num5Width/2,                 // X Pos
													num5Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num5Height/2,                 // Y Pos
													num5Height,  // Image Vertical Height
													num5Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 6:
				lcd_draw_image(		(COLS-5)-num6Width/2,                 // X Pos
													num6Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num6Height/2,                 // Y Pos
													num6Height,  // Image Vertical Height
													num6Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 7:
				lcd_draw_image(		(COLS-5)-num7Width/2,                 // X Pos
													num7Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num7Height/2,                 // Y Pos
													num7Height,  // Image Vertical Height
													num7Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 8:
				lcd_draw_image(		(COLS-5)-num8Width/2,                 // X Pos
													num8Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num8Height/2,                 // Y Pos
													num8Height,  // Image Vertical Height
													num8Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 9:
				lcd_draw_image(		(COLS-5)-num9Width/2,                 // X Pos
													num9Width,   // Image Horizontal Width
													(ROWS/2) - 1 - num9Height/2,                 // Y Pos
													num9Height,  // Image Vertical Height
													num9Bitmaps,       // Image
													LCD_COLOR_CYAN,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			default: //error
				lcd_clear_screen(LCD_COLOR_GREEN2);
				break;
		}						
		switch (player2score) {
			case 0:
				lcd_draw_image(		(COLS-5)-num0Width/2,                 // X Pos
													num0Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num0Height,  // Image Vertical Height
													num0Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 1:
				lcd_draw_image(		(COLS-5)-num1Width/2,                 // X Pos
													num1Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num1Height,  // Image Vertical Height
													num1Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 2:
				lcd_draw_image(		(COLS-5)-num2Width/2,                 // X Pos
													num2Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num2Height,  // Image Vertical Height
													num2Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 3:
				lcd_draw_image(		(COLS-5)-num3Width/2,                 // X Pos
													num1Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num3Height,  // Image Vertical Height
													num3Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 4:
				lcd_draw_image(		(COLS-5)-num4Width/2,                 // X Pos
													num4Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num4Height,  // Image Vertical Height
													num4Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 5:
				lcd_draw_image(		(COLS-5)-num5Width/2,                 // X Pos
													num5Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num5Height,  // Image Vertical Height
													num5Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 6:
				lcd_draw_image(		(COLS-5)-num6Width/2,                 // X Pos
													num6Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num6Height,  // Image Vertical Height
													num6Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 7:
				lcd_draw_image(		(COLS-5)-num7Width/2,                 // X Pos
													num7Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num7Height,  // Image Vertical Height
													num7Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 8:
				lcd_draw_image(		(COLS-5)-num8Width/2,                 // X Pos
													num8Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num8Height,  // Image Vertical Height
													num8Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			case 9:
				lcd_draw_image(		(COLS-5)-num9Width/2,                 // X Pos
													num9Width,   // Image Horizontal Width
													(ROWS/2) + 1 + num0Height/2,                 // Y Pos
													num9Height,  // Image Vertical Height
													num9Bitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
				break;
			default: //error
				lcd_clear_screen(LCD_COLOR_GREEN2);
				break;
		}
	}
}

// draw fireworks on game won screen
void drawFireworks(void) {
			int color1 = rand() % 12; // 0 to 11
			int color2 = rand() % 12;
			switch (color1) {
				case 0:
					color1 = LCD_COLOR_WHITE;
					break;
				case 1:
					color1 = LCD_COLOR_RED;
					break;
				case 2:
					color1 = LCD_COLOR_GREEN;
					break;
				case 3:
					color1 = LCD_COLOR_GREEN2;
					break;
				case 4:
					color1 = LCD_COLOR_BLUE;
					break;
				case 5:
					color1 = LCD_COLOR_BLUE2;
					break;
				case 6:
					color1 = LCD_COLOR_YELLOW;
					break;
				case 7:
					color1 = LCD_COLOR_ORANGE;
					break;
				case 8:
					color1 = LCD_COLOR_CYAN;
					break;
				case 9:
					color1 = LCD_COLOR_MAGENTA;
					break;
				case 10:
					color1 = LCD_COLOR_GRAY;
					break;
				case 11:
					color1 = LCD_COLOR_BROWN;
					break;
			}
			switch (color2) {
				case 0:
					color2 = LCD_COLOR_WHITE;
					break;
				case 1:
					color2 = LCD_COLOR_RED;
					break;
				case 2:
					color2 = LCD_COLOR_GREEN;
					break;
				case 3:
					color2 = LCD_COLOR_GREEN2;
					break;
				case 4:
					color2 = LCD_COLOR_BLUE;
					break;
				case 5:
					color2 = LCD_COLOR_BLUE2;
					break;
				case 6:
					color2 = LCD_COLOR_YELLOW;
					break;
				case 7:
					color2 = LCD_COLOR_ORANGE;
					break;
				case 8:
					color2 = LCD_COLOR_CYAN;
					break;
				case 9:
					color2 = LCD_COLOR_MAGENTA;
					break;
				case 10:
					color2 = LCD_COLOR_GRAY;
					break;
				case 11:
					color2 = LCD_COLOR_BROWN;
					break;
			}
			lcd_draw_image(
											(COLS/2),                 // X Pos
											fireworksWidth,   // Image Horizontal Width
											((ROWS/2)/2),                 // Y Pos
											fireworksHeight,  // Image Vertical Height
											fireworksBitmaps,       // Image
											color1,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
					
			lcd_draw_image(
											(COLS/2),                 // X Pos
											fireworksWidth,   // Image Horizontal Width
											(3*(ROWS/2)/2),                 // Y Pos
											fireworksHeight,  // Image Vertical Height
											fireworksBitmaps,       // Image
											color2,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
}
//*****************************************************************************
void DisableInterrupts(void)
{
  __asm {
         CPSID  I
  }
}

//*****************************************************************************
void EnableInterrupts(void)
{
  __asm {
    CPSIE  I
  }
}



// returns true if the ball is touching a player
bool playercontact(void){
	
		bool isContacting = false;
		
		// hit detection
		if (player2x - playerWidth/2 < ballx + ballWidth/2 &&
				player2x + playerWidth/2 > ballx - ballWidth/2 &&
				player2y - playerHeight/2 < bally + ballHeight/2 &&
				player2y + playerHeight/2 > bally - ballHeight/2) {
			isContacting = true;
		}
		if (player1x - playerWidth/2 < ballx + ballWidth/2 &&
				player1x + playerWidth/2 > ballx - ballWidth/2 &&
				player1y - playerHeight/2 < bally + ballHeight/2 &&
				player1y + playerHeight/2 > bally - ballHeight/2) {
			isContacting = true;
		}
		// this logic will result in if the ball is touching both players 
		// (due to some crazy gamemodes)
		// that the ball twitches back and forth as long as it's touching one player
		return isContacting;
}

// returns true if the ball is touching a wall
bool wallcontact(void){
		bool isContacting = false;
		// half of the ball is past the left or right wall 
		// (because of the motion this ball is about to take)
		if (ballx < 0 + ballWidth)
			isContacting = true;
		if (ballx > COLS - ballWidth)
			isContacting = true;
		
		return isContacting;
}


// randomizes the direction the ball will travel in
void randomizeBall(void) {
	ballChangex = (rand() % (ballMaxSpeed*2+1)) - ballMaxSpeed; // from 0 to (ballMaxSpeed*2), then from -ballMaxSpeed to ballMaxSpeed
	ballChangey = (rand() % (ballMaxSpeed*2+1)) - ballMaxSpeed;
	// prevent infinite loop left and right (but not up and down cause that'd be funny)
	while (ballChangey==0){
		ballChangex = (rand() % (ballMaxSpeed*2+1)) - ballMaxSpeed; 
		ballChangey = (rand() % (ballMaxSpeed*2+1)) - ballMaxSpeed;
	}
}

//*****************************************************************************
//************DEATH**********************PONG**********************************
//*****************************************************************************

// run the game: DEATH PONG
int 
main(void)
{
	bool done = false;
	int accelX = 0;
	
	// ideals: player 40 width, 10 height. ball 10 width, 10 height.
	playerWidth = playerWidthPixels; // get these values from bitmap
	playerHeight = playerHeightPixels; // get these values from bitmap
	ballWidth = ballWidthPixels; // get these values from bitmap
	ballHeight = ballHeightPixels; // get these values from bitmap
	
	gamemode = 0;
	player1score = 0;
	player2score = 0;
	randomSeed = 0;
	// initialize hardware
	project_initialize_hardware();

	// EEPROM example
	//eeprom_byte_write(I2C1_BASE,addrGamemode, gamemode);
	//eeprom_byte_read(I2C1_BASE,addrGamemode, &gamemode);
	// read EEPROM for gamemode currently
	// read EEPROM for player scores
	// read EEPROM for random seed
	eeprom_byte_read(I2C1_BASE, addrGamemode, &gamemode);
	eeprom_byte_read(I2C1_BASE, addrPlayer1Score, &player1score);
	eeprom_byte_read(I2C1_BASE, addrPlayer2Score, &player2score);
	eeprom_byte_read(I2C1_BASE, addrRandomSeed, &randomSeed);
	
	// initialize menu defaults
	menu = 1;
	movingJoystick = 0;
	ballWait = 0;
	waitLoser = 0;
	
	// initialize random number generator 
  srand(randomSeed);   // Initialization, should only be called once.
	randomSeed++; //loops around, only uint8
	// write random seed to EEPROM
	eeprom_byte_write(I2C1_BASE,addrRandomSeed, randomSeed);
	
	// initialize screen
	lcd_config_screen();
	lcd_clear_screen(LCD_COLOR_BLACK);
	drawMenu();
	
	while (!done) {
		// pre-game menu selection
		if (menu) {
			// joystick move
			if (direction!=0 && movingJoystick==0){
					// not allowed to move until moving stops
					movingJoystick = 1;
					// move to new choice (wrap around motion)
					if (direction==1) { // up  
							gamemode--;
							if (gamemode==0xFF) gamemode = GAMEMODES; //0xFF as -1
					}
					else if (direction==3) { // down
							gamemode++;
							if (gamemode==GAMEMODES+1) gamemode = 0; 
					}
					// write currently selected gamemode to EEPROM
					eeprom_byte_write(I2C1_BASE,addrGamemode, gamemode);
					
					// redraw the menu 
					lcd_clear_screen(LCD_COLOR_BLACK);
					drawMenu();
			}
			// joystick is back at center
			else if (direction==0) {
				// can move it again
					movingJoystick = 0;
			}
			// select-menu-option button pressed
			if (sw1_debounce()) {
					// select menu option by turning off menu
					// and going into current gamemode
					menu = 0;
					// initialize variables based on gamemode
				  switch (gamemode) {
					// regular pong
					case 0:
						player1x = (COLS/2);
						player1y = (ROWS/6);
						player2x = (COLS/2);
						player2y = (5*ROWS/6);
						ballx = (COLS/2);
						bally = (ROWS/2);
						// randomize ball
						randomizeBall();
						
						lcd_clear_screen(LCD_COLOR_BLACK);
						// draw players
						lcd_draw_image(
													player1x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player1y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						lcd_draw_image(
													player2x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player2y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw ball
						lcd_draw_image(
													ballx,                 // X Pos
													ballWidth,   // Image Horizontal Width
													bally,                 // Y Pos
													ballHeight,  // Image Vertical Height
													ballBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw score
						drawScore();
						break;
					// gravity pong
					case 1:
						player1x = (COLS/2);
						player1y = (ROWS/6);
						player2x = (COLS/2);
						player2y = (5*ROWS/6);
						ballx = (COLS/2);
						bally = (ROWS/2);
						// randomize ball
						randomizeBall();
						
						lcd_clear_screen(LCD_COLOR_BLACK);
						// draw players
						lcd_draw_image(
													player1x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player1y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						lcd_draw_image(
													player2x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player2y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw ball
						lcd_draw_image(
													ballx,                 // X Pos
													ballWidth,   // Image Horizontal Width
													bally,                 // Y Pos
													ballHeight,  // Image Vertical Height
													ballBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw score
						drawScore();
						break;
					// heisenberg pong
					case 2:
						player1x = (COLS/2);
						player1y = (ROWS/6);
						player2x = (COLS/2);
						player2y = (5*ROWS/6);
						ballx = (COLS/2);
						bally = (ROWS/2);
						// randomize ball
						randomizeBall();
						
						lcd_clear_screen(LCD_COLOR_BLACK);
						// draw players
						lcd_draw_image(
													player1x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player1y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						lcd_draw_image(
													player2x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player2y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw ball
						lcd_draw_image(
													ballx,                 // X Pos
													ballWidth,   // Image Horizontal Width
													bally,                 // Y Pos
													ballHeight,  // Image Vertical Height
													ballBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw score
						drawScore();
						break;
					// weird controls pong
					case 3:
						player1x = (COLS/2);
						player1y = (ROWS/6);
						player2x = (COLS/2);
						player2y = (5*ROWS/6);
						ballx = (COLS/2);
						bally = (ROWS/2);
						// randomize ball
						randomizeBall();
						
						lcd_clear_screen(LCD_COLOR_BLACK);
						// draw players
						lcd_draw_image(
													player1x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player1y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						lcd_draw_image(
													player2x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player2y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw ball
						lcd_draw_image(
													ballx,                 // X Pos
													ballWidth,   // Image Horizontal Width
													bally,                 // Y Pos
													ballHeight,  // Image Vertical Height
													ballBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw score
						drawScore();
						break;
					// more dimensional pong
					case 4:
						player1x = (COLS/2);
						player1y = (ROWS/6);
						player2x = (COLS/2);
						player2y = (5*ROWS/6);
						ballx = (COLS/2);
						bally = (ROWS/2);
						// randomize ball
						randomizeBall();
						
						lcd_clear_screen(LCD_COLOR_BLACK);
						// draw players
						lcd_draw_image(
													player1x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player1y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						lcd_draw_image(
													player2x,                 // X Pos
													playerWidth,   // Image Horizontal Width
													player2y,                 // Y Pos
													playerHeight,  // Image Vertical Height
													playerBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw ball
						lcd_draw_image(
													ballx,                 // X Pos
													ballWidth,   // Image Horizontal Width
													bally,                 // Y Pos
													ballHeight,  // Image Vertical Height
													ballBitmaps,       // Image
													LCD_COLOR_BLUE,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						// draw score
						drawScore();
						break;
					// reset scores
					case 5: 
						// easter egg with scores of 4-2
						if (player1score==4 && player2score==2) {
							lcd_clear_screen(LCD_COLOR_MAGENTA);
							// lcd draw image of an easter egg
							lcd_draw_image(
											(COLS/2),                 // X Pos
											easterEggWidth,   // Image Horizontal Width
											(ROWS/2),                 // Y Pos
											easterEggHeight,  // Image Vertical Height
											easterEggBitmaps,       // Image
											LCD_COLOR_ORANGE,      // Foreground Color
											LCD_COLOR_BLACK     // Background Color
										);
							player1score = 0;
							player2score = 0;
							// write scores to EEPROM
							eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
							eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
							// back to 0 gamemode 
							gamemode = 0;					
							// write currently selected gamemode to EEPROM
							eeprom_byte_write(I2C1_BASE,addrGamemode, gamemode);
						
							done = 1;
							break;
						}
						player1score = 0;
						player2score = 0;
						// write scores to EEPROM
						eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
						eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						
						// back to 0 gamemode and stay in the menu
						gamemode = 0;
						menu = 1;
					
						// write currently selected gamemode to EEPROM
						eeprom_byte_write(I2C1_BASE,addrGamemode, gamemode);
						// redraw the menu 
						lcd_clear_screen(LCD_COLOR_BLACK);
						drawMenu();
						break;
					default:
						gamemode = 0; // set to regular pong if random value
						break;
				} 
			}
		}
		// post-game 
		else if (player1score>=winScore || player2score>=winScore){
				delayWaitFunction();
				if (!waitLoser){
					// initialize post game 
					if (player1score==winScore || player2score==winScore) {
							if (player1score==winScore && player2score==winScore) {
									// freak tie occurence
									player1score = 0;
									player2score = 0;
									// write scores to EEPROM
									eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
									eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						
									// so reset the scores and keep playing!
							}
							else if (player1score==winScore){
								player2score = 0;
								player1score++;
								// write scores to EEPROM
								eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
								eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
								
							}
							// player2score==winScore
							else {
								player1score = 0;
								player2score++;
								//write scores to EEPROM
								eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
								eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
							}
					}
					else if (player1score>=winScore) {
							lcd_clear_screen(LCD_COLOR_BLACK);
							drawFireworks();
							lcd_draw_image(
														(COLS/2),                 // X Pos
														player1winWidth,   // Image Horizontal Width
														(ROWS/2),                 // Y Pos
														player1winHeight,  // Image Vertical Height
														player1winBitmaps,       // Image
														LCD_COLOR_CYAN,      // Foreground Color
														LCD_COLOR_BLACK     // Background Color
													);
							player1score = 0;
							player2score = 0;
							//write scores to EEPROM
							eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
							eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
							done = true;
							
					}
					else {
							lcd_clear_screen(LCD_COLOR_BLACK);
							drawFireworks();
							lcd_draw_image(
													(COLS/2),                 // X Pos
													player2winWidth,   // Image Horizontal Width
													(ROWS/2),                 // Y Pos
													player2winHeight,  // Image Vertical Height
													player2winBitmaps,       // Image
													LCD_COLOR_MAGENTA,      // Foreground Color
													LCD_COLOR_BLACK     // Background Color
												);
						player1score = 0;
						player2score = 0;
						//write scores to EEPROM
						eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
						eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						done = true;
					}
				
				}
		}
		// in-game gameplay
		else {
				if (gameTick()) {
							switch (gamemode) {
//////////////// regular pong
							case 0:
								// PLAYER MOVEMENT
								// player 1: touchscreen 
								// screen goes from 0 to ROWS=320 and 0 to COLS=240
								if (ft6x06_read_td_status()>0){
									int x;
									// hide player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									x = ft6x06_read_x();
									if (player1x-x>maxSpeed)
										player1x = player1x-maxSpeed;
									else if (x-player1x>maxSpeed)
										player1x = player1x+maxSpeed;
									else
										player1x = x;
									
									// redraw player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
								}
								
								
								// player 2: joystick
								// joystick_y and joystick_x from 000 to FFF
								if (direction!=0){
									// hide player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									if (direction==2) { // left  
										player2x = player2x-maxSpeed;
										// wall cutoffs
										if (player2x<0+playerWidth/2)
											player2x = player2x+maxSpeed;
									}
									else if (direction==4) { // right
										player2x = player2x+maxSpeed;
										// wall cutoffs
										if (player2x>COLS-playerWidth/2)
											player2x = player2x-maxSpeed;
									}
									
									// redraw player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLUE,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}
								
								// BALL MOVEMENT
								delayWaitFunction();
								if (!ballWait) {
									// hide ball
									lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									ballx = ballx+ballChangex;
									bally = bally+ballChangey;
									// player 1 point
									if (bally>ROWS-ballHeight){
											// point
											player1score = player1score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
											if (player1score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									// player 2 point
									else if (bally<ballHeight){
											// point
											player2score = player2score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						
											if (player2score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									else {
										// player contact 
										if (playercontact()) {
											ballChangey = -ballChangey;
											// for each player contact, randomly change the changex by a bit so not an infinite loop of up and down											
											if (ballChangex==0 && rand()%noupdown==0)
												ballChangex = ballChangex + (-1)^(rand() % 2); 
										}	
										// wallcontact 
										if (wallcontact()) {
											ballChangex = -ballChangex;
										}
									 }
									 //redraw ball
									 lcd_draw_image(
																ballx,                 // X Pos
																ballWidth,   // Image Horizontal Width
																bally,                 // Y Pos
																ballHeight,  // Image Vertical Height
																ballBitmaps,       // Image
																LCD_COLOR_RED,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}
								break;
//////////////// GRAVITY pong
							// paddle and paddle slow down in the center and speed up at extremes
							case 1:
								// PLAYER MOVEMENT
								// player 1: touchscreen 
								// screen goes from 0 to ROWS=320 and 0 to COLS=240
								if (ft6x06_read_td_status()>0){
									int x;
									// hide player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									x = ft6x06_read_x();
									if ((COLS/2)-player1x>0) // left side
									{
										if (player1x-x>0) // move left
											player1x = player1x-(((COLS/2)-player1x)/10 + 1);
										else // (x-player1x>0) move right
											player1x = player1x+(((COLS/2)-player1x)/10 + 1);
										// wall cutoffs
										if (player1x<0+playerWidth/2)
											player1x = playerWidth/2;
										if (player1x>COLS-playerWidth/2)
											player1x = COLS-playerWidth/2;
									}
									else { // right side
										if (player1x-x>0) // move left
											player1x = player1x-((player1x-(COLS/2))/10 + 1);
										else // (x-player1x>0) move right
											player1x = player1x+((player1x-(COLS/2))/10 + 1);
										// wall cutoffs
										if (player1x<0+playerWidth/2)
											player1x = playerWidth/2;
										if (player1x>COLS-playerWidth/2)
											player1x = COLS-playerWidth/2;
									}
									
									// redraw player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
								}
								
								
								// player 2: joystick
								// joystick_y and joystick_x from 000 to FFF
								if (direction!=0){
									// hide player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									
									if ((COLS/2)-player2x>0) // left side
									{
										if (direction==2) { // move left  
											player2x = player2x-(((COLS/2)-player2x)/10 + 1);
											// wall cutoffs
											if (player2x<0+playerWidth/2)
												player2x = playerWidth/2;
										}
										else if (direction==4) { // move right
											player2x = player2x+(((COLS/2)-player2x)/10 + 1);
											// wall cutoffs
											if (player2x>COLS-playerWidth/2)
												player2x = COLS-playerWidth/2;
										}
									}
									else { // right side										if (player1x-x>0) // move left
											if (direction==2) { // move left  
												player2x = player2x-((player2x-(COLS/2))/10 + 1);
												// wall cutoffs
												if (player2x<0+playerWidth/2)
													player2x = playerWidth/2;
												}
											else if (direction==4) { // move right
												player2x = player2x+((player2x-(COLS/2))/10 + 1);
												// wall cutoffs
												if (player2x>COLS-playerWidth/2)
													player2x = COLS-playerWidth/2;
											}
									}
									
									// redraw player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLUE,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}
								
								// BALL MOVEMENT
								delayWaitFunction();
								if (!ballWait) {
									// hide ball
									lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									ballx = ballx+ballChangex;
									if (bally==ROWS/2) // middle
										bally = bally+(ballChangey>0 ? 1 : -1);
									else if (bally>ROWS/2) { // bottom half 
										if (ballChangey>0)
											bally = bally+(ballChangey/((bally-(ROWS/2))/40))+1;
										else
											bally = bally+(ballChangey/((bally-(ROWS/2))/40))-1;
									}
									else {// top half
										if (ballChangey>0)
											bally = bally+(ballChangey/(((ROWS/2)-bally)/40))+1;
										else
											bally = bally+(ballChangey/(((ROWS/2)-bally)/40))-1;
									}
									// player 1 point
									if (bally>ROWS-ballHeight){
											// point
											player1score = player1score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
											if (player1score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									// player 2 point
									else if (bally<ballHeight){
											// point
											player2score = player2score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						
											if (player2score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									else {
										// player contact 
										if (playercontact()) {
											ballChangey = -ballChangey;
											// for each player contact, randomly change the changex by a bit so not an infinite loop of up and down											
											if (ballChangex==0 && rand()%noupdown==0)
												ballChangex = ballChangex + (-1)^(rand() % 2); 
										}	
										// wallcontact 
										if (wallcontact()) {
											ballChangex = -ballChangex;
										}
									 }
									 //redraw ball
									 lcd_draw_image(
																ballx,                 // X Pos
																ballWidth,   // Image Horizontal Width
																bally,                 // Y Pos
																ballHeight,  // Image Vertical Height
																ballBitmaps,       // Image
																LCD_COLOR_RED,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}
								break;
//////////////// heisenberg pong
							// can see paddle or ball but not both
							case 2:
								// required at the start for blink boolean to be equal for paddles and ball
								delayWaitFunction();
							
								// PLAYER MOVEMENT
								// player 1: touchscreen 
								// screen goes from 0 to ROWS=320 and 0 to COLS=240
								if (ft6x06_read_td_status()>0){
									int x;
									// hide player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									x = ft6x06_read_x();
									if (player1x-x>maxSpeed)
										player1x = player1x-maxSpeed;
									else if (x-player1x>maxSpeed)
										player1x = player1x+maxSpeed;
									else
										player1x = x;
									
									// maybe redraw player1 
									if (!heisenbergBlinking) {
											lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									}
								}
								// player 1 not moving
								else {
									if (heisenbergBlinking) {
										// hide player1
										lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									}
									else {
										lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									}
								}
								
								
								// player 2: joystick
								// joystick_y and joystick_x from 000 to FFF
								if (direction!=0){
									// hide player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									if (direction==2) { // left  
										player2x = player2x-maxSpeed;
										// wall cutoffs
										if (player2x<0+playerWidth/2)
											player2x = player2x+maxSpeed;
									}
									else if (direction==4) { // right
										player2x = player2x+maxSpeed;
										// wall cutoffs
										if (player2x>COLS-playerWidth/2)
											player2x = player2x-maxSpeed;
									}
									
									// maybe redraw player2
									if (!heisenbergBlinking){
										lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLUE,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									}
								}
								// player 2 not moving
								else {
									if (heisenbergBlinking){
										// hide player2
										lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									}
									else {
										lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLUE,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									}
								}
								
								// BALL MOVEMENT
								if (!ballWait) {
									// hide ball
									lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									ballx = ballx+ballChangex;
									bally = bally+ballChangey;
									// player 1 point
									if (bally>ROWS-ballHeight){
											// point
											player1score = player1score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
											if (player1score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									// player 2 point
									else if (bally<ballHeight){
											// point
											player2score = player2score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						
											if (player2score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									else {
										// player contact 
										if (playercontact()) {
											ballChangey = -ballChangey;
											// for each player contact, randomly change the changex by a bit so not an infinite loop of up and down											
											if (ballChangex==0 && rand()%noupdown==0)
												ballChangex = ballChangex + (-1)^(rand() % 2); 
										}	
										// wallcontact 
										if (wallcontact()) {
											ballChangex = -ballChangex;
										}
									 }
									 // maybe redraw ball
									 if (heisenbergBlinking){
										 lcd_draw_image(
																ballx,                 // X Pos
																ballWidth,   // Image Horizontal Width
																bally,                 // Y Pos
																ballHeight,  // Image Vertical Height
																ballBitmaps,       // Image
																LCD_COLOR_RED,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									 }
								}
								// ball not moving
								else {
									if (!heisenbergBlinking){
										// hide ball
										lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									}
									else {
										lcd_draw_image(
																ballx,                 // X Pos
																ballWidth,   // Image Horizontal Width
																bally,                 // Y Pos
																ballHeight,  // Image Vertical Height
																ballBitmaps,       // Image
																LCD_COLOR_RED,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									}
								}
								 break;
//////////////// weird controls pong
							// accelerometer vs buttons
							case 3:
								// PLAYER MOVEMENT
								// player 1: accelerometer 
								// screen goes from 0 to ROWS=320 and 0 to COLS=240
								accelX = accel_read_x();
								if (accelX>accelSensitivity | accelX<-accelSensitivity){
									// hide player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									
									if (accelX>accelSensitivity){
										player1x = player1x-maxSpeed;
										// wall cutoffs
										if (player1x<0+playerWidth/2)
											player1x = player1x+maxSpeed;
									}
									else {// accelX<-accelSensitivity
										player1x = player1x+maxSpeed;
										// wall cutoffs
										if (player1x>COLS-playerWidth/2)
											player1x = player1x-maxSpeed;
									}
									
									// redraw player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
								}
								
								
								// player 2: IO buttons
								if (ioButtonLeft|ioButtonRight){
									// hide player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									if (ioButtonLeft) { // left  
										player2x = player2x-maxSpeed;
										// wall cutoffs
										if (player2x<0+playerWidth/2)
											player2x = player2x+maxSpeed;
									}
									if (ioButtonRight) { // right
										player2x = player2x+maxSpeed;
										// wall cutoffs
										if (player2x>COLS-playerWidth/2)
											player2x = player2x-maxSpeed;
									}
									
									// redraw player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLUE,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}
								
								// BALL MOVEMENT
								delayWaitFunction();
								if (!ballWait) {
									// hide ball
									lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									ballx = ballx+ballChangex;
									bally = bally+ballChangey;
									// player 1 point
									if (bally>ROWS-ballHeight){
											// point
											player1score = player1score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
											if (player1score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									// player 2 point
									else if (bally<ballHeight){
											// point
											player2score = player2score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						
											if (player2score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									else {
										// player contact 
										if (playercontact()) {
											ballChangey = -ballChangey;
											// for each player contact, randomly change the changex by a bit so not an infinite loop of up and down											
											if (ballChangex==0 && rand()%noupdown==0)
												ballChangex = ballChangex + (-1)^(rand() % 2); 
										}	
										// wallcontact 
										if (wallcontact()) {
											ballChangex = -ballChangex;
										}
									 }
									 //redraw ball
									 lcd_draw_image(
																ballx,                 // X Pos
																ballWidth,   // Image Horizontal Width
																bally,                 // Y Pos
																ballHeight,  // Image Vertical Height
																ballBitmaps,       // Image
																LCD_COLOR_RED,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}
								
								break;
//////////////// multi dimensional pong
							// not just left/right, but up down
							case 4:
								// PLAYER MOVEMENT
								// player 1: touchscreen 
								// screen goes from 0 to ROWS=320 and 0 to COLS=240
								if (ft6x06_read_td_status()>0){
									int x, y;
									// hide player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									x = ft6x06_read_x();
									y = ft6x06_read_y();
									if (player1x-x>maxSpeed)
										player1x = player1x-maxSpeed;
									else if (x-player1x>maxSpeed)
										player1x = player1x+maxSpeed;
									else
										player1x = x;
									if (player1y-y>maxSpeed)
										player1y = player1y-maxSpeed;
									else if (y-player1y>maxSpeed)
										player1y = player1y+maxSpeed;
									else
										player1y = y;
									
									// redraw player1
									lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
								}
								
								
								// player 2: joystick
								// joystick_y and joystick_x from 000 to FFF
								if (direction!=0){
									// hide player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
									if (direction==2) { // left  
										player2x = player2x-maxSpeed;
										// wall cutoffs
										if (player2x<0+playerWidth/2)
											player2x = player2x+maxSpeed;
									}
									else if (direction==4) { // right
										player2x = player2x+maxSpeed;
										// wall cutoffs
										if (player2x>(COLS-playerWidth/2))
											player2x = player2x-maxSpeed;
									}
									else if (direction==1) { // up
										player2y = player2y-maxSpeed;
										// wall cutoffs
										if (player2y<0+(playerHeight/2))
											player2y = player2y+maxSpeed;
									}
									else if (direction==3) { // down
										player2y = player2y+maxSpeed;
										// wall cutoffs
										if (player2y>(ROWS-(playerHeight/2)))
											player2y = player2y-maxSpeed;
									}
									
									// redraw player2
									lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLUE,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}
								
								// BALL MOVEMENT
								delayWaitFunction();
								if (!ballWait) {
									// hide ball
									lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
									ballx = ballx+ballChangex;
									bally = bally+ballChangey;
									// player 1 point
									if (bally>ROWS-ballHeight){
											// point
											player1score = player1score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer1Score, player1score);
											if (player1score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset players
											lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
											lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
											player1x = (COLS/2);
											player1y = (ROWS/6);
											player2x = (COLS/2);
											player2y = (5*ROWS/6);
											lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
											lcd_draw_image(
															player2x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player2y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
											// reset ball 
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									// player 2 point
									else if (bally<ballHeight){
											// point
											player2score = player2score+1;
											// write score to EEPROM
											eeprom_byte_write(I2C1_BASE,addrPlayer2Score, player2score);
						
											if (player2score>=winScore) {
												// delay here for the loser to realize they've just lost and freeze frame that moment
												waitLoser = 1;	
												lcd_draw_image(
															ballx,                 // X Pos
															ballWidth,   // Image Horizontal Width
															bally,                 // Y Pos
															ballHeight,  // Image Vertical Height
															ballBitmaps,       // Image
															LCD_COLOR_RED,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
												break;
											}
											
											// redraw score
											drawScore();
											// reset players
											lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLACK,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
											lcd_draw_image(
																player2x,                 // X Pos
																playerWidth,   // Image Horizontal Width
																player2y,                 // Y Pos
																playerHeight,  // Image Vertical Height
																playerBitmaps,       // Image
																LCD_COLOR_BLACK,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
											player1x = (COLS/2);
											player1y = (ROWS/6);
											player2x = (COLS/2);
											player2y = (5*ROWS/6);
											lcd_draw_image(
															player1x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player1y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
											lcd_draw_image(
															player2x,                 // X Pos
															playerWidth,   // Image Horizontal Width
															player2y,                 // Y Pos
															playerHeight,  // Image Vertical Height
															playerBitmaps,       // Image
															LCD_COLOR_BLUE,      // Foreground Color
															LCD_COLOR_BLACK     // Background Color
														);
											// reset ball and wait for a bit
											ballx = COLS/2;
											bally = ROWS/2;
											// randomize
											randomizeBall();
											
											// wait for players to reset
											ballWait = 1; 
									}
									else {
										// player contact 
										if (playercontact()) {
											ballChangey = -ballChangey;
											// for each player contact, randomly change the changex by a bit so not an infinite loop of up and down											
											if (ballChangex==0 && rand()%noupdown==0)
												ballChangex = ballChangex + (-1)^(rand() % 2); 
										}	
										// wallcontact 
										if (wallcontact()) {
											ballChangex = -ballChangex;
										}
									 }
									 //redraw ball
									 lcd_draw_image(
																ballx,                 // X Pos
																ballWidth,   // Image Horizontal Width
																bally,                 // Y Pos
																ballHeight,  // Image Vertical Height
																ballBitmaps,       // Image
																LCD_COLOR_RED,      // Foreground Color
																LCD_COLOR_BLACK     // Background Color
															);
								}

								break;
							// possible future gamemodes
							// multiple ball pong
							// powerup pong
							// colorful craziness pong
							// run from chasing ball
							// keep ball from touching wall
							// pong where the old images aren't hidden (so laggy window being dragged around pong or slurred pong), screen only cleared on point
							// random game mode
							default:
								gamemode = 0; // set to regular pong if error value
								break;
						} 
						}
		}
	};
	// freeze screen at the end, blink LEDs
	while (1) {
		// blink LEDs
		delayWaitFunction();
		if (heisenbergBlinking){
			// turn all on
			io_expander_write_reg(MCP23017_GPIOA_R, 0xFF);
		}
		else {
			// turn all off
			io_expander_write_reg(MCP23017_GPIOA_R, 0x00);
		}
		
	}
}
