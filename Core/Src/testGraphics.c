
#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"
#include "Font7s.h"
#include "Font16.h"
#include "Font32.h"
#include "Font64.h"
#include "canbus.h"
#include <stdio.h>
#include <string.h>

extern uint32_t nNumberOfReceivedMessages;
extern uint32_t nNumberOfCanInterrupts;

#define COLOR_BUFFER_SIZE 4000 /* bytes for one character. Is twice the pixel count of one character. */
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
      charBitmapPtr = chrtbl_f64[(uint8_t)ch];
      width = widtbl_f64[(uint8_t)ch];
      height = chr_hgt_f64;
      gap = -3;
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
	if (blInit) {
		ILI9341_FillScreen(BLACK);
		  ILI9341_DrawText("loops", FONT3, 10, 0*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("rxCount", FONT3, 10, 1*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("rxUpTime", FONT3, 10, 2*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("checkpoint", FONT3, 10, 3*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("EVSEPresentV", FONT2, 0, 150, GREENYELLOW, BLACK);
		  ILI9341_DrawText("uCcsInlet_V", FONT3, 200, 150, GREENYELLOW, BLACK);

		  ILI9341_DrawText("Temperatures [celsius]", FONT1, 180, 0*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("T1", FONT3, 180, 1*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("T2", FONT3, 180, 2*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("T3", FONT3, 180, 3*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawText("CPU", FONT3, 180, 4*LINESIZEY, GREENYELLOW, BLACK);
		  ILI9341_DrawHollowRectangleCoord(179, 0, 309, 5*LINESIZEY, DARKCYAN);
	}
    sprintf(BufferText1, "%d  ", nMainLoops);
    (void)TestGraphics_drawString(BufferText1, 100, 0*LINESIZEY, GREENYELLOW, BLACK, 4);
    //(void)TestGraphics_drawString(BufferText, 150, 130, GREENYELLOW, DARKCYAN, 6);
    //(void)TestGraphics_drawString(BufferText, 150, 190, YELLOW, BLUE, 7);

    sprintf(BufferText1, "%ld  ", nNumberOfReceivedMessages);
    (void)TestGraphics_drawString(BufferText1, 100, 1*LINESIZEY, GREENYELLOW, BLACK, 2);

    sprintf(BufferText1, "%ld  ", canRxDataUptime);
    (void)TestGraphics_drawString(BufferText1, 100, 2*LINESIZEY, GREENYELLOW, BLACK, 2);

    sprintf(BufferText1, "%d  ", canRxCheckpoint);
    (void)TestGraphics_drawString(BufferText1, 100, 3*LINESIZEY, GREENYELLOW, BLACK, 4);

    sprintf(BufferText1, "%d  ", EVSEPresentVoltage);
    (void)TestGraphics_drawString(BufferText1, 0, 170, GREENYELLOW, BLACK, 7);

    sprintf(BufferText1, "%d  ", uCcsInlet_V);
    (void)TestGraphics_drawString(BufferText1, 200, 170, GREENYELLOW, BLACK, 7);

    /* Temperatures */
    sprintf(BufferText1, "%d  ", ((int16_t)temperatureChannel_1_M40)-40);
    (void)TestGraphics_drawString(BufferText1, 240, 1*LINESIZEY, YELLOW, BLACK, 2);

    sprintf(BufferText1, "%d  ", ((int16_t)temperatureChannel_2_M40)-40);
    (void)TestGraphics_drawString(BufferText1, 240, 2*LINESIZEY, YELLOW, BLACK, 2);

    sprintf(BufferText1, "%d  ", ((int16_t)temperatureChannel_3_M40)-40);
    (void)TestGraphics_drawString(BufferText1, 240, 3*LINESIZEY, YELLOW, BLACK, 2);

    sprintf(BufferText1, "%d  ", ((int16_t)temperatureCpu_M40)-40);
    (void)TestGraphics_drawString(BufferText1, 240, 4*LINESIZEY, YELLOW, BLACK, 2);

    if ((nNumberOfReceivedMessages & 0x08)) {
  	  ILI9341_DrawRectangle(310, 0, 5, 5, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(310, 0, 5, 5, BLACK);
    }
    if ((nNumberOfReceivedMessages & 0x04)) {
  	  ILI9341_DrawRectangle(310, 9, 5, 5, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(310, 9, 5, 5, BLACK);
    }
    if ((nNumberOfReceivedMessages & 0x02)) {
  	  ILI9341_DrawRectangle(310, 18, 5, 5, GREENYELLOW);
    } else {
  	  ILI9341_DrawRectangle(310, 18, 5, 5, BLACK);
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
		nLastPage = nCurrentPage;
	}
	if (nCurrentPage==1) showpage1(0);
	if (nCurrentPage==2) showpage2(0);
	counterPageSwitch++;
	if (counterPageSwitch>30) {
		counterPageSwitch=0;
		nCurrentPage++;
		if (nCurrentPage>2) nCurrentPage = 1;
	}
	uint32_t uptime_s;
	uptime_s = HAL_GetTick() / 1000; /* the uptime in seconds */
	if (uptime_s>3) {
		if (blIoniqDetected) {
			nCurrentPage=2;
		} else {
			nCurrentPage=1;
		}
	}
}
