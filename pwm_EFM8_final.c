//  square.c: Uses timer 2 interrupt to generate a square wave in pin
//  P2.0 and a 75% duty cycle wave in pin P2.1
//  Copyright (c) 2010-2018 Jesus Calvino-Fraga
//  ~C51~

#include <stdio.h>
#include <stdlib.h>
#include <EFM8LB1.h>

// ~C51~  

#define SYSCLK 72000000L
#define BAUDRATE 115200L

#define LCD_RS P2_6
// #define LCD_RW Px_x // Not used in this code.  Connect to GND
#define LCD_E  P2_5
#define LCD_D4 P2_4
#define LCD_D5 P2_3
#define LCD_D6 P2_2
#define LCD_D7 P2_1
#define CHARS_PER_LINE 16

#define OUT0 P1_6		//motor 1
#define OUT1 P1_5

#define OUT2 P1_3		//motor 2
#define OUT3 P1_1

#define MODE_BUTTON P0_3
#define FWD_BUTTON P0_5
#define BCK_BUTTON P0_7
#define RGT_BUTTON P1_2
#define LFT_BUTTON P1_4
#define STOP_BUTTON P1_7


#define VDD 3.3035 // The measured value of VDD in volts

volatile unsigned char pwm_count0=0;
volatile unsigned char pwm_count1=0;

volatile unsigned int in0 = 50;
volatile unsigned int in1 = 50;

volatile unsigned int in2 = 50;
volatile unsigned int in3 = 50;

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN=0x80;       // enable VDD monitor
	RSTSRC=0x02|0x04;  // Enable reset on missing clock detector and VDD

	#if (SYSCLK == 48000000L)	
		SFRPAGE = 0x10;
		PFE0CN  = 0x10; // SYSCLK < 50 MHz.
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		SFRPAGE = 0x00;
	#endif
	
	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)	
		// Before setting clock to 48 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		// Before setting clock to 72 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif
	
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X00;
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	// Configure Uart 0
	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	SCON0 = 0x10;
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready

	// Initialize timer 2 for periodic interrupts
	TMR2CN0=0x00;   // Stop Timer2; Clear TF2;
	CKCON0|=0b_0001_0000; // Timer 2 uses the system clock
	TMR2RL=(0x10000L-(SYSCLK/10000L)); // Initialize reload value
	TMR2=0xffff;   // Set to reload immediately
	ET2=1;         // Enable Timer2 interrupts
	TR2=1;         // Start Timer2 (TMR2CN is bit addressable)

	EA=1; // Enable interrupts

  	
	return 0;
}

void InitADC (void)
{
	SFRPAGE = 0x00;
	ADC0CN1 = 0b_10_000_000; //14-bit,  Right justified no shifting applied, perform and Accumulate 1 conversion.
	ADC0CF0 = 0b_11111_0_00; // SYSCLK/32
	ADC0CF1 = 0b_0_0_011110; // Same as default for now
	ADC0CN0 = 0b_0_0_0_0_0_00_0; // Same as default for now
	ADC0CF2 = 0b_0_01_11111 ; // GND pin, Vref=VDD
	ADC0CN2 = 0b_0_000_0000;  // Same as default for now. ADC0 conversion initiated on write of 1 to ADBUSY.
	ADEN=1; // Enable ADC
}

// Uses Timer3 to delay <us> micro-seconds. 
void Timer3us(unsigned char us)
{
	unsigned char i;               // usec counter
	
	// The input for Timer 3 is selected as SYSCLK by setting T3ML (bit 6) of CKCON0:
	CKCON0|=0b_0100_0000;
	
	TMR3RL = (-(SYSCLK)/1000000L); // Set Timer3 to overflow in 1us.
	TMR3 = TMR3RL;                 // Initialize Timer3 for first overflow
	
	TMR3CN0 = 0x04;                 // Sart Timer3 and clear overflow flag
	for (i = 0; i < us; i++)       // Count <us> overflows
	{
		while (!(TMR3CN0 & 0x80));  // Wait for overflow
		TMR3CN0 &= ~(0x80);         // Clear overflow indicator
	}
	TMR3CN0 = 0 ;                   // Stop Timer3 and clear overflow flag
}

