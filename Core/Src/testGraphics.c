
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
#include "serialComm.h"

#define USE_SIZE_2
#define USE_SIZE_4
//#define USE_SIZE_6
//#define USE_SIZE_7
#define USE_SIZE_9

/* fixed-size-font: e.g. this: https://github.com/idispatch/raster-fonts/blob/master/font-9x16.c */

extern uint32_t nNumberOfReceivedMessages;
extern uint32_t nNumberOfCanInterrupts;



extern uint8_t timeoutcounter_595;
extern int32_t PIntegral_Wh;
extern int32_t IIntegral_0Ah01;
extern float uCcsInlet_V;

uint32_t oldTime100ms;

#define COLOR_BUFFER_SIZE 6000 /* bytes for one character. Is twice the pixel count of one character. */
uint8_t myColorBuffer[COLOR_BUFFER_SIZE];
uint16_t colorBufferIndex;
static char BufferText1[40];
static char BufferText2[41];

uint8_t nCurrentPage, nLastPage;
uint16_t counterPageSwitch;
uint16_t nMainLoops;
float wheelspeed_FL_meterPerSecond;
float force_N;

int16_t simulatedPedal;
int16_t simulatedForce_N;
int16_t simulatedRandom;


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
#ifdef USE_SIZE_2
    if (size == 2) {
      charBitmapPtr = chrtbl_f16[(uint8_t)ch];
      width = widtbl_f16[(uint8_t)ch];
      height = chr_hgt_f16;
      gap = 1;
    }
#endif
#ifdef USE_SIZE_4
    if (size == 4) {
      charBitmapPtr = chrtbl_f32[(uint8_t)ch];
      width = widtbl_f32[(uint8_t)ch];
      height = chr_hgt_f32;
      gap = -3;
    }
#endif
#ifdef USE_SIZE_6
   if (size == 6) {
      charBitmapPtr = chrtbl_f64[(uint8_t)ch];
      width = widtbl_f64[(uint8_t)ch];
      height = chr_hgt_f64;
      gap = -3;
   }
#endif
#ifdef USE_SIZE_7
    if (size == 7) {
      charBitmapPtr = chrtbl_f7s[(uint8_t)ch];
      width = widtbl_f7s[(uint8_t)ch];
      height = chr_hgt_f7s;
      gap = 2;
    }
#endif
#ifdef USE_SIZE_9
    if (size == 9) {
    	return drawChar12x16(ch, X, Y, color, bgcolor);
    }
