//  EFM8_Servo.c: Uses timer 5 interrupt to generate a servo motor control signal.
//
//  Copyright (c) 2010-2018 Jesus Calvino-Fraga
//  ~C51~

#include <EFM8LB1.h>
#include <stdlib.h>
#include <stdio.h>

volatile unsigned int pwm_reload0;
volatile unsigned int pwm_reload1;
volatile unsigned char pwm_state0=0;
volatile unsigned char pwm_state1=0;
volatile unsigned char count20ms;

#define PWMOUT0 P2_6
#define PWMOUT1 P3_0

#define SYSCLK 72000000L // SYSCLK frequency in Hz
#define BAUDRATE 115200L
#define RELOAD_10MS (0x10000L-(SYSCLK/(12L*100L)))

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN |= 0x80;
	RSTSRC = 0x02;

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
	P2MDOUT|=0b_0000_0011;
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X10; // Enable T0 on P0.0
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	// Configure Uart 0
	SCON0 = 0x10;
	CKCON0 |= 0b_0000_0000 ; // Timer 1 uses the system clock divided by 12.
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready

	// Initialize timer 5 for periodic interrupts
	SFRPAGE=0x10;
	TMR5CN0=0x00;
	pwm_reload0=0x10000L-(SYSCLK*1.5e-3)/12.0; // 1.5 miliseconds pulse is the center of the servo
	pwm_reload1=0x10000L-(SYSCLK*1.5e-3)/12.0; // 1.5 miliseconds pulse is the center of the servo
	TMR5=0xffff;   // Set to reload immediately
	EIE2|=0b_0000_1000; // Enable Timer5 interrupts
	TR5=1;         // Start Timer5 (TMR5CN0 is bit addressable)
	
	EA=1;
	
	SFRPAGE=0x00;
	
	return 0;
}

void Timer5_ISR (void) interrupt INTERRUPT_TIMER5
{
	SFRPAGE=0x10;
	TF5H = 0; // Clear Timer5 interrupt flag
	// Since the maximum time we can achieve with this timer in the
	// configuration above is about 10ms, implement a simple state
	// machine to produce the required 20ms period.
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

	//SFRPAGE=0x10;
	TF5H = 0; // Clear Timer5 interrupt flag
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

void main (void)
{
    float pulse_width0;
    float pulse_width1;
    
    count20ms=0; // Count20ms is an atomic variable, so no problem sharing with timer 5 ISR
    while((1000/20)>count20ms); // Wait a second to give PuTTy a chance to start
    
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	printf("EFM8 Servo motor signal generation using Timer 5.\r\n");

    // In a HS-422 servo a pulse width between 0.6 to 2.4 ms gives about 180 deg
    // of rotation range.
	while(1)
	{
		printf("\nPulse width [0.6,2.4] (ms)=");
		//scanf("%f", &pulse_width0);
		scanf("%f %f", &pulse_width0, &pulse_width1);
		// if((pulse_width0>=0.6)&&(pulse_width0<=2.4001))
		// {
	 //        pwm_reload0=0x10000L-(SYSCLK*pulse_width0*1.0e-3)/12.0;
		// }
		// else
		// {
		//    printf("Error: pulse width must be between 0.6 and 2.4 ms\n");
		// }

		if((pulse_width0>=0.6)&&(pulse_width0<=2.4001)&&(pulse_width1>=0.6)&&(pulse_width1<=2.4001))
		{
	 	       pwm_reload0=0x10000L-(SYSCLK*pulse_width0*1.0e-3)/12.0;
	 			  pwm_reload1=0x10000L-(SYSCLK*pulse_width0*1.0e-3)/12.0;
		}
		else
		{
		   printf("Error: pulse width must be between 0.6 and 2.4 ms\n");
		}
	}
}
