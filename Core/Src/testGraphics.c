
#include "ILI9341_STM32_Driver.h"
#include "ILI9341_GFX.h"
#include "Font7s.h"
#include "Font16.h"
#include "Font32.h"
#include "Font64.h"
#include "canbus.h"
#include <stdio.h>

extern uint32_t nNumberOfReceivedMessages;
extern uint32_t nNumberOfCanInterrupts;

#define COLOR_BUFFER_SIZE 4000 /* bytes for one character. Is twice the pixel count of one character. */
uint8_t myColorBuffer[COLOR_BUFFER_SIZE];
uint16_t colorBufferIndex;
static char BufferText[40];

uint8_t nCurrentPage, nLastPage;
uint16_t counterPageSwitch;
uint16_t nMainLoops;




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
    sprintf(BufferText, "%d  ", nMainLoops);
    (void)TestGraphics_drawString(BufferText, 100, 0*LINESIZEY, GREENYELLOW, BLACK, 4);
    //(void)TestGraphics_drawString(BufferText, 150, 130, GREENYELLOW, DARKCYAN, 6);
    //(void)TestGraphics_drawString(BufferText, 150, 190, YELLOW, BLUE, 7);

    sprintf(BufferText, "%ld  ", nNumberOfReceivedMessages);
    (void)TestGraphics_drawString(BufferText, 100, 1*LINESIZEY, GREENYELLOW, BLACK, 2);

    sprintf(BufferText, "%ld  ", canRxDataUptime);
    (void)TestGraphics_drawString(BufferText, 100, 2*LINESIZEY, GREENYELLOW, BLACK, 2);

    sprintf(BufferText, "%d  ", canRxCheckpoint);
    (void)TestGraphics_drawString(BufferText, 100, 3*LINESIZEY, GREENYELLOW, BLACK, 4);

    sprintf(BufferText, "%d  ", EVSEPresentVoltage);
    (void)TestGraphics_drawString(BufferText, 0, 170, GREENYELLOW, BLACK, 7);

    sprintf(BufferText, "%d  ", uCcsInlet_V);
    (void)TestGraphics_drawString(BufferText, 200, 170, GREENYELLOW, BLACK, 7);

    /* Temperatures */
    sprintf(BufferText, "%d  ", ((int16_t)temperatureChannel_1_M40)-40);
    (void)TestGraphics_drawString(BufferText, 240, 1*LINESIZEY, YELLOW, BLACK, 2);

    sprintf(BufferText, "%d  ", ((int16_t)temperatureChannel_2_M40)-40);
    (void)TestGraphics_drawString(BufferText, 240, 2*LINESIZEY, YELLOW, BLACK, 2);

    sprintf(BufferText, "%d  ", ((int16_t)temperatureChannel_3_M40)-40);
    (void)TestGraphics_drawString(BufferText, 240, 3*LINESIZEY, YELLOW, BLACK, 2);

    sprintf(BufferText, "%d  ", ((int16_t)temperatureCpu_M40)-40);
    (void)TestGraphics_drawString(BufferText, 240, 4*LINESIZEY, YELLOW, BLACK, 2);

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

void showpage2(uint8_t blInit) {
	if (blInit) {
		ILI9341_FillScreen(BLACK);
		  //TestGraphics_DrawChar('2', 0, 180, GREENYELLOW, DARKCYAN);
		  //TestGraphics_DrawChar('3', 80, 180, GREENYELLOW, DARKCYAN);
		  //TestGraphics_DrawChar('4', 160, 180, GREENYELLOW, DARKCYAN);
		  //(void)TestGraphics_drawString("12345", 0, 180, GREENYELLOW, DARKCYAN, 2);
		  //(void)TestGraphics_drawString("Hello", 150, 100, GREENYELLOW, DARKCYAN, 4);
		  //(void)TestGraphics_drawString("12", 150, 130, GREENYELLOW, DARKCYAN, 6);
		  //(void)TestGraphics_drawString("12345", 150, 190, GREENYELLOW, DARKCYAN, 7);

	}
    (void)TestGraphics_drawString("pedal %", 0, 0, YELLOW, BLACK, 2);
	sprintf(BufferText, "%d ", acceleratorPedal_prc);
    (void)TestGraphics_drawString(BufferText, 0, 20, YELLOW, BLACK, 6);

    (void)TestGraphics_drawString("power kW", 120, 0, YELLOW, BLACK, 2);
	sprintf(BufferText, "%3.1f", PBatt_W/1000.0);
	(void)TestGraphics_drawString(BufferText, 120, 20, YELLOW, BLACK, 6);

    (void)TestGraphics_drawString("km/h", 250, 0, YELLOW, BLACK, 2);
	sprintf(BufferText, "%d",wheelspeed_FL_kmh);
	(void)TestGraphics_drawString(BufferText, 250, 20, YELLOW, BLACK, 6);

    sprintf(BufferText, "%d  ", nMainLoops);
    (void)TestGraphics_drawString(BufferText, 150, 190, BLUE, BLACK, 7);

}

void TestGraphics_showPage(void) {
	nMainLoops++;
	if (nLastPage!=nCurrentPage) {
		/* page changed. Clear and prepare static content. */
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
	if (uptime_s>20) {
		if (blIoniqDetected) {
			nCurrentPage=2;
		} else {
			nCurrentPage=1;
		}
	}
}
