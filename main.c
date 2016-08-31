#include <stm32f0xx_rcc.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_usart.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_flash.h>
#include "hd44780.h"

#define KEYTRANSISTORSC_PORT GPIOA
#define KEYTRANSISTORSC_PIN GPIO_Pin_5

#define KEYTRANSISTOR24_PORT GPIOA
#define KEYTRANSISTOR24_PIN GPIO_Pin_4

#define LED_PORT GPIOD
#define LED_PIN GPIO_Pin_2

#define SWITCH_PORT GPIOA
#define SWITCH_PIN GPIO_Pin_12

struct Pin
{
	GPIO_TypeDef * Port;
	uint16_t Pin;
};

struct Pin pins_to_check[16];

void RCC_Config(void)
{
	ErrorStatus HSEStartUpStatus;

	RCC_DeInit();

	RCC_HSICmd(ENABLE);

	HSEStartUpStatus = RCC_WaitForHSEStartUp();

	if (HSEStartUpStatus == SUCCESS)
	{
		FLASH_PrefetchBufferCmd(ENABLE);
		FLASH_SetLatency(FLASH_Latency_1);

		RCC_HCLKConfig(RCC_SYSCLK_Div1);
		RCC_PCLKConfig(RCC_HCLK_Div1);
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, RCC_PLLMul_12);
		RCC_PLLCmd(ENABLE);
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET);
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
		while (RCC_GetSYSCLKSource() != 0x08);

	}
}

unsigned int SysTick_Config_Mod(unsigned long int SysTick_CLKSource, unsigned long int Ticks)
{
	unsigned long int Settings;
	assert_param(IS_SYSTICK_CLK_SOURCE(SysTick_CLKSource));
	if (Ticks > SysTick_LOAD_RELOAD_Msk)
		return 1;
	SysTick->LOAD = (Ticks & SysTick_LOAD_RELOAD_Msk) - 1;
	NVIC_SetPriority(SysTick_IRQn, 0);
	SysTick->VAL = 0;
	Settings = SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
	if (SysTick_CLKSource == SysTick_CLKSource_HCLK)
		Settings |= SysTick_CLKSource_HCLK;
	else
		Settings |= SysTick_CLKSource_HCLK_Div8;

	SysTick->CTRL = Settings;
	return 0;
}

void GPIO_Config()
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = KEYTRANSISTORSC_PIN;
	GPIO_Init(KEYTRANSISTORSC_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = KEYTRANSISTOR24_PIN;
	GPIO_Init(KEYTRANSISTOR24_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = SWITCH_PIN;
	GPIO_Init(SWITCH_PORT, &GPIO_InitStructure);
}

void SetKey24()
{
	GPIO_WriteBit(KEYTRANSISTOR24_PORT, KEYTRANSISTOR24_PIN, Bit_SET);
}

void ResetKey24()
{
	GPIO_WriteBit(KEYTRANSISTOR24_PORT, KEYTRANSISTOR24_PIN, Bit_RESET);
}

void SetKeySC()
{
	GPIO_WriteBit(KEYTRANSISTORSC_PORT, KEYTRANSISTORSC_PIN, Bit_SET);
}

void ResetKeySC()
{
	GPIO_WriteBit(KEYTRANSISTORSC_PORT, KEYTRANSISTORSC_PIN, Bit_RESET);
}

uint8_t GetSwitchState()
{
	return GPIO_ReadInputDataBit(SWITCH_PORT, SWITCH_PIN);
}

void UART_Config()
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	USART_InitTypeDef USART_InitStructure;
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	USART_Cmd(USART2, ENABLE);
}

uint32_t systick_counter = 0;

void Systick_Handler(void)
{
	systick_counter++;
	if (systick_counter == 100)
	{
		systick_counter = 0;
		GPIO_WriteBit(LED_PORT, LED_PIN, !GPIO_ReadOutputDataBit(LED_PORT, LED_PIN));
	}
}

void DefinePins()
{
	//FAN1 GND harting front
	pins_to_check[0].Port = GPIOA;
	pins_to_check[0].Pin = GPIO_Pin_9;

	//FAN1 24V harting front
	pins_to_check[1].Port = GPIOA;
	pins_to_check[1].Pin = GPIO_Pin_8;

	//TP 3V3 harting front
	pins_to_check[2].Port = GPIOC;
	pins_to_check[2].Pin = GPIO_Pin_8;

	//TP 1.24 harting front
	pins_to_check[3].Port = GPIOC;
	pins_to_check[3].Pin = GPIO_Pin_9;

	//RFU GND harting front
	pins_to_check[4].Port = GPIOA;
	pins_to_check[4].Pin = GPIO_Pin_11;

	//RFU 24V harting front
	pins_to_check[5].Port = GPIOA;
	pins_to_check[5].Pin = GPIO_Pin_10;


	//FAN1 GND harting back
	pins_to_check[6].Port = GPIOB;
	pins_to_check[6].Pin = GPIO_Pin_13;

	//FAN1 24V harting back
	pins_to_check[7].Port = GPIOB;
	pins_to_check[7].Pin = GPIO_Pin_12;


	//FAN3 GND (LED) KK A
	pins_to_check[8].Port = GPIOA;
	pins_to_check[8].Pin = GPIO_Pin_6;

	//FAN3 24V (LED) KK A
	pins_to_check[9].Port = GPIOA;
	pins_to_check[9].Pin = GPIO_Pin_7;

	//FAN1 GND KK A
	pins_to_check[10].Port = GPIOC;
	pins_to_check[10].Pin = GPIO_Pin_4;

	//FAN1 24V KK A
	pins_to_check[11].Port = GPIOC;
	pins_to_check[11].Pin = GPIO_Pin_5;


	//TP 3V3 KK B
	pins_to_check[12].Port = GPIOB;
	pins_to_check[12].Pin = GPIO_Pin_14;

	//TP 1.24 KK B
	pins_to_check[13].Port = GPIOB;
	pins_to_check[13].Pin = GPIO_Pin_15;

	//RFU GND KK B
	pins_to_check[14].Port = GPIOC;
	pins_to_check[14].Pin = GPIO_Pin_6;

	//RFU 24V KK B
	pins_to_check[15].Port = GPIOC;
	pins_to_check[15].Pin = GPIO_Pin_7;
}

uint8_t logic_table[16][16] =
{
		{1,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0},
		{0,1,0,0,0,0,0,1,0,0,0,1,0,0,0,0},
		{0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0},
		{0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0},
		{0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0},
		{0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
		{0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0},
		{0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0},
		{0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0},
		{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1}
};

uint8_t CheckLine(uint8_t line)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = pins_to_check[line].Pin;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;


	for(int i = line; i < 16; i++)
	{

	}
}



int main(void)
{
	RCC_Config();
	GPIO_Config();
	UART_Config();
	LCDInit();
	SysTick_Config_Mod(SysTick_CLKSource_HCLK_Div8, 6000); //1kHz

	DefinePins();

	while (1)
	{
	}

}
