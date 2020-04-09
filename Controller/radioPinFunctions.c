/*
* ----------------------------------------------------------------------------
* THE COFFEEWARE LICENSEù (Revision 1
* <ihsan@kehribar.me> wrote this file. As long as you retain this notice you
* can do whatever you want with this stuff. If we meet some day, and you think
* this stuff is worth it, you can buy me a coffee in return.
* -----------------------------------------------------------------------------
* Please define your platform specific functions in this file ...
* -----------------------------------------------------------------------------
*/

#include "stm32f05xxx.h"
#include <stdint.h>

/* ------------------------------------------------------------------------- */
void nrf24_setupPins()
{
	// The pins are configured in file main.c, in the function config_SPI

}

/* ------------------------------------------------------------------------- */
void nrf24_ce_digitalWrite(uint8_t state)
{
	if(state)
	{
		GPIOA_ODR |= BIT3;
	}
	else
	{
		GPIOA_ODR &= ~BIT3;
	}
}

/* ------------------------------------------------------------------------- */
void nrf24_csn_digitalWrite(uint8_t state)
{
	int j;
	
	if(state)
	{
		for(j=0; j<50; j++); // The SPI in the STM32 is a bit quick reporting that is done, so wait a bit
		GPIOA_ODR |= BIT4;
	}
	else
	{
		GPIOA_ODR &= ~BIT4;
		for(j=0; j<50; j++);
	}
}

/* ------------------------------------------------------------------------- */
void nrf24_sck_digitalWrite(uint8_t state)
{
    // Nothing to do.  The SPI hardware handles it.
}

/* ------------------------------------------------------------------------- */
void nrf24_mosi_digitalWrite(uint8_t state)
{
    // Nothing to do.  The SPI hardware handles it.
}

/* ------------------------------------------------------------------------- */
uint8_t nrf24_miso_digitalRead()
{
    // Nothing to do.  The SPI hardware handles it.
}
/* ------------------------------------------------------------------------- */
