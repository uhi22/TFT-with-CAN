
#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"
#include "Font7s.h"
#include "Font16.h"
#include "Font32.h"
//#include "Font64.h"
#include "canbus.h"
#include <stdio.h>
#include <string.h>
#include "flashhandler.h"

/* fixed-size-font: e.g. this: https://github.com/idispatch/raster-fonts/blob/master/font-9x16.c */

const char strStatusMessages[] = "\
StatusZero              \
Waiting App Handshake   \
Stopped                 \
Schema Negotiated       \
Session Established     \
Services Discovered     \
Payment Selected        \
Power Delivery          \
Charge Parameter Dcvy   \
Cable Check             \
Precharging             \
Contract Auth           \
Authorization           \
Current Demand          \
Welding Detection       \
Session Stop            \
Stopped Forever         \
TCP Connection Broken   \
Listening TCP           \
TCP Connected           \
Invalid20               \
Invalid21               \
Invalid22               \
Invalid23               \
";

#define SIZE_OF_ONE_MESSAGE 24
#define LAST_MESSAGE_INDEX 23

extern uint32_t nNumberOfReceivedMessages;
extern uint32_t nNumberOfCanInterrupts;
extern uint8_t timeoutcounter_595;

uint32_t oldTime100ms;

#define COLOR_BUFFER_SIZE 6000 /* bytes for one character. Is twice the pixel count of one character. */
uint8_t myColorBuffer[COLOR_BUFFER_SIZE];
uint16_t colorBufferIndex;
static char BufferText1[40];
//static char BufferText2[41];

uint8_t nCurrentPage, nLastPage;
uint16_t counterPageSwitch;
uint16_t nMainLoops;

uint8_t debugByte0=0xff;
uint8_t debugByte1=0xfe;
uint8_t debugByte2=0x00;
uint8_t debugByte3=0xa5;
extern uint8_t rawmessage678[8];
extern uint8_t rawmessage679[8];
extern uint8_t rawmessage67A[8];


uint16_t oldTestGraphics_DrawChar(char ch, uint16_t X, uint16_t Y, uint16_t color, uint16_t bgcolor, uint8_t size)
{
    uint16_t width;
    uint16_t height;
    uint16_t pixelColor;
    uint8_t const *charBitmapPtr;
    int16_t gap;
    uint16_t bitnr;
    uint8_t mask;
    uint8_t bytesPerLine;
    
	if ((ch < 32) || (ch > 127)) return 0;
    ch = ch - 32;
    if (size == 2) {
      charBitmapPtr = chrtbl_f16[(uint8_t)ch];
      width = widtbl_f16[(uint8_t)ch];
      height = chr_hgt_f16;
      gap = 1;
    }
    if (size == 4) {
      charBitmapPtr = chrtbl_f32[(uint8_t)ch];
      width = widtbl_f32[(uint8_t)ch];
      height = chr_hgt_f32;
      gap = -3;
    }
   if (size == 6) {
      //charBitmapPtr = chrtbl_f64[(uint8_t)ch];
      //width = widtbl_f64[(uint8_t)ch];
      //height = chr_hgt_f64;
      //gap = -3;
   }
    if (size == 7) {
      charBitmapPtr = chrtbl_f7s[(uint8_t)ch];
      width = widtbl_f7s[(uint8_t)ch];
      height = chr_hgt_f7s;
      gap = 2;
    }
    colorBufferIndex = 0;
    bytesPerLine = (width+7)/8;
	for (int j=0; j < height; j++)
	{
        bitnr = 0;
		for (int i=0; i < width; i++)
		{
            mask = 1 << (7 - (bitnr%8));
            if (charBitmapPtr[j*bytesPerLine + bitnr/8] & mask) {
                pixelColor = color;
             } else {
                pixelColor = bgcolor;
            }
            //ILI9341_DrawPixel(X+i, Y+j, pixelColor);
            myColorBuffer[colorBufferIndex] = (uint8_t)(pixelColor >> 8);
            myColorBuffer[colorBufferIndex+1] = (uint8_t)pixelColor;
            if (colorBufferIndex<COLOR_BUFFER_SIZE-2) {
              colorBufferIndex+=2;
            }
            bitnr++;
		}
	}
    ILI9341_SetAddress(X, Y, X+width-1, Y+height-1);
    //ILI9341_DrawColorBurst(color, height*width);
    HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(HSPI_INSTANCE, myColorBuffer, colorBufferIndex, 10);
	HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
    return width+gap;
}

