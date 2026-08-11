#include "stm32g431xx.h"


static void systick_init(void)
{
	//1. LOAD  → quy định counter sẽ đếm bao nhiêu
	//2. VAL   → xóa/reset giá trị counter hiện tại
	//3. CTRL  → chọn clock và ENABLE counter
	SysTick->LOAD =  (SystemCoreClock / 1000U) - 1U;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
}

static void delay_ms(uint32_t ms)
{
	for(uint32_t i=0; i<ms; i++)
	{
		while((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
		{}
	}
}



int main(void)
{
    	// Bật clock cho GPIOC GPIOA
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    	// PC13 = input mode 00 at MODE13
    	GPIOC->MODER &= ~(3U << (13 * 2));

	// PA5 = output mode
	GPIOA->MODER &= ~(3U << (5 * 2));
	GPIOA->MODER |=  (1U << (5 * 2));

	//init systick 
	systick_init();

    while (1)
    {

	if((GPIOC->IDR & GPIO_IDR_ID13) != 0)
	{
		delay_ms(20);
		if ((GPIOC->IDR & GPIO_IDR_ID13) != 0)
		{
			// LED ON
        		GPIOA->BSRR = (1U << 5);
			while((GPIOC->IDR & GPIO_IDR_ID13) !=0);
		}
	}
	else
	{
        	// LED OFF
        	GPIOA->BSRR = (1U << (5 + 16));
	}
    }
}