void waitms (unsigned int ms)
{
	unsigned int j;
	for(j=ms; j!=0; j--)
	{
		Timer3us(249);
		Timer3us(249);
		Timer3us(249);
		Timer3us(250);
	}
}

void LCD_pulse (void)
{
	LCD_E=1;
	Timer3us(40);
	LCD_E=0;
}

void LCD_byte (unsigned char x)
{
	// The accumulator in the C8051Fxxx is bit addressable!
	ACC=x; //Send high nible
	LCD_D7=ACC_7;
	LCD_D6=ACC_6;
	LCD_D5=ACC_5;
	LCD_D4=ACC_4;
	LCD_pulse();
	Timer3us(40);
	ACC=x; //Send low nible
	LCD_D7=ACC_3;
	LCD_D6=ACC_2;
	LCD_D5=ACC_1;
	LCD_D4=ACC_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS=1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS=0;
	LCD_byte(x);
	waitms(5);
}

void LCD_4BIT (void)
{
	LCD_E=0; // Resting state of LCD's enable is zero
	// LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode and then change to 4-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x32); // Change to 4-bit mode

	// Configure the LCD
	WriteCommand(0x28);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, bit clear)
{
	int j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}

int getsn (char * buff, int len)
{
	int j;
	char c;
	
	for(j=0; j<(len-1); j++)
	{
		c=getchar();
		if ( (c=='\n') || (c=='\r') )
		{
			buff[j]=0;
			return j;
		}
		else
		{
			buff[j]=c;
		}
	}
	buff[j]=0;
	return len;
}

void Timer2_ISR (void) interrupt 5
{
	TF2H = 0; // Clear Timer2 interrupt flag
	
	pwm_count0++;
	if(pwm_count0>100) pwm_count0=0;
	
	OUT0=pwm_count0>in0?0:1;
	OUT1=pwm_count0>in1?0:1;

	pwm_count1++;
	if(pwm_count1>100) pwm_count1=0;
	
	OUT2=pwm_count1>in2?0:1;
	OUT3=pwm_count1>in3?0:1;

}

void InitPinADC (unsigned char portno, unsigned char pinno)
 {
	unsigned char mask;
	
	mask=1<<pinno;

 	SFRPAGE = 0x20;
 	switch (portno)
 	{
 		case 0:
 			P0MDIN &= (~mask); // Set pin as analog input
 			P0SKIP |= mask; // Skip Crossbar decoding for this pin
 			break;
 			case 1:
 			P1MDIN &= (~mask); // Set pin as analog input
 			P1SKIP |= mask; // Skip Crossbar decoding for this pin
 			break;
 			case 2:
 			P2MDIN &= (~mask); // Set pin as analog input
 			P2SKIP |= mask; // Skip Crossbar decoding for this pin
 			break;
 			default:
 			break;
 		}
 		SFRPAGE = 0x00;
 	}

 unsigned int ADC_at_Pin(unsigned char pin)
 	{
 	ADC0MX = pin;   // Select input from pin
 	ADBUSY=1;       // Dummy conversion first to select new pin
 	while (ADBUSY); // Wait for dummy conversion to finish
 	ADBUSY = 1;     // Convert voltage at the pin
 	while (ADBUSY); // Wait for conversion to complete
 	return (ADC0);
 }

 float Volts_at_Pin(unsigned char pin)
 {
 	return ((ADC_at_Pin(pin)*VDD)/0b_0011_1111_1111_1111);
 }

// void printPython(void){
// 	while(1){
// 		printf("%f\n", Volts_at_Pin(QFP32_MUX_P1_5));
// 	}
// }