extern unsigned char console_font_12x16[];

uint16_t drawChar12x16(char ch, uint16_t X, uint16_t Y, uint16_t color, uint16_t bgcolor) {
    uint16_t width;
    uint16_t height;
    uint16_t pixelColor;
    uint8_t const *charBitmapPtr;
    uint16_t bitnr;
    uint8_t mask;
    uint8_t bytesPerLine;
    uint8_t yFactor = 6;
    uint8_t xFactor = 3;
    uint8_t n,m;

    width = 12;
    height = 16;

    charBitmapPtr = &console_font_12x16[2*height*(uint16_t)ch];

    int i;
    colorBufferIndex = 0;
    bytesPerLine = (width+7)/8;
	for (int j=0; j < height; j++)
	{
        bitnr = 0;
		for (i=0; i < width; i++) {
            mask = 1 << (7 - (bitnr%8));
            if (charBitmapPtr[j*bytesPerLine + bitnr/8] & mask) {
                pixelColor = color;
             } else {
                pixelColor = bgcolor;
            }
            //ILI9341_DrawPixel(X+i, Y+j, pixelColor);
            myColorBuffer[colorBufferIndex] = (uint8_t)(pixelColor >> 8);
            myColorBuffer[colorBufferIndex+1] = (uint8_t)pixelColor;
            if (colorBufferIndex<COLOR_BUFFER_SIZE-2) {
              colorBufferIndex+=2;
            }
            for (m=1; m<xFactor; m++) {
				myColorBuffer[colorBufferIndex] = (uint8_t)(pixelColor >> 8);
				myColorBuffer[colorBufferIndex+1] = (uint8_t)pixelColor;
				if (colorBufferIndex<COLOR_BUFFER_SIZE-2) {
				  colorBufferIndex+=2;
				}
            }
            bitnr++;
		}
		for (n=1; n<yFactor; n++) {
			for (i=0; i < xFactor*width; i++) {
				uint16_t lineOffset = xFactor*2*width; /* pixel per line */
				myColorBuffer[colorBufferIndex]   = myColorBuffer[colorBufferIndex-lineOffset];
				myColorBuffer[colorBufferIndex+1] = myColorBuffer[colorBufferIndex+1-lineOffset];
				if (colorBufferIndex<COLOR_BUFFER_SIZE-2) {
				  colorBufferIndex+=2;
				}
			}
		}
	}
	ILI9341_SetAddress(X, Y, X+xFactor*width-1, Y+yFactor*height-1);
	HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
	HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
	HAL_SPI_Transmit(HSPI_INSTANCE, myColorBuffer, colorBufferIndex, 10);
	HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
    return xFactor*width;
}

