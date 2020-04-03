//  square.c: Uses timer 2 interrupt to generate a square wave in pin
//  P2.0 and a 75% duty cycle wave in pin P2.1
//  Copyright (c) 2010-2018 Jesus Calvino-Fraga
//  ~C51~

#include <stdlib.h>
#include <stdio.h>
#include <EFM8LB1.h>
#include <ctype.h>
#include <string.h>
#include "Tunes.h"

#define	PAUSE	0
#define	BASE_C	1
#define	BASE_Cs	2
#define	BASE_D	3
#define	BASE_Ds	4
#define	BASE_E	5
#define	BASE_F	6
#define	BASE_Fs	7
#define	BASE_G	8
#define	BASE_Gs	9
#define	BASE_A	10
#define	BASE_As	11
#define	BASE_B	12

#define	NORMAL		7
#define	LEGATO		8
#define	STACCATO	3

#define SYSCLK 72000000L
#define BAUDRATE 115200L
#define RELOAD_10MS (0x10000L-(SYSCLK/(12L*100L)+1))

#define EQ(A,B) !strcmp((A),(B))

void ParseMDL(char * music);

#define BUFFSIZE 15
char buff[BUFFSIZE+1];
const char what[]="What?\n";

unsigned char style, octave, note, tempo;
int actLen, defLen, cur;
extern volatile int timer_count;

// timer 0 used for systemclock
#define TIMER0_RELOAD_VALUE (65536L-((SYSCLK/12L)/1000L))	////!!!!!!!!!!!!

#define SOUNDPIN P2_6

volatile int timer_count;

extern const float FTone[];

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
volatile unsigned int sound_flag = 0;

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

	#if ( ((SYSCLK/BAUDRATE)/(12L*2L)) > 0x100)
		#error Can not configure baudrate using timer 1 
	#endif

		// Configure Uart 0
	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	
	// Configure the pins used for square output
	P1MDOUT|=0b_1000_0000;
	P2MDOUT|=0b_0100_0011;
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X10; // Enable T0 on P0.0
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	// initialize timer0 for system clock					////////////!!!!!!!!!!!!
	TR0=0; // stop timer 0
	TMOD =(TMOD&0xf0)|0x01; // T0=16bit timer
	TMR0=TIMER0_RELOAD_VALUE;
	TR0=1; // start timer 0
	ET0=1; // enable timer 0 interrupt

	// Configure Uart 0
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


void Timer0_ISR (void) interrupt INTERRUPT_TIMER0
{
	TMR0=TIMER0_RELOAD_VALUE;
	timer_count++;
}

