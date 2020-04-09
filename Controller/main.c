#include <stdio.h>
#include <string.h>
#include "stm32f05xxx.h"
#include "nrf24.h"
#include "adc.h"

#define F_CPU 48000000L

// These two functions are in serial.c
int egets(char *s, int Max);
int serial_data_available(void);

void delay(int dly)
{
	while( dly--);
}

void wait_1ms(void)
{
	// For SysTick info check the STM32F0xxx Cortex-M0 programming manual page 85.
	STK_RVR = (F_CPU/1000L) - 1;  // set reload register, counter rolls over from zero, hence -1
	STK_CVR = 0; // load the SysTick counter
	STK_CSR = 0x05; // Bit 0: ENABLE, BIT 1: TICKINT, BIT 2:CLKSOURCE
	while((STK_CSR & BIT16)==0); // Bit 16 is the COUNTFLAG.  True when counter rolls over from zero.
	STK_CSR = 0x00; // Disable Systick counter
}

void delayMs(int len)
{
	while(len--) wait_1ms();
}

// https://community.st.com/s/question/0D50X00009XkXwy/stm32f0-spi-read-and-write
static void config_SPI(void)
{
	RCC_AHBENR |= BIT17;  // Enable GPIOA clock
	RCC_APB2ENR |= BIT12; // peripheral clock enable for SPI1 (page 122 of RM0091 Reference manual)
	
	// Configure PA3 for CE, pin 9 in LQFP32 package
	GPIOA_MODER |= BIT6; // Make pin PA3 output
	GPIOA_ODR |= BIT3; // CE=1
	
	// Configure PA4 for CSn, pin 10 in LQFP32 package
	GPIOA_MODER |= BIT8; // Make pin PA4 output
	GPIOA_ODR |= BIT4; // CSn=1
	
	//Configure PA5 for SPI1_SCK, pin 11 in LQFP32 package
	GPIOA_MODER |= BIT11; // AF-Mode (page 157 of RM0091 Reference manual)
	GPIOA_AFRL  |= 0; // AF0 selected (page 161 of RM0091 Reference manual)
	
	//Configure PA6 for SPI1_MISO, pin 12 in LQFP32 package
	GPIOA_MODER |= BIT13; // AF-Mode (page 157 of RM0091 Reference manual)
	GPIOA_AFRL  |= 0; // AF0 selected (page 161 of RM0091 Reference manual)
	
	//Configure PA7 for SPI1_MOSI, pin 13 in LQFP32 package
	GPIOA_MODER |= BIT15; // AF-Mode (page 157 of RM0091 Reference manual)
	GPIOA_AFRL  |= 0; // AF0 selected (page 161 of RM0091 Reference manual)
	
	SPI1_CR1 = 0x00000000; // Reset SPI1 CR1 Registry.  Page 801 of RM0091 Reference manual
	SPI1_CR1 = (( 0ul << 0) | // CPHA=0 (the nRF24L01 samples data in the falling edge of the clock which works oke for mode (0,0) in the STM32!)
				( 0ul << 1) | // CPOL=0
				( 1ul << 2) | // MSTR=1
				( 7ul << 3) | // BR (fPCLK/256) ~= 187 kbit/sec ]
				( 0ul << 7) | // MSBFIRST
				( 1ul << 8) | // SSI must be 1 when SSM=1 or a frame error occurs
				( 1ul << 9)); // SSM
	SPI1_CR2 = ( BIT10 | BIT9 | BIT8 ); // 8-bits at a time (page 803 of RM0091 Reference manual)
	SPI1_CR1 |= BIT6; // Enable SPI1
}

uint8_t spi_transfer(uint8_t tx)
{
	uint8_t data=0;
	
	while ((SPI1_SR & BIT1) == 0); // SPI status register (SPIx_SR) is in page 806
	*((uint8_t *)&SPI1_DR) = tx; // "SPI1_DR = wr;" sends 16 bits instead of 8, that is why we are type-casting
	while (SPI1_SR & BIT7); // Check Busy flag (Page 806)
	//while ((SPI1_SR & BIT0) == 0); // 0: Rx buffer empty (hangs here)
	data = *((uint8_t *)&SPI1_DR); // "data = SPI1_DR;" waits for 16-bits instead of 8, that is why we are type-casting
	
	return data;
}

uint8_t temp;
uint8_t data_array[32];
uint8_t tx_address[] = "TXADD";
uint8_t rx_address[] = "RXADD";
 