uint16_t TestGraphics_DrawChar(char ch, uint16_t X, uint16_t Y, uint16_t color, uint16_t bgcolor, uint8_t size)
{
    uint16_t width;
    uint16_t height;
    uint16_t pixelColor;
    uint8_t const *charBitmapPtr;
    int16_t gap;
    uint16_t bitnr;
    uint8_t mask;
    uint8_t bytesPerLine;
    uint8_t isDoubleWidth = 0;
    uint8_t isDoubleHeight = 0;
    int i;

    if (size & 64) {
    	isDoubleHeight = 1;
    	size &= ~64;
    }

	if ((ch < 32) || (ch > 127)) return 0;
    ch = ch - 32;
    if (size == 2) {
      charBitmapPtr = chrtbl_f16[(uint8_t)ch];
      width = widtbl_f16[(uint8_t)ch];
      height = chr_hgt_f16;
      gap = 1;
    }
    if (size == 4) {
      charBitmapPtr = chrtbl_f32[(uint8_t)ch];
      width = widtbl_f32[(uint8_t)ch];
      height = chr_hgt_f32;
      gap = -3;
    }
   if (size == 6) {
      //charBitmapPtr = chrtbl_f64[(uint8_t)ch];
      //width = widtbl_f64[(uint8_t)ch];
      //height = chr_hgt_f64;
      //gap = -3;
   }
    if (size == 7) {
      charBitmapPtr = chrtbl_f7s[(uint8_t)ch];
      width = widtbl_f7s[(uint8_t)ch];
      height = chr_hgt_f7s;
      gap = 2;
    }
    if (size == 9) {
    	return drawChar12x16(ch, X, Y, color, bgcolor);
    }
    colorBufferIndex = 0;
    bytesPerLine = (width+7)/8;
	for (int j=0; j < height; j++)
	{
        bitnr = 0;
		for (i=0; i < width; i++)
		{
            mask = 1 << (7 - (bitnr%8));
            if (charBitmapPtr[j*bytesPerLine + bitnr/8] & mask) {
                pixelColor = color;
             } else {
                pixelColor = bgcolor;
            }
            //ILI9341_DrawPixel(X+i, Y+j, pixelColor);
            myColorBuffer[colorBufferIndex] = (uint8_t)(pixelColor >> 8);
            myColorBuffer[colorBufferIndex+1] = (uint8_t)pixelColor;
            if (colorBufferIndex<COLOR_BUFFER_SIZE-2) {
              colorBufferIndex+=2;
            }
            if (isDoubleWidth) {
                myColorBuffer[colorBufferIndex] = (uint8_t)(pixelColor >> 8);
                myColorBuffer[colorBufferIndex+1] = (uint8_t)pixelColor;
                if (colorBufferIndex<COLOR_BUFFER_SIZE-2) {
                  colorBufferIndex+=2;
                }
            }
            bitnr++;
		}
		if (isDoubleHeight) {
			/* we duplicate the complete last line */
			for (i=0; i < width; i++) {
			  uint16_t lineOffset = 2*width; /* pixel per line */
              myColorBuffer[colorBufferIndex]   = myColorBuffer[colorBufferIndex-lineOffset];
              myColorBuffer[colorBufferIndex+1] = myColorBuffer[colorBufferIndex+1-lineOffset];
              if (colorBufferIndex<COLOR_BUFFER_SIZE-2) {
                colorBufferIndex+=2;
              }
			}
		}
	}
	if (!isDoubleWidth) {
		if (isDoubleHeight) {
			ILI9341_SetAddress(X, Y, X+width-1, Y+2*height-1);
			//ILI9341_DrawColorBurst(color, height*width);
			HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
			HAL_SPI_Transmit(HSPI_INSTANCE, myColorBuffer, colorBufferIndex, 10);
			HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
			return width+gap;
		} else {
			ILI9341_SetAddress(X, Y, X+width-1, Y+height-1);
			//ILI9341_DrawColorBurst(color, height*width);
			HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
			HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
			HAL_SPI_Transmit(HSPI_INSTANCE, myColorBuffer, colorBufferIndex, 10);
			HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
			return width+gap;
		}
	} else {
		/* twice the size */
		ILI9341_SetAddress(X, Y, X+2*width-1, Y+height-1);
		//ILI9341_DrawColorBurst(color, height*width);
		HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
		HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
		HAL_SPI_Transmit(HSPI_INSTANCE, myColorBuffer, colorBufferIndex, 10);
		HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
		return 2*width+gap;
	}
}

int16_t TestGraphics_drawString(char *string, int16_t poX, int16_t poY, uint16_t color, uint16_t bgcolor, uint8_t size)
{
    int16_t sumX = 0;

    while(*string)
    {
        int16_t xPlus = TestGraphics_DrawChar(*string, poX, poY, color, bgcolor, size);
        sumX += xPlus;
        string++;
        poX += xPlus;                            /* Move cursor right       */
    }
    return sumX;
}