void Timer2_ISR (void) interrupt INTERRUPT_TIMER2
{
	TF2H = 0; // Clear Timer2 interrupt flag
	
	if (sound_flag == 1) {
		SOUNDPIN=!SOUNDPIN;
	} else {

		TR2=1;

		pwm_count0++;
		if(pwm_count0>100) pwm_count0=0;
		
		OUT0=pwm_count0>in0?0:1;
		OUT1=pwm_count0>in1?0:1;

		pwm_count1++;
		if(pwm_count1>100) pwm_count1=0;
		
		OUT2=pwm_count1>in2?0:1;
		OUT3=pwm_count1>in3?0:1;
	}
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

unsigned char AsciiToHex(char * buff)
{
	return ((buff[0]-'0')*0x10)+(buff[1]-'0');
}


// Frequencies for equal-tempered scale, A4 = 440 Hz
// http://pages.mtu.edu/~suits/notefreqs.html

//2020 - April 01: pitch adjusting
const float FTone[] = 
{
	   30.87,32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,51.91,
	   55.00,58.27,
	   61.74,   65.41,   69.30,   73.42,   77.78,   82.41,   87.31,   92.50,
	   98.00,  103.83,  110.00,  116.54,  123.47,  130.81,  138.59,  146.83,
	  155.56,  164.81,  174.61,  185.00,  196.00,  207.65,  220.00,  233.08,
	  246.94,  261.62,  277.18,  293.66,  311.13,  329.63,  349.23,  369.99,
	  391.99,  415.30,  440.00,  466.16,  493.88,  523.25,  554.36,  587.33,
	  622.25,  659.25,  698.45,  739.98,  783.99,  830.60,  879.99,  932.32,
	  987.76, 1046.50, 1108.72, 1174.65, 1244.50, 1318.50, 1396.90, 1479.97,
	 1567.97, 1661.21, 1759.99, 1864.64, 1975.52, 2092.99, 2217.45, 2349.30,
   	 2489.00, 2637.00, 2793.81, 2959.94, 3135.94, 3322.42, 3519.98, 3729.29,
	 3951.04};
	 
//Use timer 2 to play the note.
void PlayNote(void)
{
	int tmsec, toff;

	// Compute in milliseconds the duration of a note or silence using:
	tmsec= ( (60000L/tempo)*400L ) / actLen;

	// Set the time when the sound will be turned off:
	toff=(tmsec*style)/8L;

	// Start the sound
	if (note!=0)
	{
		TMR2=TMR2RL=(unsigned int)(65536.0-((72.0e6)/(FTone[note]*2*12.0)));
		TR2=1;
		//turn flag on when sound out
    }
    else //It is a silence...
    {
    	TR2=0; //Turn off timer 2
    }

	//Count the milliseconds for the note or silence
	timer_count=0;
	while(timer_count<tmsec)
	{
		if(timer_count>toff) TR2=0; //Turn off timer 2
	}
	TR2=0; //Turn off timer 2
}

int GetNumber(char * music)
{
	int n=0;
	/*Get the number*/
	while (isdigit(music[cur]) && music[cur]) n=(n*10)+(music[cur++]-'0');
	return n;
}

void ParseMDL(char * music)
{
	bit getout;
	
	cur=0;
	style=NORMAL;
	//TR2=0;		
	
	while(music[cur] && (RI==0))
	{
	    //putchar(music[cur]);
		switch (toupper(music[cur]))
		{
			case '>':
				cur++;
				octave++;
			break;
	
			case '<':
				cur++;
				octave--;
			break;
	
			case 'O':
				cur++;
				octave=GetNumber(music);
			break;
	
			/*Choose a note or pause*/
			case 'A': case 'B': case 'C': case 'D':
			case 'E': case 'F': case 'G': case 'P':
	
				/*Select the note number from name and octave*/
				note=(octave*12);
				switch (toupper(music[cur])-'A')
				{
					case 0:  note+=BASE_A; break;
					case 1:  note+=BASE_B; break;
					case 2:  note+=BASE_C; break;
					case 3:  note+=BASE_D; break;
					case 4:  note+=BASE_E; break;
					case 5:  note+=BASE_F; break;
					case 6:  note+=BASE_G; break;
					default: note =PAUSE ; break;
				}
		
				cur++;
				actLen=defLen;
				
				getout=0;
				while(!getout)
				{
					switch (toupper(music[cur]))
					{
						case '+': case '#':
							cur++;
							note++;
						break;
	
						case '-':
							cur++;
							note--;
						break;
	
						case '.':
							cur++;
							actLen=(actLen*2)/3;
						break;
						
						/*Get note duration*/
						case '0': case '1': case '2': case '3': case '4': 
						case '5': case '6': case '7': case '8': case '9': 
							actLen=GetNumber(music)*100;/*Increase resolution*/
						break;
	
						default:
							/*Play note and continue*/
							PlayNote();
							getout=1;
						break;
					}
				}
			break;
	
			/*Play a note by its number*/
			case 'N':
				cur++;
				note=GetNumber(music);
				actLen=defLen;
				PlayNote();
			break;
			
			/*Set the default note duration*/
			case 'L':
				cur++;
				defLen=GetNumber(music)*100;/*Increase resolution*/
			break;
	
			/*Choose the playing style. Ignore foreground and
			background commands, since only background works here*/
			case 'M':
				cur++; 
				switch (toupper(music[cur]))
				{
					case 'N': style=NORMAL;   break;
					case 'L': style=LEGATO;   break;
					case 'S': style=STACCATO; break;
				}
				cur++;
			break;
	
			/*Select the tempo*/
			case 'T':
				cur++;
				tempo=GetNumber(music);
			break;
	
			/*Ignore and discard substrings.*/
			case 'X':
				cur++;
				/*Discard substring*/
				while ((music[cur]!='$') && music[cur]) cur++;
				cur++;/*Discard also the "$"*/
			break;
			
			/*Unknown commands and blanks are just ignored and discarded*/
			default:
				cur++;
			break;
		}
	}
	//TR2=1;
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
	//ParseMDL(cointune);
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
	unsigned long frequency;
	unsigned long freq_init;
	float volt_init[2];
	float v[2];
	int coin_count = 0;
	char c;

	sound_flag = 1;
	ParseMDL(starttune);		//mario start song
	sound_flag = 0;
	
	TIMER0_Init();

	InitPinADC(1, 1);
	InitPinADC(1, 2);
    InitADC();

    count20ms=0; // Count20ms is an atomic variable, so no problem sharing with timer 5 ISR
    waitms(500);		//wait for putty to start

    /*********** FREQUENCY ***********/
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

	/*********** ADC Voltage **********/
	volt_init[0] = Volts_at_Pin(QFP32_MUX_P1_1);
	volt_init[1] = Volts_at_Pin(QFP32_MUX_P1_2);

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
		v[0] = Volts_at_Pin(QFP32_MUX_P1_1);
		v[1] = Volts_at_Pin(QFP32_MUX_P1_2);


		printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
		printf("\033[%d;%dH", 1, 1);   //set cursor
		printf("\rMetal Detector: f=%luHz\n", frequency);
		printf("\rPerimeter Detector: P1.1=%7.5fV, P1.2=%7.5fV\r", v[0], v[1]);

		//CHECK METAL DETECTOR
		// if (frequency >= freq_init + 100) {
		// 	in0 = 20;
		// 	in1 = 80;
		// 	in2 = 20;
		// 	in3 = 80;
		// 	waitms(500);
		// 	in0 = 50;
		// 	in1 = 50;
		// 	in2 = 50;
		// 	in3 = 50;
		// 	arm_pick_up();
		//  ParseMDL(cointune);
		// 	coin_count++;
		// } else {
		// 	in0 = 60;
		// 	in1 = 40;
		// 	in2 = 60;
		// 	in3 = 40;
		// }

		//CHECK PERIMETER DETECTOR
		if ((v[0] >= volt_init[0] + 0.200) || (v[1] >= volt_init[1] + 0.200)){
			in0 = 80;
			in1 = 20;
			in2 = 20;
			in3 = 80;
			waitms(1000);

		}

		/********** SWITCH TO REMOTE CONTROL ************/
		// while(coin_count == 3){


		// }


    }
}



