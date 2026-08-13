#include "stm32g431xx.h"


static void systick_init(void)
{
	//1. LOAD  → quy định counter sẽ đếm bao nhiêu
	//2. VAL   → xóa/reset giá trị counter hiện tại
	//3. CTRL  → chọn clock và ENABLE counter
	SysTick->LOAD =  (SystemCoreClock / 1000000U) - 1U;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
}


static void delay_us(uint32_t us)
{
	for (uint32_t i=0; i<us; i++)
	{
		while((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0)
		{}
	}
}

static void delay_ms(uint32_t ms)
{
	delay_us(ms*1000U);
}


static void ADC_init(void)
{
/*configurate clock and mode*/

	// Enable GPIOC clock
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;

	//set PC2 to Analog mode
	GPIOC->MODER |= (3U << (2 * 2));

	// Enable ADC1/ADC2 peripheral clock
	RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;

	//clock source selection ADC12 kernel clock source = SYSCLK (10)
	RCC->CCIPR &= ~RCC_CCIPR_ADC12SEL;
	RCC->CCIPR |= RCC_CCIPR_ADC12SEL_1;

	//ADC clock mode Async mode: use adc_ker_ck CKMODE = 00
	ADC12_COMMON->CCR &= ~ADC_CCR_CKMODE;

	//ADC prescaler input ADC clock divided by 2 PRESC = 0001
	ADC12_COMMON->CCR &= ~ADC_CCR_PRESC;
	ADC12_COMMON->CCR |= ADC_CCR_PRESC_0;

	//total number of conversions in the sequence (0000)
	ADC1->SQR1 &= ~ADC_SQR1_L_Msk;

	//select channel 8 at sequence 1
	ADC1->SQR1 &= ~ADC_SQR1_SQ1_Msk;
	ADC1->SQR1 |= (8U << ADC_SQR1_SQ1_Pos);

/*Analog run*/

/*procedure to Calibrate the ADC*/
	//exit deep-power-down mode  DEEPPWD
	ADC1->CR &= ~ADC_CR_DEEPPWD;

	//Enable the ADC voltage regulator  ADVREGEN
	ADC1->CR |= ADC_CR_ADVREGEN;

	//wait for the startup time of regulator to configure the ADC (20micro sec)
	delay_us(20);

	//select the input mode for this calibarion (single ADCALDIF=0) 
	ADC1->CR &= ~ADC_CR_ADCALDIF;

	//set ADCAL (start calibration)
	ADC1->CR |= ADC_CR_ADCAL;

	//wait until ADCAL = 0 (End calibration)
	while((ADC1->CR & ADC_CR_ADCAL) != 0)
	{}


/*procedure to enable ADC*/
	//clear the ADRDY bit 
	ADC1->ISR |= ADC_ISR_ADRDY;

	//Set ADEN
	ADC1->CR |= ADC_CR_ADEN;

	//wait until ADRDY = 1 
	while((ADC1->ISR & ADC_ISR_ADRDY) == 0)
	{}

	//clear the ADRDY bit (optional 
	ADC1->ISR |= ADC_ISR_ADRDY;

/*sampling time*/
	ADC1->SMPR1 &= ~ADC_SMPR1_SMP8_Msk;
	ADC1->SMPR1 |= ADC_SMPR1_SMP8_2; //47.5 ADC clock cycles (100)

}

static uint32_t  ADC_read(void)
{
	/*resolution mặc định 12bit
	PC2 ≈ 0 V      → DR ≈ 0
	PC2 ≈ VDDA/2   → DR ≈ 2048
	PC2 ≈ VDDA     → DR ≈ 4095
	*/
	ADC1->CR |= ADC_CR_ADSTART;

    	while ((ADC1->ISR & ADC_ISR_EOC) == 0);
	//while(1){}
	return ADC1->DR;
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
	ADC_init();

    while (1)
    {
	uint32_t val = ADC_read();

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

