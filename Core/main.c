#include "stm32g431xx.h"



int main(void)
{
    // 1) Bật clock cho GPIOC GPIOA
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    // 2) PC13 = input mode 00 at MODE13
    	GPIOC->MODER &= ~(3U << (13 * 2));

	// PA5 = output mode
	GPIOA->MODER &= ~(3U << (5 * 2));
	GPIOA->MODER |=  (1U << (5 * 2));

    while (1)
    {

	if((GPIOC->IDR & GPIO_IDR_ID13) != 0)
	{
		// LED ON
        	GPIOA->BSRR = (1U << 5);
	}
	else
	{
        	// LED OFF
        	GPIOA->BSRR = (1U << (5 + 16));
        }
    }
}