#endif
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
    #define LINESIZEY 18
	if (blInit) {
		ILI9341_FillScreen(BLACK);
		ILI9341_DrawHLine(0, 3*LINESIZEY-6, 318, DARKCYAN);
		ILI9341_DrawHLine(0, 3*LINESIZEY-5, 318, DARKCYAN);
		ILI9341_DrawHLine(0, 3*LINESIZEY-4, 318, DARKCYAN);
		ILI9341_DrawHLine(0, 4*LINESIZEY+8, 318, DARKCYAN);
		ILI9341_DrawHLine(0, 4*LINESIZEY+9, 318, DARKCYAN);
		ILI9341_DrawHLine(0, 10*LINESIZEY+0, 318, DARKCYAN);
		ILI9341_DrawHLine(0, 10*LINESIZEY+1, 318, DARKCYAN);
		ILI9341_DrawHLine(0, 10*LINESIZEY+2, 318, DARKCYAN);
		(void)TestGraphics_drawString("V",      90, 3*LINESIZEY+6, GREENYELLOW, BLACK, 2);
		(void)TestGraphics_drawString("A",     280, 3*LINESIZEY+6, GREENYELLOW, BLACK, 2);
		(void)TestGraphics_drawString("W",     260, 9*LINESIZEY, GREENYELLOW, BLACK, 2);
		(void)TestGraphics_drawString("Ah",     80, 10*LINESIZEY+4, GREENYELLOW, BLACK, 2);
		(void)TestGraphics_drawString("Wh",    80, 11*LINESIZEY+4, GREENYELLOW, BLACK, 2);
		(void)TestGraphics_drawString("s",      80, 12*LINESIZEY+4, GREENYELLOW, BLACK, 2);
	}
    sprintf(BufferText1, "%d  ", nMainLoops);
    (void)TestGraphics_drawString(BufferText1, 1, 0*LINESIZEY, GREENYELLOW, BLACK, 2);
    sprintf(BufferText1, "%ld  ", nUartRxCallbacks);
    (void)TestGraphics_drawString(BufferText1, 110, 0*LINESIZEY, GREENYELLOW, BLACK, 2);
    sprintf(BufferText1, "%ld  ", nUartRxCounterNewline);
    (void)TestGraphics_drawString(BufferText1, 220, 0*LINESIZEY, GREENYELLOW, BLACK, 2);

    (void)TestGraphics_drawString(strUartVoltage,   1, 3*LINESIZEY, GREENYELLOW, BLACK, 4);
    (void)TestGraphics_drawString(strUartCurrent, 200, 3*LINESIZEY, GREENYELLOW, BLACK, 4);
    (void)TestGraphics_drawString(strUartPower,     1, 5*LINESIZEY, GREENYELLOW, BLACK, 9);
    (void)TestGraphics_drawString(strUartCharge,    1, 10*LINESIZEY+4, GREENYELLOW, BLACK, 2);
    (void)TestGraphics_drawString(strUartEnergy,    1, 11*LINESIZEY+4, GREENYELLOW, BLACK, 2);
    (void)TestGraphics_drawString(strUartTime,      1, 12*LINESIZEY+4, GREENYELLOW, BLACK, 2);
	#undef LINESIZEY
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

#define diagramX0 2
#define diagramY0 120
#define diagramSizeX 300
#define diagramSizeY 200

#define MAX_FORCE_N 4000.0 /* Newton for maximum diagram value. 8000N is a realistic value.
                              To have better resolution, we look only to the lower 4000N. */

void diagramTest(void) {
  uint16_t diagColor;
  int16_t yPixel;
  int16_t pedalForDiagram;
  int16_t i;

  //#define USE_SIMULATION
  #ifdef USE_SIMULATION
    simulatedPedal++;
    if (simulatedPedal>=100) {
  	    simulatedPedal=0;
  	    simulatedRandom+=150;
  	    if (simulatedRandom>600) simulatedRandom=-1000;
    }
    simulatedForce_N=5000.0 * (simulatedPedal-20) / 100;
    diagColor = getColorFromTable(simulatedPedal);
    force_N = simulatedForce_N+simulatedRandom;
    pedalForDiagram = simulatedPedal;
    acceleratorPedal_prc = simulatedPedal;
    PBatt_W = 87654;
    wheelspeed_FL_kmh = 123;
  #else
    /* use the real world values from CAN */
    diagColor = getColorFromTable(wheelspeed_FL_kmh); /* map driving speed to color */
    /* use force_N calculated from battery power and speed */
    pedalForDiagram = acceleratorPedal_prc;
  #endif

  ILI9341_DrawHLine(diagramX0, diagramY0, diagramSizeX, DARKGREY); /* x axis */
  ILI9341_DrawVLine(diagramX0, diagramY0-100, diagramSizeY, DARKGREY); /* y axis */
  for (i=10; i<100; i+=10) {
	  ILI9341_DrawPixel(diagramX0+i*diagramSizeX/100, diagramY0+1, TICK_COLOR); /* ticks each 10% pedal */
	  ILI9341_DrawPixel(diagramX0+i*diagramSizeX/100, diagramY0+2, TICK_COLOR); /* ticks each 10% pedal */
  }
  ILI9341_DrawVLine(diagramX0+diagramSizeX/2, diagramY0, 6, TICK_COLOR); /* tick 50% pedal */
  ILI9341_DrawVLine(diagramX0+diagramSizeX, diagramY0, 6, TICK_COLOR); /* tick 100% pedal */
  ILI9341_DrawHLine(diagramX0, diagramY0-diagramSizeY/2, 6, TICK_COLOR); /* tick full force */
  ILI9341_DrawHLine(diagramX0, diagramY0+diagramSizeY/2, 6, TICK_COLOR); /* tick full negative force */
  ILI9341_DrawPixel(diagramX0+3*pedalForDiagram-1, diagramY0, TFT_LIGHTPINK); /* pedal marker */
  ILI9341_DrawPixel(diagramX0+3*pedalForDiagram, diagramY0, TFT_LIGHTPINK); /* pedal marker */
  ILI9341_DrawPixel(diagramX0+3*pedalForDiagram+1, diagramY0, TFT_LIGHTPINK); /* pedal marker */

  yPixel = diagramY0 - force_N * (float)(diagramSizeY/2) / MAX_FORCE_N;
  ILI9341_DrawPixel(diagramX0+3*pedalForDiagram, yPixel, diagColor);
  ILI9341_DrawPixel(diagramX0+3*pedalForDiagram+1, yPixel, diagColor);
  ILI9341_DrawPixel(diagramX0+3*pedalForDiagram+2, yPixel, diagColor);
  #ifdef USE_DOUBLE_Y_PIXEL
    ILI9341_DrawPixel(diagramX0+3*pedalForDiagram, yPixel-1, diagColor);
    ILI9341_DrawPixel(diagramX0+3*pedalForDiagram+1, yPixel-1, diagColor);
    ILI9341_DrawPixel(diagramX0+3*pedalForDiagram+2, yPixel-1, diagColor);
  #endif

}

