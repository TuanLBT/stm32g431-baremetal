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

static void GPIO_init(void)
{
	// Bật clock cho GPIOC 
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;


        //Bật clock cho GPIOA
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    	// PC13 = input mode 00 at MODE13
    	GPIOC->MODER &= ~(3U << (13 * 2));

	// PA5(LD2) = alternate function  mode 
	GPIOA->MODER &= ~(3U << (5 * 2));
	GPIOA->MODER |=  (2U << (5 * 2));

	//alter function selection
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5;
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL5_0;
}

static void TIM2_init(void)
{
	//timer 2 clock enable (tim_ker_ck) internal clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

	//timer2 prescaler (1000:1)
	TIM2->PSC = 15999;
	//APB1 clock prescaler (default 1:1)

/*output compare mode proced*/

	//select the counter clock (timer2 clock)

	//Write the desired data in the TIMx_ARR and TIMx_CCRx registers.
	//PWM frequency (1hz)
	TIM2->ARR = 999;
	//duty cycle (0%)
	TIM2->CCR1 = 0;

	//Set the CCxIE and/or CCxDE bits if an interrupt and/or a DMA request is to be generated. (not use)

	//ensure capture/compare 1 selection is output (Channel 1 = output)
	TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;

	//Select the output mode (PWM mode 1)
	TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
	TIM2->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;

	//enable the output
	TIM2->CCER |= TIM_CCER_CC1E;


/*PWM config*/
	//edge-aligned mode (00) (counter đếm theo kiểu nào) (không phải PWM )
	TIM2->CR1 &= ~TIM_CR1_CMS;

	//couting up (0)
	TIM2->CR1 &= ~TIM_CR1_DIR;

	/*tạo một update event để nạp PSC/ARR ngay lập tức:
	Lý do là PSC có preload, giá trị m viết vào không nhất thiết được dùng ngay cho counter cho đến khi có update event. UG ép một update event ngay lúc init*/
	TIM2->EGR |= TIM_EGR_UG;

	//enable counter
	TIM2->CR1 |= TIM_CR1_CEN;

}

int main(void)
{
	//init
	GPIO_init();
	TIM2_init();
	systick_init();
	ADC_init();

    while (1)
    {
	uint32_t ADC_val = ADC_read();
	uint32_t duty_cycle_cal = (ADC_val*1000U)/4059U;
	TIM2->CCR1 = duty_cycle_cal;
    }
}

