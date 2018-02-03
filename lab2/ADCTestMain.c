// ADCTestMain.c
// Runs on TM4C123
// This program periodically samples ADC channel 0 and stores the
// result to a global variable that can be accessed with the JTAG
// debugger and viewed with the variable watch feature.
// Daniel Valvano
// September 5, 2015

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2015

 Copyright 2015 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// center of X-ohm potentiometer connected to PE3/AIN0
// bottom of X-ohm potentiometer connected to ground
// top of X-ohm potentiometer connected to +3.3V 
#include <stdint.h>
#include "ADCSWTrigger.h"
#include "tm4c123gh6pm.h"
#include "PLL.h"
#include "ST7735.h"


static int32_t min_X, max_X, min_Y, max_Y;
  // X goes from 0 to 127
  // j goes from 159 to 32
  // y=Ymax maps to j=32
  // y=Ymin maps to j=159
static int32_t range_Y, range_X;

void ST7735_XYplotInit(char *title, int32_t minX, int32_t maxX, int32_t minY, int32_t maxY) {
	PLL_Init(4);
  ST7735_InitR(INITR_REDTAB);
  ST7735_OutString(title);
	ST7735_OutUDec(minX);
	ST7735_OutUDec(maxX);
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
		int x = 127*(bufX[j]-min_X)/range_X;
		int y = 159 - (127*(bufY[j]-min_Y)/range_Y);
		for(int i = 159; i >= y; i--) {
			ST7735_DrawPixel(x, i, ST7735_BLUE);
		}
		//ST7735_PlotPoints(32+127*(bufY[j]-min_Y)/range_Y, 32+127*(bufY[j+1]-min_Y)/range_Y);
    //ST7735_PlotNext(); 
		//j++
	}
}





#define PF2             (*((volatile uint32_t *)0x40025010))
#define PF1             (*((volatile uint32_t *)0x40025008))
void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode

unsigned long Time[1000];		//time array for timer 1
uint32_t ADC_DATA[1000];		//ADC data array to take in ADC value at each interrupt
int array_counter = 0;			//Array counter to increment two arrays
volatile uint32_t ADCvalue;
unsigned long difference[999];	//difference between the timers, should be .01 seconds ideal
uint32_t jitter = 0;	//max - min of the time difference
uint32_t occurence[1000];

// This debug function initializes Timer0A to request interrupts
// at a 100 Hz frequency.  It is similar to FreqMeasure.c.
void Timer0A_Init100HzInt(void){
  volatile uint32_t delay;
  DisableInterrupts();
  // **** general initialization ****
  SYSCTL_RCGCTIMER_R |= 0x01;      // activate timer0
  delay = SYSCTL_RCGCTIMER_R;      // allow time to finish activating
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = 0;                // configure for 32-bit timer mode
  // **** timer0A initialization ****
                                   // configure for periodic mode
  TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
  TIMER0_TAILR_R = 799999;         // start value for 100 Hz interrupts
  TIMER0_IMR_R |= TIMER_IMR_TATOIM;// enable timeout (rollover) interrupt
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// clear timer0A timeout flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 32-b, periodic, interrupts
  // **** interrupt initialization ****
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R = 1<<19;              // enable interrupt 19 in NVIC
}
void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;    // acknowledge timer0A timeout
  PF2 ^= 0x04;                   // profile
  PF2 ^= 0x04;                   // profile
  ADCvalue = ADC0_InSeq3();
	if(array_counter < 1000) {
		Time[array_counter] = TIMER1_TAR_R;
		ADC_DATA[array_counter] = ADCvalue;
		array_counter++;
	}
	PF2 ^= 0x04;										// profile
}
void Timer1_Init(uint32_t period){
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  //PeriodicTask = task;          // user function
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, default down-count settings
  TIMER1_TAILR_R = 0xFFFFFFFF;    // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_ICR_R = 0x00000001;    // 6) clear TIMER1A timeout flag
  //TIMER1_IMR_R = 0x00000001;    // 7) arm timeout interrupt
  //NVIC_PRI5_R = (NVIC_PRI5_R&0xFFFF00FF)|0x00008000; // 8) priority 4
// interrupts enabled in the main program after all devices initialized
// vector number 37, interrupt number 21
  //NVIC_EN0_R = 1<<21;           // 9) enable IRQ 21 in NVIC
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
}
	
void PMF_Graph() {
	uint32_t max;
	uint32_t min;
	int probmassfunc[1000];
	int x_value[1000];
	int num_x_values;
	int y_range = 0;
	min = ADC_DATA[0];
	max = ADC_DATA[0];
	for (int i = 1; i < 1000; i++) {
		if(max < ADC_DATA[i]) {
			max = ADC_DATA[i];
		}
	  if(min > ADC_DATA[i]) {
			min = ADC_DATA[i];
		}
	}
	num_x_values = max - min;
	for (int i = 0; i < 1000; i++) {
		probmassfunc[ADC_DATA[i] - min]++;
		if (probmassfunc[ADC_DATA[i] - min] > y_range) {
			y_range = probmassfunc[ADC_DATA[i] - min];
	}
}
	x_value[0] = min;
	for (int i = 1; i <= num_x_values; i++) {
		x_value[i] = x_value[i-1]+1;
	}

ST7735_XYplotInit("ADC_LAB", min, max, 0, y_range);
ST7735_XYplot(num_x_values, (int32_t*)x_value, (int32_t*)probmassfunc);
	}
int main(void){
	int temp;

	int data_counter = 0;
	uint32_t max = 0;
	uint32_t min = 0xFFFFFFFF;
  PLL_Init(Bus80MHz);                   // 80 MHz
  SYSCTL_RCGCGPIO_R |= 0x20;            // activate port F
  ADC0_InitSWTriggerSeq3_Ch9();         // allow time to finish activating
	Timer1_Init(0);
  Timer0A_Init100HzInt();               // set up Timer0A for 100 Hz interrupts
  GPIO_PORTF_DIR_R |= 0x06;             // make PF2, PF1 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x06;          // disable alt funct on PF2, PF1
  GPIO_PORTF_DEN_R |= 0x06;             // enable digital I/O on PF2, PF1
                                        // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF00F)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;               // disable analog functionality on PF
  PF2 = 0;                      // turn off LED
  EnableInterrupts();
  while(1){
    GPIO_PORTF_DATA_R ^= 0x02;  // toggles when running in main
		//PF1 ^= 0x02;
		if(array_counter >= 1000 && difference[0] == 0) {
		for (int i = 0; i < 999; i++) {
			//find the difference in time, calculate jitter
			difference[i] = Time[i] - Time[i+1];
			if(difference[i] > max) {
				max = difference[i];
			}
			if(difference[i] < min) {
					min = difference[i];
				}
			
		}
				jitter = max - min;
		for (int i = 0; i < 1000; i++) {
			for (int j = i+1; j < 1000; j ++ ) {
				if(ADC_DATA[j] < ADC_DATA[i]) {
					temp = ADC_DATA[i];	//treat min as a temp variable
					ADC_DATA[i] = ADC_DATA[j];
					ADC_DATA[j] = temp;
				}
			}
		}
		//DisableInterrupts();
		PMF_Graph();
		//EnableInterrupts();
		occurence[data_counter] = ADC_DATA[0];
		data_counter++;
			for (int i = 0; i < 1000; i ++) {
				if(ADC_DATA[i] == ADC_DATA[i+1]) {
					occurence[data_counter]++;
				} else{
					occurence[data_counter]++;
					data_counter++;
					occurence[data_counter] = ADC_DATA[i+1];
					data_counter++;
				}
			}
		}
	}
}


