//  EFM8_Servo.c: Uses timer 5 interrupt to generate a servo motor control signal.
//
//  Copyright (c) 2010-2018 Jesus Calvino-Fraga
//  ~C51~

#include <EFM8LB1.h>
#include <stdlib.h>
#include <stdio.h>

volatile unsigned int pwm_reload0;
volatile unsigned int pwm_reload1;
volatile unsigned char pwm_state0 = 0;
volatile unsigned char pwm_state1 = 0;
volatile unsigned char count20ms;
volatile unsigned int arm_flag = 0;

#define PWMOUT0 P2_0 		//bottom motor
#define PWMOUT1 P1_7		//top motor

#define PWMMAG P2_6			//electromagnet

#define SYSCLK 72000000L // SYSCLK frequency in Hz
#define BAUDRATE 115200L
#define RELOAD_10MS (0x10000L-(SYSCLK/(12L*100L)))

unsigned char overflow_count;

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
	P1MDOUT|=0b_1000_0000;
	P2MDOUT|=0b_0100_0011;
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
	waitms(500);
	arm_flag = 1;
	pwm_reload1=0x10000L-(SYSCLK*2.3*1.0e-3)/12.0;		//down
	//PWMMAG = 1;										// Electromagnet on
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
	//PWMMAG = 0;										//Electromagnet off
	waitms(500);
	arm_flag = 0;
	pwm_reload0=0x10000L-(SYSCLK*1.2*1.2e-3)/12.0;		//centered
	waitms(500);
}

void arm_reset(void) {		//resets and centers arm
	waitms(500);
	//PWMMAG = 0;											//Electromagnet off
	arm_flag = 1;
	pwm_reload1=0x10000L-(SYSCLK*1.2*1.0e-3)/12.0;		//up
	waitms(500);
	arm_flag = 0;
	pwm_reload0=0x10000L-(SYSCLK*1.2*1.0e-3)/12.0;		//centered
	waitms(500);
}

void main (void)
{

    unsigned long frequency;
	
	TIMER0_Init();
    
    count20ms=0; // Count20ms is an atomic variable, so no problem sharing with timer 5 ISR
    while((1000/20)>count20ms); // Wait a second to give PuTTy a chance to start
    
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	printf("EFM8 Servo motor signal generation using Timer 5.\r\n");

    // In a HS-422 servo a pulse width between 0.6 to 2.4 ms gives about 180 deg
    // of rotation range.
    //arm_reset();
    arm_reset();

	while(1)
	{

		//check if metal detected
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

		if (frequency >= 54620) {
			arm_pick_up();
		}

		// printf("\nPulse width bottom motor [0.6,2.4] (ms)=");

		// scanf("%f", &pulse_width0);

		// if(pulse_width0 == 1)
		// {
		// 	arm_pick_up();
		// }
		// else
		// {
		//    arm_reset();
		// }
	}
}