void showpage2(uint8_t blInit) {
	if (blInit) {
		ILI9341_FillScreen(BACKGROUNDCOLOR);
	    (void)TestGraphics_drawString("100%", 280, diagramY0+13, YELLOW, BACKGROUNDCOLOR, 2);
	    (void)TestGraphics_drawString("pedal", 280, diagramY0+28, YELLOW, BACKGROUNDCOLOR, 2);
	    (void)TestGraphics_drawString("force", 5, diagramY0-diagramSizeY/2-20, YELLOW, BACKGROUNDCOLOR, 2);
	    sprintf(BufferText1, "%1.0fN", MAX_FORCE_N);
	    (void)TestGraphics_drawString(BufferText1, 12, diagramY0-diagramSizeY/2-7, YELLOW, BACKGROUNDCOLOR, 2);
	    sprintf(BufferText1, "%1.0fN", -MAX_FORCE_N);
	    (void)TestGraphics_drawString(BufferText1, 12, diagramY0+diagramSizeY/2-7, YELLOW, BACKGROUNDCOLOR, 2);

		  //TestGraphics_DrawChar('2', 0, 180, GREENYELLOW, DARKCYAN);
		  //TestGraphics_DrawChar('3', 80, 180, GREENYELLOW, DARKCYAN);
		  //TestGraphics_DrawChar('4', 160, 180, GREENYELLOW, DARKCYAN);
		  //(void)TestGraphics_drawString("12345", 0, 180, GREENYELLOW, DARKCYAN, 2);
		  //(void)TestGraphics_drawString("Hello", 150, 100, GREENYELLOW, DARKCYAN, 4);
		  //(void)TestGraphics_drawString("12", 150, 130, GREENYELLOW, DARKCYAN, 6);
		  //(void)TestGraphics_drawString("12345", 150, 190, GREENYELLOW, DARKCYAN, 7);

	}
    #define pedalTextX 105
    #define pedalTextY 198
    (void)TestGraphics_drawString("pedal %", pedalTextX, pedalTextY, YELLOW, BACKGROUNDCOLOR, 2);
	sprintf(BufferText1, "%d  ", acceleratorPedal_prc);
	if (strlen(BufferText1)<3) {
		sprintf(BufferText2, " %s", BufferText1);
	} else {
		sprintf(BufferText2, "%s", BufferText1);
	}
    (void)TestGraphics_drawString(BufferText2, pedalTextX, pedalTextY+15, YELLOW, BACKGROUNDCOLOR, 4);

    (void)TestGraphics_drawString("power kW", 180, 198, YELLOW, BACKGROUNDCOLOR, 2);
	sprintf(BufferText1, "%3.1f  ", PBatt_W/1000.0);
	(void)TestGraphics_drawString(BufferText1, 180, 213, YELLOW, BACKGROUNDCOLOR, 4);

    (void)TestGraphics_drawString("km/h", 263, 198, YELLOW, BACKGROUNDCOLOR, 2);
	sprintf(BufferText1, "%d  ",wheelspeed_FL_kmh);
	(void)TestGraphics_drawString(BufferText1, 263, 213, YELLOW, BACKGROUNDCOLOR, 4);

    //sprintf(BufferText1, "%d  ", nMainLoops);
    //(void)TestGraphics_drawString(BufferText1, 150, 190, BLUE, BACKGROUNDCOLOR, 7);

    diagramTest();

}


