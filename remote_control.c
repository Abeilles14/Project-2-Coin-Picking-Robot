#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EFM8LB1.h>
#include "nrf24.h"

#define SYSCLK 48000000L    // Internal oscillator frequency in Hz
#define BAUDRATE 115200L
#define F_SCK_MAX 2000000L  // Max SCK freq (Hz)

#define FORWARD	8
#define BACKWARD 2
#define LEFT 4 
#define RIGHT 6
#define FORWARD_RIGHT 9
#define FORWARD_LEFT 7
#define BACKWARD_RIGHT 3
#define BACKWARD_LEFT 1
#define NO_MOVEMENT 5

volatile unsigned int in0 = 50;
volatile unsigned int in1 = 50;

volatile unsigned int in2 = 50;
volatile unsigned int in3 = 50;

// Pins used by the SPI interface:
// 	P0.0: SCK
// 	P0.1: MISO
// 	P0.2: MOSI
// 	P0.3: SS*

	
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

	P0MDOUT=0b_0001_1101;//SCK, MOSI, P0.3, TX0 are puspull, all others open-drain
	XBR0=0b_0000_0011;//SPI0E=1, URT0E=1
	XBR1=0b_0000_0000;
	XBR2=0b_0100_0000; // Enable crossbar and weak pull-ups

	#if ( ((SYSCLK/BAUDRATE)/(12L*2L)) > 0x100)
		#error Can not configure baudrate using timer 1 
	#endif
	// Configure Uart 0
	SCON0 = 0x10;
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(12L*2L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready

	// SPI inititialization
	SPI0CKR = (SYSCLK/(2*F_SCK_MAX))-1;
	SPI0CFG = 0b_0100_0000; //SPI in master mode
	SPI0CN0 = 0b_0000_0001; //SPI enabled and in three wire mode
	
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
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<4; k++) Timer3us(250);
}

uint8_t spi_transfer(uint8_t tx)
{
   SPI0DAT=tx;
   while(!SPIF);
   SPIF=0;
   return SPI0DAT;
}

void safe_gets(char *s, int n, int to)
{
	int to_cnt=0;
	unsigned char j=0;
	unsigned char c, us_cnt=0;
	
	while(1)
	{
		if(RI)
		{
			to_cnt=0;
			us_cnt=0;
			c=getchar();
			if ( (c=='\n') || (c=='\r') ) break;
			if(j<(n-1))
			{
				s[j]=c;
				j++;
			}
		}
		else
		{
			Timer3us(20);
			us_cnt++;
			if(us_cnt==50)
			{
				to_cnt++;
				us_cnt=0;
			}
		}
		if(to_cnt==to) break;
	}
	s[j]=0;
}

int ctlIn,dir,Abutton,Bbutton,Xbutton,Ybutton;
uint8_t temp;
xdata uint8_t data_array[32];
const uint8_t tx_address[] = "TXADD";
const uint8_t rx_address[] = "RXADD";
 