void main(void)
{
	float a[2]; //a[0] is x, a[1] is y
	int j[2];
	int dir;
	int current, previous,buttonA,buttonB,buttonX,buttonY;
	buttonX=0;
	buttonY=0;
	initADC();
	
	RCC_AHBENR |= 0x00020000; // peripheral clock enable for port A
	
	delayMs(500); // Give PuTTY a chance to start before sending text
	
	printf("\x1b[2J\x1b[1;1H"); // Clear screen using ANSI escape sequence.
	printf ("STM32F051 nRF24L01 test program\r\n"
	        "File: %s\r\n"
	        "Compiled: %s, %s\r\n\r\n",
	        __FILE__, __DATE__, __TIME__);
    
	config_SPI();
	
	// Use PA8 (pin 18) for pushbutton input
	GPIOA_MODER &= ~(BIT16 | BIT17); // Make pin PA8 input
	// Activate pull up for pin PA8:
	GPIOA_PUPDR |= BIT16; 
	GPIOA_PUPDR &= ~(BIT17);

	// Use PA2 (pin 8) for transmitter/receiver selection input
	GPIOA_MODER &= ~(BIT4 | BIT5); // Make pin PA2 input
	// Activate pull up for pin PA2:
	GPIOA_PUPDR |= BIT4; 
	GPIOA_PUPDR &= ~(BIT5);
	
	// Use PA0 (pin 6) for transmitter/receiver selection input
	GPIOA_MODER &= ~(BIT0 | BIT1); // Make pin PA2 input
	// Activate pull up for pin PA2:
	GPIOA_PUPDR |= BIT0; 
	GPIOA_PUPDR &= ~(BIT1);
	
	// Use PA1 (pin 7) for transmitter/receiver selection input
	GPIOA_MODER &= ~(BIT2 | BIT3); // Make pin PA2 input
	// Activate pull up for pin PA2:
	GPIOA_PUPDR |= BIT2; 
	GPIOA_PUPDR &= ~(BIT3);
	

	config_SPI();
    
    nrf24_init(); // init hardware pins
    nrf24_config(120,32); // Configure channel and payload size

    /* Set the device as Transmitter */

    	printf("Set as transmitter\r\n");
	    nrf24_tx_address(tx_address);
	    nrf24_rx_address(rx_address);


    previous=(GPIOA_IDR&BIT8)?0:1;
    while(1)
    {   
    	
    	/*Button Crap (Pin 18)
    	current=(GPIOA_IDR&BIT8)?1:0;
		if(current!=previous)
		{	
			buttonA=1;
			previous=current;
			//printf("PA8=%d\r", current);
			fflush(stdout);
		}
		else
			buttonA=0;
		*/
		buttonA=GPIOA_IDR&BIT8;
		if(buttonA==256)
			buttonA=0;
		else
			buttonA=1;
			
		buttonB=(GPIOA_IDR&BIT2);
		if(buttonB==4)
			buttonB=0;
		else
			buttonB=1;
	
		buttonX=!(GPIOA_IDR&BIT0);
	
		buttonY=!(GPIOA_IDR&BIT1);



	
    	//ADC CRAP 
    	ADC_CHSELR = BIT8;          // Select Channel 8
		j[0]=readADC();
		a[0]=(j[0]*3.3)/0x1000;
		ADC_CHSELR = BIT9;          // Select Channel 9
		j[1]=readADC();
		a[1]=(j[1]*3.3)/0x1000;
		//Direction numbers are based on the position of the number in the number pad. 
		//8 is up, 2 is down, 4 left, 6 right etc	
		if(a[0]>1.7){
			if(a[1]>1.1)
				dir = 7;
			else if(a[1]<0.95)
				dir = 1;
			else
				dir = 4;
			}
		else if(a[0]<1.5){
			if(a[1]>1.1)
				dir =9;
			else if(a[1]<0.95)
				dir =3;
			else
				dir =6;
			}
		else
			if(a[1]>1.1)
				dir =8;
			else if(a[1]<0.95)
				dir =2;
			else
				dir =5;
    
    	
    	//TRANSCIEVER CRAP
        if(nrf24_dataReady())
        {
            nrf24_getData(data_array);
        	printf("IN: %s\r\n", data_array);
        }
        
        if(serial_data_available()) // Did something arrived from the serial port?
        {
        	egets(data_array, sizeof(data_array));
		    printf("\r\n");    
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

	        //sprintf(data_array,"%i%i%i%i%i",buttonA,buttonB,buttonX,buttonY,dir); 
	        //prints out the inputs from the controller, for Debugging only

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
}