void showpage1(uint8_t blInit) {
    #define LINESIZEY 20
    #define LINESIZEY_RIGHTSIDE 14
	if (blInit) {
		ILI9341_FillScreen(BLACK);
		  ILI9341_DrawText("loops", FONT3, 10, 0*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("rxCount", FONT3, 10, 1*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("SOC", FONT3, 10, 2*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("---- Target ----", FONT3, 10, 3*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("---- Present ----", FONT3, 10, 101, GREENYELLOW, BLACK);
		  //ILI9341_DrawText("EVSEPresentV", FONT2, 0, 170, GREENYELLOW, BLACK);
		  //ILI9341_DrawText("uCcsInlet_V", FONT3, 200, 170, GREENYELLOW, BLACK);

		  ILI9341_DrawText("rawdata", FONT1, 220, 0*LINESIZEY, GREENYELLOW, BLACK);
          #define X 220
		  ILI9341_DrawText("0", FONT3, X, 1*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  ILI9341_DrawText("1", FONT3, X, 2*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  ILI9341_DrawText("2", FONT3, X, 3*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  ILI9341_DrawText("3", FONT3, X, 4*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  ILI9341_DrawText("4", FONT3, X, 5*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  ILI9341_DrawText("5", FONT3, X, 6*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  ILI9341_DrawText("6", FONT3, X, 7*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  ILI9341_DrawText("7", FONT3, X, 8*LINESIZEY_RIGHTSIDE, GREENYELLOW, BLACK);
		  //ILI9341_DrawHollowRectangleCoord(228, 0, 312, 9*LINESIZEY_RIGHTSIDE, DARKCYAN);
          #undef X
	}
    sprintf(BufferText1, "%d  ", nMainLoops);
    (void)TestGraphics_drawString(BufferText1, 100, 0*LINESIZEY, GREENYELLOW, BLACK, 2);
    //(void)TestGraphics_drawString(BufferText, 150, 130, GREENYELLOW, DARKCYAN, 6);
    //(void)TestGraphics_drawString(BufferText, 150, 190, YELLOW, BLUE, 7);

    sprintf(BufferText1, "%ld  ", nNumberOfReceivedMessages);
    (void)TestGraphics_drawString(BufferText1, 100, 1*LINESIZEY, GREENYELLOW, BLACK, 2);

    sprintf(BufferText1, "%d %%", rawmessage678[1]); /* SOC */
    (void)TestGraphics_drawString(BufferText1, 100, 2*LINESIZEY, YELLOW, BLACK, 4);

    uint32_t u, i;
    /* target voltage and target current */
    u=rawmessage679[1];
    u<<=8;
    u|=rawmessage679[0];
    i=rawmessage679[3];
    i<<=8;
    i|=rawmessage679[2];
    sprintf(BufferText1, "%ld V %ld A   ", u, i);
    (void)TestGraphics_drawString(BufferText1, 1, 76, YELLOW, BLACK, 4);

    /* present voltage and present current */
    u=rawmessage679[5];
    u<<=8;
    u|=rawmessage679[4];
    i=rawmessage679[7];
    i<<=8;
    i|=rawmessage679[6];
    sprintf(BufferText1, "%ld V %ld A   ", u, i);
    (void)TestGraphics_drawString(BufferText1, 1, 115, YELLOW, BLACK, 4);

    //sprintf(BufferText1, "%d  ", canDebugValue1);
    //(void)TestGraphics_drawString(BufferText1, 100, 4*LINESIZEY, GREENYELLOW, BLACK, 2);

    //sprintf(BufferText1, "%d  ", EVSEPresentVoltage);
    //(void)TestGraphics_drawString(BufferText1, 0, 182, GREENYELLOW, BLACK, 6);

    //sprintf(BufferText1, "%d  ", uCcsInlet_V);
    //(void)TestGraphics_drawString(BufferText1, 200, 182, GREENYELLOW, BLACK, 6);

    /* debug data on the right side */
    #define X 242
    sprintf(BufferText1, "%02x ", rawmessage678[0]);
    (void)TestGraphics_drawString(BufferText1, X, 1*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage678[1]);
    (void)TestGraphics_drawString(BufferText1, X, 2*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage678[2]);
    (void)TestGraphics_drawString(BufferText1, X, 3*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage678[3]);
    (void)TestGraphics_drawString(BufferText1, X, 4*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage678[4]);
    (void)TestGraphics_drawString(BufferText1, X, 5*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage678[5]);
    (void)TestGraphics_drawString(BufferText1, X, 6*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage678[6]);
    (void)TestGraphics_drawString(BufferText1, X, 7*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage678[7]);
    (void)TestGraphics_drawString(BufferText1, X, 8*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    #undef X

    #define X 265
    sprintf(BufferText1, "%02x ", rawmessage679[0]);
    (void)TestGraphics_drawString(BufferText1, X, 1*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage679[1]);
    (void)TestGraphics_drawString(BufferText1, X, 2*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage679[2]);
    (void)TestGraphics_drawString(BufferText1, X, 3*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage679[3]);
    (void)TestGraphics_drawString(BufferText1, X, 4*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage679[4]);
    (void)TestGraphics_drawString(BufferText1, X, 5*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage679[5]);
    (void)TestGraphics_drawString(BufferText1, X, 6*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage679[6]);
    (void)TestGraphics_drawString(BufferText1, X, 7*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    sprintf(BufferText1, "%02x ", rawmessage679[7]);
    (void)TestGraphics_drawString(BufferText1, X, 8*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
    #undef X

	#define X 290
	sprintf(BufferText1, "%02x ", rawmessage67A[0]);
	(void)TestGraphics_drawString(BufferText1, X, 1*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	sprintf(BufferText1, "%02x ", rawmessage67A[1]);
	(void)TestGraphics_drawString(BufferText1, X, 2*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	sprintf(BufferText1, "%02x ", rawmessage67A[2]);
	(void)TestGraphics_drawString(BufferText1, X, 3*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	sprintf(BufferText1, "%02x ", rawmessage67A[3]);
	(void)TestGraphics_drawString(BufferText1, X, 4*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	sprintf(BufferText1, "%02x ", rawmessage67A[4]);
	(void)TestGraphics_drawString(BufferText1, X, 5*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	sprintf(BufferText1, "%02x ", rawmessage67A[5]);
	(void)TestGraphics_drawString(BufferText1, X, 6*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	sprintf(BufferText1, "%02x ", rawmessage67A[6]);
	(void)TestGraphics_drawString(BufferText1, X, 7*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	sprintf(BufferText1, "%02x ", rawmessage67A[7]);
	(void)TestGraphics_drawString(BufferText1, X, 8*LINESIZEY_RIGHTSIDE-1, YELLOW, BLACK, 2);
	#undef X

    uint8_t messageIndex = rawmessage678[0];
    if (messageIndex>LAST_MESSAGE_INDEX) messageIndex = LAST_MESSAGE_INDEX;
    memcpy(BufferText1, strStatusMessages+SIZE_OF_ONE_MESSAGE*messageIndex, SIZE_OF_ONE_MESSAGE);
    BufferText1[SIZE_OF_ONE_MESSAGE]=0;
    (void)TestGraphics_drawString(BufferText1, 1, 205, YELLOW, BLACK, 4);

    if ((nNumberOfReceivedMessages & 0x08)) {
  	  ILI9341_DrawRectangle(310, 0, 4, 4, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(310, 0, 4, 4, BLACK);
    }
    if ((nNumberOfReceivedMessages & 0x04)) {
  	  ILI9341_DrawRectangle(310, 9, 4, 4, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(310, 9, 4, 4, BLACK);
    }
    if ((nNumberOfReceivedMessages & 0x02)) {
  	  ILI9341_DrawRectangle(310, 18, 4, 4, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(310, 18, 4, 4, BLACK);
    }

}


#define RGB_TO_TFT(r, g, b) (((r / 8) << 11) | ((g / 4) << 5) | (b / 8))

#define BACKGROUNDCOLOR RGB_TO_TFT(90, 90, 90)
#define BROWN RGB_TO_TFT(150, 60, 0)
#define MY_ORANGE RGB_TO_TFT(255, 170, 50)
#define TICK_COLOR TFT_WHITESMOKE

uint16_t getColorFromTable(uint8_t x) {
	if (x<10) return BLACK;
	if (x<20) return BROWN;
	if (x<30) return RED;
	if (x<40) return MY_ORANGE;
	if (x<50) return YELLOW;
	if (x<60) return GREEN;
	if (x<70) return BLUE;
	if (x<80) return PINK;
	if (x<90) return LIGHTGREY;
	return WHITE;
}


#ifdef JUST_AS_INSPIRATION
/* The Ioniq default page */
void showpage3(uint8_t blInit) {
    #define LINESIZEY 20
	if (blInit) {
		ILI9341_FillScreen(BLACK);
		//ILI9341_DrawText("loops", FONT3, 10, 0*LINESIZEY, GREENYELLOW, BLACK);
		//ILI9341_DrawText("rxCount", FONT3, 10, 1*LINESIZEY, GREENYELLOW, BLACK);
		ILI9341_DrawHollowRectangleCoord(0, 3, 150, 60, DARKCYAN);
		ILI9341_DrawText("12V Battery", FONT1, 10, 0*LINESIZEY, GREENYELLOW, BLACK);

		ILI9341_DrawText("kWh", FONT3, 10, 5*LINESIZEY, GREENYELLOW, BLACK);
		ILI9341_DrawText("Ah", FONT3, 10, 6*LINESIZEY, GREENYELLOW, BLACK);
		ILI9341_DrawText("U_CCS", FONT3, 10, 7*LINESIZEY, GREENYELLOW, BLACK);

		ILI9341_DrawText("BattTemp °C", FONT1, 180, 0, GREENYELLOW, BLACK);
		ILI9341_DrawText("Min", FONT3, 180, 18, GREENYELLOW, BLACK);
		ILI9341_DrawText("Max", FONT3, 250, 18, GREENYELLOW, BLACK);
		ILI9341_DrawHollowRectangleCoord(179, 0, 309, 3*LINESIZEY, DARKCYAN);

		ILI9341_DrawText("SOC %",    FONT3, 10, 170, GREENYELLOW, BLACK);
		ILI9341_DrawText("PBatt kW", FONT3, 250, 130, GREENYELLOW, BLACK);
	}

    sprintf(BufferText1, "%1.1f  ", ((float)socDisp_0p5)/2);
    (void)TestGraphics_drawString(BufferText1, 10, 185, GREENYELLOW, BLACK, 7);

    //ILI9341_DrawRectangle(250, 145, 60, 90, BLACK);
    sprintf(BufferText1, "%5.1f ", ((float)PBatt_W)/1000);
    // Font 7 is the 7-segment-font, but does not have a minus sign.
    //(void)TestGraphics_drawString(BufferText1, 170, 165, GREENYELLOW, BLACK, 7);
    //(void)TestGraphics_drawString(BufferText1, 140, 145, GREENYELLOW, BLACK, 6+64);
    (void)TestGraphics_drawString(BufferText1, 140, 145, GREENYELLOW, BLACK, 9);

#if (0)
    sprintf(BufferText1, "%d  ", nMainLoops);
    (void)TestGraphics_drawString(BufferText1, 100, 0*LINESIZEY, GREENYELLOW, BLACK, 2);

    sprintf(BufferText1, "%ld  ", nNumberOfReceivedMessages);
    (void)TestGraphics_drawString(BufferText1, 100, 1*LINESIZEY, GREENYELLOW, BLACK, 2);
#endif

    sprintf(BufferText1, "%6.3f ", ((float)PIntegral_Wh)/1000.0);
    (void)TestGraphics_drawString(BufferText1, 100, 5*LINESIZEY, GREENYELLOW, BLACK, 2);
    sprintf(BufferText1, "%5.2f ", ((float)IIntegral_0Ah01)/100.0);
    (void)TestGraphics_drawString(BufferText1, 100, 6*LINESIZEY, GREENYELLOW, BLACK, 2);

    if (uCcsInlet_V<1000) {
      sprintf(BufferText1, "%5.1f ", uCcsInlet_V);
    } else {
      sprintf(BufferText1, "-      ");
    }
    (void)TestGraphics_drawString(BufferText1, 100, 7*LINESIZEY, GREENYELLOW, BLACK, 2);



    sprintf(BufferText1, "%d  ", TBattMin_C);
    (void)TestGraphics_drawString(BufferText1, 180, 34, GREENYELLOW, BLACK, 4);

    sprintf(BufferText1, "%d  ", TBattMax_C);
    (void)TestGraphics_drawString(BufferText1, 250, 34, GREENYELLOW, BLACK, 4);

    /* 12V battery state */
    /*
    extern float   BAT11_BAT_SNSR_I;
    extern uint8_t BAT11_BAT_SOC;
    extern float   BAT11_BAT_SNSR_V;
    extern float   BAT11_BAT_SNSR_Temp;
    extern uint8_t BAT11_BAT_SOH;
    */
    sprintf(BufferText1, "%2.1fV %2.1fA ", BAT11_BAT_SNSR_V, BAT11_BAT_SNSR_I);
    (void)TestGraphics_drawString(BufferText1, 4, 10, GREENYELLOW, BLACK, 4);
    #ifdef SHOW_12V_SOC_AND_SOH
      /* SOC is reported with 255, SOH with 127%. So it does not make sense to show them. */
      sprintf(BufferText1, "%2.0f°C %d%% %d%% ", BAT11_BAT_SNSR_Temp, BAT11_BAT_SOC, BAT11_BAT_SOH);
      (void)TestGraphics_drawString(BufferText1, 4, 34, GREENYELLOW, BLACK, 4);
    #else
      sprintf(BufferText1, "%2.0f°C %d%% ", BAT11_BAT_SNSR_Temp, BAT11_BAT_SOC);
      (void)TestGraphics_drawString(BufferText1, 4, 34, GREENYELLOW, BLACK, 2);
    #endif

    if ((nNumberOfReceivedMessages & 0x08)) {
  	  ILI9341_DrawRectangle(315, 0, 2, 2, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(315, 0, 2, 2, BLACK);
    }
    if ((nNumberOfReceivedMessages & 0x04)) {
  	  ILI9341_DrawRectangle(315, 9, 2, 2, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(315, 9, 2, 2, BLACK);
    }
    if ((nNumberOfReceivedMessages & 0x02)) {
  	  ILI9341_DrawRectangle(315, 18, 2, 2, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(315, 18, 2, 2, BLACK);
    }

}
#endif

void task100ms(void) {
}


void TestGraphics_showPage(void) {
	nMainLoops++;

	if (nLastPage!=nCurrentPage) {
		/* page changed. Clear and prepare the static content. */
		if (nCurrentPage==1) showpage1(1);
		//if (nCurrentPage==2) showpage2(1);
		//if (nCurrentPage==3) showpage3(1);
		nLastPage = nCurrentPage;
	}
	if (nCurrentPage==1) showpage1(0);
	//if (nCurrentPage==2) showpage2(0);
	//if (nCurrentPage==3) showpage3(0);
	//counterPageSwitch++;
	//if (counterPageSwitch>30) {
	//	counterPageSwitch=0;
	//	nCurrentPage++;
	//	if (nCurrentPage>2) nCurrentPage = 1;
	//}
	uint32_t uptime_s;
	uptime_s = HAL_GetTick() / 1000; /* the uptime in seconds */
	nCurrentPage=1;
	uint32_t t;
	t = HAL_GetTick();
	if (t>=oldTime100ms+100) {
		oldTime100ms+=100;
		task100ms();
	}
}