/* The Ioniq default page */
void showpage3(uint8_t blInit) {
    #define LINESIZEY 20
	if (blInit) {
		ILI9341_FillScreen(BLACK);
		//ILI9341_DrawText("loops", FONT3, 10, 0*LINESIZEY, GREENYELLOW, BLACK);
		//ILI9341_DrawText("rxCount", FONT3, 10, 1*LINESIZEY, GREENYELLOW, BLACK);
		ILI9341_DrawHollowRectangleCoord(0, 3, 150, 55, DARKCYAN);
		ILI9341_DrawText("12V Battery", FONT1, 10, 0*LINESIZEY, GREENYELLOW, BLACK);

		ILI9341_DrawText("kWh", FONT3, 10, 3*LINESIZEY, GREENYELLOW, BLACK);
		ILI9341_DrawText("Ah", FONT3, 10, 4*LINESIZEY, GREENYELLOW, BLACK);
		ILI9341_DrawText("U_CCS", FONT3, 10, 5*LINESIZEY, GREENYELLOW, BLACK);

		ILI9341_DrawText("BattTemp °C", FONT1, 180, 0, GREENYELLOW, BLACK);
		ILI9341_DrawText("Min", FONT3, 180, 18, GREENYELLOW, BLACK);
		ILI9341_DrawText("Max", FONT3, 250, 18, GREENYELLOW, BLACK);
		ILI9341_DrawHollowRectangleCoord(179, 0, 309, 3*LINESIZEY, DARKCYAN);

		ILI9341_DrawText("SOC %",    FONT3, 10, 150, GREENYELLOW, BLACK);
		ILI9341_DrawText("PBatt kW", FONT3, 140, 130, GREENYELLOW, BLACK);
	}

    sprintf(BufferText1, "%1.1f  ", ((float)socDisp_0p5)/2);
    (void)TestGraphics_drawString(BufferText1, 10, 165, GREENYELLOW, BLACK, 7);

    //ILI9341_DrawRectangle(250, 145, 60, 90, BLACK);
    sprintf(BufferText1, "%5.1f ", ((float)PBatt_W)/1000);
    // Font 7 is the 7-segment-font, but does not have a minus sign.
    //(void)TestGraphics_drawString(BufferText1, 170, 165, GREENYELLOW, BLACK, 7);
    //(void)TestGraphics_drawString(BufferText1, 140, 145, GREENYELLOW, BLACK, 6+64);
    (void)TestGraphics_drawString(BufferText1, 140, 145, GREENYELLOW, BLACK, 9);


    sprintf(BufferText1, "%d  ", nMainLoops);
    (void)TestGraphics_drawString(BufferText1, 130, 0*LINESIZEY, GREENYELLOW, BLACK, 2);
    //(void)TestGraphics_drawString(BufferText, 150, 130, GREENYELLOW, DARKCYAN, 6);
    //(void)TestGraphics_drawString(BufferText, 150, 190, YELLOW, BLUE, 7);

    //sprintf(BufferText1, "%ld  ", nNumberOfReceivedMessages);
    sprintf(BufferText1, "%ld  ", nUartRxCallbacks);
    (void)TestGraphics_drawString(BufferText1, 130, 1*LINESIZEY, GREENYELLOW, BLACK, 2);
    sprintf(BufferText1, "%ld  ", nUartRxCounterNewline);
    (void)TestGraphics_drawString(BufferText1, 130, 2*LINESIZEY, GREENYELLOW, BLACK, 2);


    sprintf(BufferText1, "%6.3f ", ((float)PIntegral_Wh)/1000.0);
    (void)TestGraphics_drawString(BufferText1, 100, 3*LINESIZEY, GREENYELLOW, BLACK, 2);
    sprintf(BufferText1, "%5.2f ", ((float)IIntegral_0Ah01)/100.0);
    (void)TestGraphics_drawString(BufferText1, 100, 4*LINESIZEY, GREENYELLOW, BLACK, 2);

    sprintf(BufferText1, "%5.1f ", uCcsInlet_V);
    (void)TestGraphics_drawString(BufferText1, 100, 5*LINESIZEY, GREENYELLOW, BLACK, 2);



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
    sprintf(BufferText1, "%1.0fC %d%% %d%% ", BAT11_BAT_SNSR_Temp, BAT11_BAT_SOC, BAT11_BAT_SOH);
    (void)TestGraphics_drawString(BufferText1, 4, 35, GREENYELLOW, BLACK, 2);


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


void task100ms(void) {
	if (timeoutcounter_595>0) {
		timeoutcounter_595--;
		if (timeoutcounter_595==0) {
			/* timeout of the BMS. It is time to store the accumulated data. */
			flashhandler_saveToFlash();
		}
	}
}



void TestGraphics_showPage(void) {
	nMainLoops++;
	wheelspeed_FL_meterPerSecond = wheelspeed_FL_kmh;
	wheelspeed_FL_meterPerSecond /= 3.6;
	if (wheelspeed_FL_meterPerSecond>0.5) {
	  float P_motor_W;
	  P_motor_W = PBatt_W - 300; /* assume 300W consumption of electronics */
	  force_N = P_motor_W / wheelspeed_FL_meterPerSecond;
	} else {
	  /* in case we have no speed, we assume no significant force */
	  force_N = 0;
	}
	/* limit the calculated force to be inside the diagram range: */
	if (force_N>MAX_FORCE_N) force_N = MAX_FORCE_N;
	if (force_N<-MAX_FORCE_N) force_N = -MAX_FORCE_N;

	if (nLastPage!=nCurrentPage) {
		/* page changed. Clear and prepare the static content. */
		if (nCurrentPage==1) showpage1(1);
		if (nCurrentPage==2) showpage2(1);
		if (nCurrentPage==3) showpage3(1);
		nLastPage = nCurrentPage;
	}
	if (nCurrentPage==1) showpage1(0);
	if (nCurrentPage==2) showpage2(0);
	if (nCurrentPage==3) showpage3(0);
	//counterPageSwitch++;
	//if (counterPageSwitch>30) {
	//	counterPageSwitch=0;
	//	nCurrentPage++;
	//	if (nCurrentPage>2) nCurrentPage = 1;
	//}
	uint32_t uptime_s;
	uptime_s = HAL_GetTick() / 1000; /* the uptime in seconds */
	if (uptime_s>1) {
			nCurrentPage=1;
	}
	uint32_t t;
	t = HAL_GetTick();
	if (t>=oldTime100ms+100) {
		oldTime100ms+=100;
		task100ms();
	}
}
