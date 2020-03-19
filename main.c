#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "stm32f05xxx.h"
#include "adc.h"
/*
To be used with the "PrintADC2Inputs" folder and associated makefile
Will Chaba March 18, 2020

Connect X axis to Channel 8(white wire to pin 15)
Connect Y axis to Channel 9(yellow wire to pin 14)
input range:
X Axis: Center Point = ~1.6V
		Left Extreme = ~2.6V
		Right Extreme = 0V

Y Axis: Center Point = ~0.87-0.89V
		Up Extreme   = ~2.03-2.08V
		Down Extreme = ~0V
*/
void delay(int dly)
{
	while( dly--);
}

void main(void)
{
	float a[2]; //a[0] is x, a[1] is y
	int j[2];
	int dir;
	
	printf("ADC/SERIAL/CLOCK test.\r\n");
	
	initADC();
	
	RCC_AHBENR |= 0x00020000; // peripheral clock enable for port A
	GPIOA_MODER |= 0x00000001; // Make pin PA0 output
	while(1)
	{
		ADC_CHSELR = BIT8;          // Select Channel 8
		j[0]=readADC();
		a[0]=(j[0]*3.3)/0x1000;
		ADC_CHSELR = BIT9;          // Select Channel 9
		j[1]=readADC();
		a[1]=(j[1]*3.3)/0x1000;
		//Direction numbers are based on the position of the number in the number pad. 
		//8 is up, 2 is down, 4 left, 6 right etc	
		if(a[0]>1.7){
			if(a[1]>0.9)
				dir = 7;
			else if(a[1]<0.8)
				dir = 1;
			else
				dir = 4;
			}
		else if(a[0]<1.5){
			if(a[1]>0.9)
				dir =9;
			else if(a[1]<0.8)
				dir =3;
			else
				dir =6;
			}
		else
			if(a[1]>0.9)
				dir =8;
			else if(a[1]<0.8)
				dir =2;
			else
				dir =5;	
		
		printf("ADC[8]=0x%04x V=%fV, ADC[9]=0x%04x V=%fV, Direction: %d \r", j[0], a[0], j[1], a[1], dir);
		
		fflush(stdout);
		GPIOA_ODR |= BIT0; // PA0=1
		delay(500000);
		GPIOA_ODR &= ~(BIT0); // PA0=0
		delay(500000);
	}
}
