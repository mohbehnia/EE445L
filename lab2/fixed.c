// filename ******** fixed.c ************** 
// possible header file for Lab 1 Spring 2018
// feel free to change the specific syntax of your system
// Mohammad Behnia, Dang Phuc
// 1/23/2018


/****************ST7735_sDecOut2***************
 converts fixed point number to LCD
 format signed 32-bit with resolution 0.01
 range -99.99 to +99.99
 Inputs:  signed 32-bit integer part of fixed-point number
 Outputs: none
 send exactly 6 characters to the LCD 
Parameter LCD display
 12345    " **.**"
  2345    " 23.45"  
 -8100    "-81.00"
  -102    " -1.02" 
    31    "  0.31" 
-12345    "-**.**"
 */ 
 #include "fixed.h"
void ST7735_sDecOut2(int32_t n) {
	uint8_t error[] = {' ', '*', '*', '.', '*', '*', '\0'};
	uint8_t msg[7]; 
	int is_neg = 0;
	if(n < 0) {
		is_neg = 1;
		error[0] = '-';
	}
	if((n > 9999) || (n < -9999)) {
		ST7735_OutString(error);
		return;
	}
	int32_t temp = n;
	if(n < 0) { 
		msg[0] = '-';
		temp = -n;
	}
	else msg[0] = ' ';
	uint8_t thousands = temp/1000;
	msg[1] = thousands + '0';
	temp %= 1000;
	uint8_t hundreds = temp/100;
	if(thousands == 0) {
		msg[0] = ' ';
		if(is_neg) msg[1] = '-';
		else msg[1] = ' ';
	}
	msg[2] = hundreds + '0';
	temp %= 100;
	uint8_t tens = temp/10;
	msg[3] = '.'; 
	msg[4] = tens + '0';
	temp %= 10;
	uint8_t ones = temp;
	msg[5] = ones + '0';
	msg[6] = '\0';
	ST7735_OutString(msg);
	return;
}


/**************ST7735_uBinOut6***************
 unsigned 32-bit binary fixed-point with a resolution of 1/64. 
 The full-scale range is from 0 to 999.99. 
 If the integer part is larger than 63999, it signifies an error. 
 The ST7735_uBinOut6 function takes an unsigned 32-bit integer part 
 of the binary fixed-point number and outputs the fixed-point value on the LCD
 Inputs:  unsigned 32-bit integer part of binary fixed-point number
 Outputs: none
 send exactly 6 characters to the LCD 
Parameter LCD display
     0	  "  0.00"
     1	  "  0.01"
    16    "  0.25"
    25	  "  0.39"
   125	  "  1.95"
   128	  "  2.00"
  1250	  " 19.53"
  7500	  "117.19"
 63999	  "999.99"
 64000	  "***.**"
*/
void ST7735_uBinOut6(uint32_t n) {
	uint8_t error[] = {'*', '*', '*', '.', '*', '*', '\0'};
	if(n > 63999) {
		ST7735_OutString(error);
		return;
	}
	uint32_t temp = (100*n+32)>>6;
	uint8_t msg[7];
	uint8_t tenthousands = temp/10000;
	if(tenthousands != 0) msg[0] = tenthousands + '0';
	else msg[0] = ' ';
	temp %= 10000;
	uint8_t thousands = temp/1000;
	if(tenthousands != 0 || thousands != 0) msg[1] = thousands + '0';
	else msg[1] = ' ';
	temp %= 1000;
	uint8_t hundreds = temp/100;
	msg[2] = hundreds + '0';
	temp %= 100;
	uint8_t tens = temp/10;
	msg[3] = '.'; 
	msg[4] = tens + '0';
	temp %= 10;
	uint8_t ones = temp;
	msg[5] = ones + '0';
	msg[6] = '\0';
	ST7735_OutString(msg);
	return;
	
}

/**************ST7735_XYplotInit***************
 Specify the X and Y axes for an x-y scatter plot
 Draw the title and clear the plot area
 Inputs:  title  ASCII string to label the plot, null-termination
          minX   smallest X data value allowed, resolution= 0.001
          maxX   largest X data value allowed, resolution= 0.001
          minY   smallest Y data value allowed, resolution= 0.001
          maxY   largest Y data value allowed, resolution= 0.001
 Outputs: none
 assumes minX < maxX, and miny < maxY
*/

static int32_t min_X, max_X, min_Y, max_Y;
  // X goes from 0 to 127
  // j goes from 159 to 32
  // y=Ymax maps to j=32
  // y=Ymin maps to j=159
static int32_t range_Y, range_X;

void ST7735_XYplotInit(char *title, int32_t minX, int32_t maxX, int32_t minY, int32_t maxY) {
	PLL_Init();
  ST7735_InitR(INITR_REDTAB);
  ST7735_OutString(title);
	ST7735_PlotClear(minY,maxY);
	min_X = minX;
	max_X = maxX;
	min_Y = minY;
	max_Y = maxY;
	range_X = maxX-minX;
	range_Y = maxY-minY;
}

/**************ST7735_XYplot***************
 Plot an array of (x,y) data
 Inputs:  num    number of data points in the two arrays
          bufX   array of 32-bit fixed-point data, resolution= 0.001
          bufY   array of 32-bit fixed-point data, resolution= 0.001
 Outputs: none
 assumes ST7735_XYplotInit has been previously called
 neglect any points outside the minX maxY minY maxY bounds
*/
void ST7735_XYplot(uint32_t num, int32_t bufX[], int32_t bufY[]) {
	uint32_t j;
	for(j=0; j<num; j++) {
		if(bufX[j] < min_X || bufX[j] > max_X) break;
		if(bufY[j] < min_Y || bufY[j] > max_Y) break;
		ST7735_DrawPixel(127*(bufX[j]-min_X)/range_X, 159 - (127*(bufY[j]-min_Y)/range_Y), ST7735_BLUE);
		//ST7735_PlotPoints(32+127*(bufY[j]-min_Y)/range_Y, 32+127*(bufY[j+1]-min_Y)/range_Y);
    //ST7735_PlotNext(); 
		//j++
	}
}



