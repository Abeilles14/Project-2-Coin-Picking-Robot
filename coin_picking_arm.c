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
#define RELOAD_10MS (0x10000L-(SYSCLK/(12L*100L)+1))

#define LCD_RS P2_6
// #define LCD_RW Px_x // Not used in this code.  Connect to GND
#define LCD_E  P2_5
#define LCD_D4 P2_4
#define LCD_D5 P2_3
#define LCD_D6 P2_2
#define LCD_D7 P2_1
#define CHARS_PER_LINE 16

#define OUT0 P2_4		//motor 1
#define OUT1 P2_3

#define OUT2 P2_2		//motor 2
#define OUT3 P2_1

#define PWMOUT0 P2_0 		//bottom motor
#define PWMOUT1 P1_7		//top motor

#define PWMMAG P3_0			//electromagnet

#define VDD 3.3035 // The measured value of VDD in volts

volatile unsigned char pwm_count0=0;
volatile unsigned char pwm_count1=0;

volatile unsigned int in0 = 50;
volatile unsigned int in1 = 50;

volatile unsigned int in2 = 50;
volatile unsigned int in3 = 50;

volatile unsigned int pwm_reload0;
volatile unsigned int pwm_reload1;
volatile unsigned char pwm_state0 = 0;
volatile unsigned char pwm_state1 = 0;
volatile unsigned char count20ms;
volatile unsigned int arm_flag = 0;

unsigned char overflow_count;

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
	
	// Configure the pins used for square output
	P1MDOUT|=0b_1000_0000;
	P2MDOUT|=0b_0100_0011;
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X10; // Enable T0 on P0.0
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	// Configure Uart 0
	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif

	SCON0 = 0x10;
	CKCON0 |= 0b_0000_0000 ; // Timer 1 uses the system clock divided by 12.
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

	// Initialize timer 5 for periodic interrupts
	SFRPAGE=0x10;
	TMR5CN0=0x00;   // Stop Timer5; Clear TF5;
	pwm_reload0=0x10000L-(SYSCLK*1.5e-3)/12.0; // 1.5 miliseconds pulse is the center of the servo
	pwm_reload1=0x10000L-(SYSCLK*1.5e-3)/12.0; // 1.5 miliseconds pulse is the center of the servo
	TMR5=0xffff;   // Set to reload immediately
	EIE2|=0b_0000_1000; // Enable Timer5 interrupts
	TR5=1;         // Start Timer5 (TMR5CN0 is bit addressable)

	EA=1; // Enable interrupts

	SFRPAGE=0x00;
  	
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

void TIMER0_Init(void)
{
	TMOD&=0b_1111_0000; // Set the bits of Timer/Counter 0 to zero
	TMOD|=0b_0000_0101; // Timer/Counter 0 used as a 16-bit counter
	TR0=0; // Stop Timer/Counter 0
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

void Timer5_ISR (void) interrupt INTERRUPT_TIMER5
{
	SFRPAGE=0x10;
	TF5H = 0; // Clear Timer5 interrupt flag
	// Since the maximum time we can achieve with this timer in the
	// configuration above is about 10ms, implement a simple state
	// machine to produce the required 20ms period.
	if (arm_flag == 0){
		switch (pwm_state0)
		{
		   case 0:
		      PWMOUT0=1;
		      TMR5RL=RELOAD_10MS;
		      pwm_state0=1;
		      count20ms++;
		   break;
		   case 1:
		      PWMOUT0=0;
		      TMR5RL=RELOAD_10MS-pwm_reload0;
		      pwm_state0=2;
		   break;
		   default:
		      PWMOUT0=0;
		      TMR5RL=pwm_reload0;
		      pwm_state0=0;
		   break;
		}
	}	else if (arm_flag == 1) {
			switch (pwm_state1)
			{
			   case 0:
			      PWMOUT1=1;
			      TMR5RL=RELOAD_10MS;
			      pwm_state1=1;
			      count20ms++;
			   break;
			   case 1:
			      PWMOUT1=0;
			      TMR5RL=RELOAD_10MS-pwm_reload1;
			      pwm_state1=2;
			   break;
			   default:
			      PWMOUT1=0;
			      TMR5RL=pwm_reload1;
			      pwm_state1=0;
			   break;
			}
	}

}


void arm_pick_up(void) {			//picks up coins
	PWMMAG = 0;	
	waitms(500);
	arm_flag = 1;
	pwm_reload1=0x10000L-(SYSCLK*2.3*1.0e-3)/12.0;		//down
	PWMMAG = 1;						//electromagnet on
	waitms(500);
	arm_flag = 0;
	pwm_reload0=0x10000L-(SYSCLK*2.4*1.0e-3)/12.0;		//sweep left
	waitms(500);
	arm_flag = 1;
	pwm_reload1=0x10000L-(SYSCLK*0.6*1.0e-3)/12.0;		//pick up
	waitms(500);
	arm_flag = 0;
	pwm_reload0=0x10000L-(SYSCLK*0.9*1.0e-3)/12.0;		//carry right
	waitms(500);
	arm_flag = 1;
	pwm_reload1=0x10000L-(SYSCLK*1.0*1.0e-3)/12.0;		//drop
	PWMMAG = 0;										//Electromagnet off
	waitms(500);
	arm_flag = 0;
	pwm_reload0=0x10000L-(SYSCLK*1.2*1.1e-3)/12.0;		//centered
	waitms(500);
}

void arm_reset(void) {		//resets and centers arm
	PWMMAG = 0;											//Electromagnet off
	arm_flag = 1;
	pwm_reload1=0x10000L-(SYSCLK*1.2*1.0e-3)/12.0;		//up
	waitms(500);
	arm_flag = 0;
	pwm_reload0=0x10000L-(SYSCLK*1.2*1.0e-3)/12.0;		//centered
	waitms(500);
}

void main (void)
{
	int state = 0;
	int previous_state = 0;
	int inrange = 1;
	unsigned long frequency;
	unsigned long freq_init;
	
	TIMER0_Init();

   count20ms=0; // Count20ms is an atomic variable, so no problem sharing with timer 5 ISR
   waitms(500);		//wait for putty to start

	// InitPinADC(1, 0); // Configure P2.5 as analog input
	// InitADC();
	// LCD_4BIT();

  	//initial frequency  
   	TL0=0;
	TH0=0;
	overflow_count=0;
	TF0=0;
	TR0=1; // Start Timer/Counter 0
		
	waitms(1000);
	TR0=0; // Stop Timer/Counter 0
	freq_init=overflow_count*0x10000L+TH0*0x100L+TL0;

	while(freq_init< 50000);	//ensures that frequency readings are correct

   	arm_reset();

	in0 = 60;
	in1 = 40;
	in2 = 60;
	in3 = 40;

	while(1)
	{
		TL0=0;
		TH0=0;
		overflow_count=0;
		TF0=0;
		TR0=1; // Start Timer/Counter 0
		
		waitms(1000);
		TR0=0; // Stop Timer/Counter 0
		frequency=overflow_count*0x10000L+TH0*0x100L+TL0;

		printf("\rf=%luHz", frequency);
		printf("\x1b[0K"); // ANSI: Clear from cursor to end of line.

		if (frequency >= freq_init + 100) {
			in0 = 20;
			in1 = 80;
			in2 = 20;
			in3 = 80;
			waitms(500);
			in0 = 50;
			in1 = 50;
			in2 = 50;
			in3 = 50;
			arm_pick_up();
		} else {
			in0 = 60;
			in1 = 40;
			in2 = 60;
			in3 = 40;
		}
	}

}
