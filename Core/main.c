#include "stm32g431xx.h"


volatile uint32_t push_cnt = 0;


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

	// PA5(LD2) GPIO alternate function  mode 
	GPIOA->MODER &= ~(3U << (5 * 2));
	GPIOA->MODER |=  (2U << (5 * 2));

	//alter function selection for PA5
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5;
	GPIOA->AFR[0] |= GPIO_AFRL_AFSEL5_0;

	//PA2 (TX),PA3(RX) GPIO alternate function mode 
	GPIOA->MODER &= ~(3U << (2 * 2));
        GPIOA->MODER |=  (2U << (2 * 2));

	GPIOA->MODER &= ~(3U << (3 * 2));
        GPIOA->MODER |=  (2U << (3 * 2));

	//alter function selection for PA2 and PA3 (AF12)
	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2;
        GPIOA->AFR[0] |= GPIO_AFRL_AFSEL2_2 | GPIO_AFRL_AFSEL2_3;

	GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3;
        GPIOA->AFR[0] |= GPIO_AFRL_AFSEL3_2 | GPIO_AFRL_AFSEL3_3;

}


static void TIM2_init(void)
{
	//timer 2 clock enable (tim_ker_ck) internal clock
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;

	//timer counter clock = timer clock / 16000
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


static void LPUART1_init(void)
{
	//clock source selection (HSI16)
	RCC->CCIPR &= ~RCC_CCIPR_LPUART1SEL;
	RCC->CCIPR |= RCC_CCIPR_LPUART1SEL_1;

	//enable clock for LPUART1
	RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;


	// disable UE before config
	LPUART1->CR1 &= ~USART_CR1_UE;

	//1 start bit , 8 data bits length 
	LPUART1->CR1 &= ~USART_CR1_M0;
	LPUART1->CR1 &= ~USART_CR1_M1;

	//baud rate (115200 symbol/bit events per second)
	LPUART1->BRR = 35556;

	//number of stop bits (1 stopbit (00))
	LPUART1->CR2 &= ~USART_CR2_STOP;

	//enable the LPUART1
	LPUART1->CR1 &= ~USART_CR1_UE;
	LPUART1->CR1 |= USART_CR1_UE;

	//enable DMA (if use)

	//Set the TE bit in USART_CR1 to send an idle frame as first transmission.
	LPUART1->CR1 |= USART_CR1_TE;

	//Set the RE bit LPUART_CR1. This enables the receiver which begins searching for a start bit.
	LPUART1->CR1 |= USART_CR1_RE;

}

static void LPUART1_transmit(uint8_t *msg, uint32_t len)
{

	//Write the data to send in the USART_TDR register.
	/*TXE/TXFNF = 	TDR sẵn sàng nhận byte tiếp theo
			TC = byte cuối đã truyền xong hoàn toàn*/
	for (uint32_t i=0; i < len; i++)
	{
		while ((LPUART1->ISR & USART_ISR_TXE_TXFNF) == 0) {}
		LPUART1->TDR = msg[i];
	}
	while ((LPUART1->ISR & USART_ISR_TC) == 0) {}
}

static void LPUART1_transmit_byte(uint8_t data)
{
	while ((LPUART1->ISR & USART_ISR_TXE_TXFNF) == 0) {}
	LPUART1->TDR = data;
}

static void LPUART1_transmit_uint32(uint32_t value)
{
	uint8_t buf[10];
	uint32_t i = 0;

	// special case: 0
	if (value == 0)
	{
		LPUART1_transmit_byte('0');
		return;
	}

	// tách từng chữ số từ phải sang trái
	while (value > 0)
	{
		buf[i] = (value % 10U) + '0';
		value /= 10U;
		i++;
	}

	// gửi ngược lại để đúng thứ tự
	while (i > 0)
	{
		i--;
		LPUART1_transmit_byte(buf[i]);
	}
}

static uint32_t  LPUART1_receive(void)
{
	//wait if RXNE is set (when a character is received)
	while ((LPUART1->ISR & USART_ISR_RXNE) == 0) {}

	//store byte received
	return LPUART1->RDR;
}

/*
EXTI 13 (PC13)
configure tje EXTI_IMR register 
Configure the Trigger Selection bits of the Interrupt line (EXTI_RTSR and EXTI_FTSR)
Configure the enable and mask bits that control the NVIC IRQ channel mapped to the
EXTI so that an interrupt coming from one of the EXTI lines can be correctly
acknowledged.
*/
static void EXTI_config(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // PC13 input
    //GPIOC->MODER &= ~(3U << (13 * 2));

    // PC13 -> EXTI13
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI13_Msk;
    SYSCFG->EXTICR[3] |=  SYSCFG_EXTICR4_EXTI13_PC;

    // unmask EXTI13
    EXTI->IMR1 |= EXTI_IMR1_IM13;

    // both rising and falling edge trigger interrupt 
    EXTI->FTSR1 |= EXTI_FTSR1_FT13;
    EXTI->RTSR1 |=  EXTI_RTSR1_RT13;

	// enable EXTI15_10_IRQn interrupt in NVIC (CMSIS Core API)
    NVIC_SetPriority(EXTI15_10_IRQn, 5);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR1 & EXTI_PR1_PIF13)//phân biệt ngắt từ EXTI13 hay EXTI10,11,12,14,15
    {
		EXTI->PR1 = EXTI_PR1_PIF13;
		push_cnt++;
		if(GPIOC->IDR & GPIO_IDR_ID13)
		{
			//PWM = 100% duty cycle
			TIM2->CCR1 = 1000;
		}
		else
		{
			//PWM = 0% duty cycle
			TIM2->CCR1 = 0;
		}
	}
}

int main(void)
{
	//init
	GPIO_init();
	TIM2_init();
	systick_init();
	ADC_init();
	LPUART1_init();
	EXTI_config();
	while (1)
	{

	}
}