void main (void)
{
	
	waitms(500);
	printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	
/*	Probably don't need this 
	printf ("EFM8LB1 SPI/nRF24L01 transceiver test program.\n"
	        "File: %s\n"
	        "Compiled: %s, %s\n\n",
	        __FILE__, __DATE__, __TIME__);
*/
	
    nrf24_init(); // init hardware pins
    nrf24_config(120,32); // Configure channel and payload size
 
    /* Set the device as reciever */

    	printf("Set as receiver\r\n");
	    nrf24_tx_address(rx_address);
	    nrf24_rx_address(tx_address);
    

    while(1)
    {    
        if(nrf24_dataReady())
        {
            nrf24_getData(data_array); 
            ctlIn =atoi(data_array);	//converts string recieved from radio into an integer
     
           
            /*ctlIn is formated as follows: 
              the 5 bit positions carry the values for the push buttons and the analog stick direction
              Starting from the leftmost bit:
              A button  (0 or 1)
              B button  (0 or 1)
              X button  (0 or 1)
              Y button  (0 or 1)
              direction (1-9) */
            						
            if(ctlIn>=10000){
            	Abutton=1;
            	ctlIn=ctlIn%10000;	//removes the leftmost bit if set
            } else
            	Abutton=0;
            
            if(ctlIn>=1000){
            	Bbutton=1;
            	ctlIn=ctlIn%1000;	//removes the leftmost bit if set
            } else
            	Bbutton=0;
            	
            if(ctlIn>=100){
            	Xbutton=1;
            	ctlIn=ctlIn%100;	//removes the leftmost bit if set
            } else 
            	Xbutton=0;
            if(ctlIn>=10){
            	Ybutton=1;
            	ctlIn=ctlIn%10;		//removes the leftmost bit if set
            } else
            	Ybutton=0;
            
            dir=ctlIn;
             printf("IN: %i%i%i%i%i\r\n", Abutton,Bbutton,Xbutton,Ybutton,dir); //Prints data that is recieved, may want to remove
            	
            	            	
            /* FOR DEBUGGING ONLY
            if(inputs==0)
            	printf("A button pressed");
            printf("ctlIn:%i",ctlIn);
            printf("button=%i,dir=%i",button,dir);	
            */         	
        }
        
        if(RI) //Other radio junk, stuff to do with handling lost messages and such
        {
        	//safe_gets(data_array, sizeof(data_array), 2000);
        	gets(data_array);
		    printf("\r\n");    
	        nrf24_send(data_array);        
		    while(nrf24_isSending());
		    temp = nrf24_lastMessageStatus();
			if(temp == NRF24_MESSAGE_LOST)
		    {                    
		        printf("> Message lost\r\n"); //for debugging, may want to remove
		        dir = NO_MOVEMENT; //makes robot not move if message is lost
		        Abutton=0;    
		    }
			nrf24_powerDown();
    		nrf24_powerUpRx();
		}
		/* Transmitter code, not needed while being used as reciever
		if(P3_7==0)
		{
			while(P3_7==0);
			strcpy(data_array, "Button test");
	        nrf24_send(data_array);
		    while(nrf24_isSending());
		    temp = nrf24_lastMessageStatus();
			if(temp == NRF24_MESSAGE_LOST)
		    {                    
		        printf("> Message lost\r\n");    
		    }
			nrf24_powerDown();
    		nrf24_powerUpRx();
		}
		*/
		
		/*Motor Control:
		  Directions are defined at the top of the file and look like this:
		  	7	8	9
		  	4	5	6	
		  	1	2	3
		  This is based of a standard dial pad, with the analog stick centered at 5
		  */		
				if (dir == FORWARD) {
					in0 = 70;
					in1 = 30;
					in2 = 70;
					in3 = 30;
				} else if (dir == BACKWARD) {
					in0 = 30;
					in1 = 70;
					in2 = 30;
					in3 = 70;
				} else if (dir == FORWARD_RIGHT) {
					in0 = 70;
					in1 = 30;
					in2 = 50;
					in3 = 50;
				} else if (dir == FORWARD_LEFT) {
					in0 = 50;
					in1 = 50;
					in2 = 70;
					in3 = 30;
				} else if (dir == RIGHT) {
					in0 = 70;
					in1 = 30;
					in2 = 30;
					in3 = 70;
				} else if (dir == LEFT) {
					in0 = 30;
					in1 = 70;
					in2 = 70;
					in3 = 30;
				} else if (dir == BACKWARD_RIGHT) {
					in0 = 30;
					in1 = 70;
					in2 = 50;
					in3 = 50;
				} else if (dir == BACKWARD_LEFT) {
					in0 = 50;
					in1 = 50;
					in2 = 30;
					in3 = 70;
				} else if (dir == NO_MOVEMENT) {
					in0 = 50;
					in1 = 50;
					in2 = 50;
					in3 = 50;
				}
    }
}