void main (void)
{
	char *buff;
	int state = 0;
	int previous_state = 0;
	int inrange = 1;

	InitPinADC(1, 0); // Configure P2.5 as analog input
	InitADC();
	LCD_4BIT();

	waitms(500); // Give PuTTY a chance to start.

	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	// printf("\rEnter 2 numbers between 0-100 (separated by a space): ");
	// scanf("%d %d\n", &in0,&in1);

	printf("\rEnter 4 spaced numbers (2 for right/left motors) between 0-100: ");
	scanf("%d %d %d %d\n", &in0,&in1,&in2,&in3);

//	while(1){
		while(1)
		{
			// printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
			// // printf("\rEnter 2 numbers between 0-100 (separated by a space): ");
			// // scanf("%d %d\n", &in0,&in1);

			// printf("\rEnter 4 spaced numbers (2 for right/left motors) between 0-100: ");
			// scanf("%d %d %d %d\n", &in0,&in1,&in2,&in3);

			if (in0<0 || in0>100 || in1<0 || in1>100 || in2<0 || in2>100 || in3<0 || in3>100) {
				in0 = 50;
				in1 = 50;
				in2 = 50;
				in3 = 50;
				inrange = 0;
				break;
			}

			if (STOP_BUTTON == 0) {
				break;
				} else if (FWD_BUTTON == 0) {
					in0 = 30;
					in1 = 70;
					in2 = 30;
					in3 = 70;
				} else if (BCK_BUTTON == 0) {
					in0 = 70;
					in1 = 30;
					in2 = 70;
					in3 = 30;
				} else if (RGT_BUTTON == 0) {
					in0 = 30;
					in1 = 70;
					in2 = 50;
					in3 = 50;
				} else if (LFT_BUTTON == 0) {
					in0 = 50;
					in1 = 50;
					in2 = 30;
					in3 = 70;
				}

			/****** LCD *******/
			if (MODE_BUTTON == 0) {
				if (state == 0) {			//display PWM signal inputs
					LCDprint("               ",1,1);
					LCDprint("               ",2,1);

					buff = malloc(17*sizeof(char));
					sprintf(buff, "PWM1:%d,%d",in0,in1);
					LCDprint(buff,1,1);
					free(buff);

					buff = malloc(17*sizeof(char));
					sprintf(buff, "PWM2:%d,%d",in2,in3);
					LCDprint(buff,2,1);
					free(buff);

					state=1;

				} else if (state == 1) {
					LCDprint("               ",1,1);
					LCDprint("               ",2,1);

					buff = malloc(17*sizeof(char));
					sprintf(buff,"Direction:");
					LCDprint(buff,1,1);
					free(buff);

					buff = malloc(17*sizeof(char));

					if (in0>in1 && in2==in0 && in3==in1){	//backward (try 70 30 70 30)
						sprintf(buff,"Backward");
					} else if (in0<in1 && in2==in0 && in3==in1) {	//forward (try 30 70 30 70)
						sprintf(buff,"Forward");
					} else if (in0<in1 && in3<in1) {	//forward left (try 30 70 40 60)
						sprintf(buff,"Forward Left");
					} else if (in2<in3 && in1<in3) {	//forward right (try 30 70 40 60)
						sprintf(buff,"Forward Right");
					} else if (in0==in1 && in2==in3) {	//stop (try 30 70 30 70)
						sprintf(buff,"Stop");
					} else if (in0==in1 && in2>in3) {	//back left (try 50 50 70 30)
						sprintf(buff,"Back Left");
					} else if (in0==in1 && in2<in3) {	//back right (try 70 30 50 50)
						sprintf(buff,"Back Righ");
					} else {
						sprintf(buff,"Turning");
					}

					LCDprint(buff,2,1);
					free(buff);

					state=0;
				}
			}
			printf("%f\n", Volts_at_Pin(QFP32_MUX_P1_0));

		}
		// if (inrange) {
		// 	while (inrange){
		// 		printPython(); //Comment out for rest of code
		// 		if (STOP_BUTTON ==0) {
		// 			break;
		// 		}
		//  	}
	 // 	}
 //	}
 	//if (!inrange) {
		printf("\x1b[2J");
		printf("\rNumbers entered not within 0-100 range\n Program Ended");
	//}
}
