#include "stm32f05xxx.h"
#include "serial.h"

void ResetInit(void);
void Default_Handler(void);
void Hard_Fault_Handler(void);
void isr_usart1(void);

// Allocated stack space as defined in linker script lpc824.ld
extern unsigned int _StackTop;

void main(void);
// The section "vectors" is placed at the beginning of flash by the linker script stm32f05xxx.ld
const void * Vectors[] __attribute__((section(".vectors"))) ={
	&_StackTop,         /* Top of stack */
	ResetInit,   		/* Reset Handler */
	Default_Handler,	/* NMI */
	Hard_Fault_Handler,	/* Hard Fault.  If not using the right libraries this will happen. */
	0,	                /* Reserved */
	0,            		/* Reserved */
	0,                  /* Reserved */
	0,                  /* Reserved */
	0,                  /* Reserved */
	0,                  /* Reserved */
	0,                  /* Reserved */
	Default_Handler,	/* SVC */
	0,                 	/* Reserved */
	0,                 	/* Reserved */
	Default_Handler,   	/* PendSV */
	Default_Handler, 	/* SysTick */		
/* External interrupt handlers follow */
	Default_Handler, 	/* WWDG */
	Default_Handler, 	/* PVD */
	Default_Handler, 	/* RTC */
	Default_Handler, 	/* FLASH */
	Default_Handler, 	/* RCC */
	Default_Handler, 	/* EXTI0_1 */
	Default_Handler, 	/* EXTI2_3 */
	Default_Handler, 	/* EXTI4_15 */
	Default_Handler, 	/* TSC */
	Default_Handler, 	/* DMA_CH1 */
	Default_Handler, 	/* DMA_CH2_3 */
	Default_Handler, 	/* DMA_CH4_5 */
	Default_Handler,	/* ADC_COMP */
	Default_Handler ,  	/* TIM1_BRK_UP_TRG_COM */
	Default_Handler, 	/* TIM1_CC */
	Default_Handler, 	/* TIM2 */
	Default_Handler, 	/* TIM3 */
	Default_Handler, 	/* TIM6_DAC */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* TIM14 */
	Default_Handler, 	/* TIM15 */
	Default_Handler,	/* TIM16 */
	Default_Handler, 	/* TIM17 */
	Default_Handler, 	/* I2C1 */
	Default_Handler, 	/* I2C2 */
	Default_Handler, 	/* SPI1 */
	Default_Handler, 	/* SPI2 */
	isr_usart1,     	/* USART1 */
	Default_Handler, 	/* USART2 */
	Default_Handler, 	/* RESERVED */
	Default_Handler, 	/* CEC */
	Default_Handler 	/* RESERVED */
};

// gcc and g++ compatible C runtime initialization.
extern unsigned int _etext, _data, _edata, _bss, _ebss;
extern unsigned int __init_array_start;
extern unsigned int __init_array_end;
//
static inline void crt0 (void)
{
    unsigned int *src, *dest;

    // copy the data section
    src  = &_etext;
    dest = &_data;
    while (dest < &_edata)
        *(dest++) = *(src++);

    // blank the bss section
    while (dest < &_ebss)
        *(dest++) = 0;

    // call C++ constructors
    dest = &__init_array_start;
    while (dest < &__init_array_end)
      (*(void(**)(void)) dest++)();
}

void initClock()
{
    // Set the PLL up
    // Disable PLL
    RCC_CR &= ~BIT24; // PLL_ON=0
    while( (RCC_CR & BIT25)); // wait for PLL ready to be cleared
	// Warning here: if system clock is greater than 24MHz then wait-state(s) need to be
    // inserted into Flash memory interface
    FLASH_ACR |= BIT0;
    FLASH_ACR &=~(BIT2 | BIT1);
    // Turn on FLASH prefetch buffer
    FLASH_ACR |= BIT4;

    // set PLL multiplier to 12 (yielding 48MHz)
    RCC_CFGR &= ~(BIT21 | BIT20 | BIT19 | BIT18);
    RCC_CFGR |= (BIT21 | BIT19 ); 

    // Need to limit ADC clock to below 14MHz so will change ADC prescaler to 4
    RCC_CFGR |= BIT14;

	// Do the following to push HSI clock out on PA8 (MCO)
	// for measurement purposes.  Should be 8MHz or thereabouts (verified with oscilloscope)
	/*
    RCC_CFGR |= ( BIT26 | BIT24 );
    RCC_AHBENR |= BIT17;
    GPIOA_MODER |= BIT17;
	*/

    // and turn the PLL back on again
    RCC_CR |= BIT24;
    // Wait for the PLL to lock
    while( !(RCC_CR & BIT25));
          
    // set PLL as system clock source 
    RCC_CFGR |= BIT1;
}

// Reset entry point. Sets up the C runtime environment.
__attribute__ ((section(".after_vectors"), naked))
void ResetInit()
{
	initClock();
    crt0();
	initUART(115200);
	enable_interrupts();

	#ifdef SHOW_LD_SCRIPT_ASSIGMENTS
	PRINTVAR(&_etext)
	PRINTVAR(&_data)	    
	PRINTVAR(&_edata)	    
	PRINTVAR(&_bss)	    
	PRINTVAR(&_ebss)
	PRINTVAR(&__init_array_start)
	PRINTVAR(&__init_array_end)	    
	#endif
   
	main();
	//eputs("Returned from main()");
	while (1) ; // hang if main returns
}

void Default_Handler()
{
	eputs("Interrupt without ISR.  System stop.");
	while(1);
}

void Hard_Fault_Handler()
{
	eputs("Hard Fault interrupt.  System stop.");
	while(1);
}