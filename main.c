#include <stm32f0xx_rcc.h>
#include <stm32f0xx_tim.h>
#include <stm32f0xx_usart.h>
#include <stm32f0xx_gpio.h>
#include <stm32f0xx_misc.h>
#include <stm32f0xx_flash.h>
#include <stm32f0xx_exti.h>
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

struct ErrorState
{
	uint8_t MainLine; //99 means no errors
	uint8_t ProblemType ; // 0 - no connection, 1 - short
	uint8_t SecondLine; // line that has shorts or no connection
};

struct ErrorState errors;

void RCC_Config(void)
{
//	ErrorStatus HSIStartUpStatus;

//	RCC_DeInit();

	RCC_HSICmd(ENABLE);

	while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET);

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
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC | RCC_AHBPeriph_GPIOD, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = KEYTRANSISTORSC_PIN;
	GPIO_Init(KEYTRANSISTORSC_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = KEYTRANSISTOR24_PIN;
	GPIO_Init(KEYTRANSISTOR24_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin = LED_PIN;
	GPIO_Init(LED_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Pin = SWITCH_PIN;
	GPIO_Init(SWITCH_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(SWITCH_PORT, SWITCH_PIN, Bit_SET);
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

uint32_t button_check_counter = 0;
uint8_t previous_button_state = 0;
uint8_t current_button_state = 0;
uint8_t button_pressed_flag = 0;
uint32_t button_pressed_counter = 0;
uint16_t second_counter = 0;

void SysTick_Handler(void)
{
	systick_counter++;
	button_check_counter++;
	second_counter++;

	if(button_check_counter >= 30)
	{
		button_check_counter = 0;

		previous_button_state = current_button_state;
		current_button_state = GPIO_ReadInputDataBit(SWITCH_PORT, SWITCH_PIN);

		if(previous_button_state == 1 && current_button_state == 0)
		{
			button_pressed_flag = 1;
		}
		if(previous_button_state == 0 && current_button_state == 0)
		{
			button_pressed_counter++;
		}
		else button_pressed_counter = 0;
	}
	if(second_counter >= 1000)
	{
		second_counter = 0;
		GPIO_WriteBit(LED_PORT, LED_PIN, !GPIO_ReadOutputDataBit(LED_PORT, LED_PIN));
	}
}

void play()
{
	LCDClear();
	LCDOutString("D              ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("=D             ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("==D            ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("8==D           ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString(" 8==D          ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("  8==D         ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("   8==D        ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("    8==D       ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("     8==D      ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("      8==D     ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("       8==D    ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("        8==D   ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("         8==D  ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("          8==D ");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("           8==D");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("            8==");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("             8=");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("              8");
	wait_ms(200);
	LCDXY(0,0);
	LCDOutString("               ");

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

	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = pins_to_check[0].Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	errors.MainLine = line;
	for(int i = 0; i < 16; i++)
	{
		GPIO_InitStructure.GPIO_Pin = pins_to_check[i].Pin;
		GPIO_Init(pins_to_check[i].Port, &GPIO_InitStructure);
	}
	GPIO_InitStructure.GPIO_Pin = pins_to_check[line].Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_Init(pins_to_check[line].Port, &GPIO_InitStructure);

	GPIO_WriteBit(pins_to_check[line].Port, pins_to_check[line].Pin, Bit_SET);

	for(int i = line + 1; i < 16; i++)
	{
		uint8_t input_state = GPIO_ReadInputDataBit(pins_to_check[i].Port, pins_to_check[i].Pin);
		if(input_state != logic_table[line][i])
		{
			errors.SecondLine = i;
			if(input_state == 0) errors.ProblemType = 0;
			else errors.ProblemType = 1;
			return 1;
		}
	}


	return 0;
}

uint8_t CheckAllLines()
{
	for(int i = 0; i < 16; i++)
	{
		if(CheckLine(i)) return i;
	}
	return 99;
}

void ReportLastTest()
{
	if(errors.MainLine < 15)
	{
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		USART_SendData(USART2, errors.MainLine);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		USART_SendData(USART2, errors.ProblemType);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		USART_SendData(USART2, errors.SecondLine);

		LCDXY(0,0);
		switch (errors.MainLine) {
			case 0:
				LCDOutString("FANGND-");
				break;
			case 1:
				LCDOutString("FAN24V-");
				break;
			case 2:
				LCDOutString("TP 3V3-");
				break;
			case 3:
				LCDOutString("TP1.24-");
				break;
			case 4:
				LCDOutString("PROGND-");
				break;
			case 5:
				LCDOutString("PRO24V-");
				break;
			case 6:
				LCDOutString("FANGND-");
				break;
			case 7:
				LCDOutString("FAN24V-");
				break;
			case 8:
				LCDOutString("LEDGND-");
				break;
			case 9:
				LCDOutString("LED24V-");
				break;
			case 10:
				LCDOutString("FANGND-");
				break;
			case 11:
				LCDOutString("FAN24V-");
				break;
			case 12:
				LCDOutString("TP 3V3-");
				break;
			case 13:
				LCDOutString("TP1.24-");
				break;
			case 14:
				LCDOutString("PROGND-");
				break;
			case 15:
				LCDOutString("PRO24V-");
				break;
			default:
				break;
		}
		if(errors.ProblemType == 0)
			LCDSendByte('/');
		else
			LCDSendByte('-');

		switch (errors.SecondLine) {
			case 0:
				LCDOutString("-FANGND");
				break;
			case 1:
				LCDOutString("-FAN24V");
				break;
			case 2:
				LCDOutString("-TP 3V3");
				break;
			case 3:
				LCDOutString("-TP1.24");
				break;
			case 4:
				LCDOutString("-PROGND");
				break;
			case 5:
				LCDOutString("-PRO24V");
				break;
			case 6:
				LCDOutString("-FANGND");
				break;
			case 7:
				LCDOutString("-FAN24V");
				break;
			case 8:
				LCDOutString("-LEDGND");
				break;
			case 9:
				LCDOutString("-LED24V");
				break;
			case 10:
				LCDOutString("-FANGND");
				break;
			case 11:
				LCDOutString("-FAN24V");
				break;
			case 12:
				LCDOutString("-TP 3V3");
				break;
			case 13:
				LCDOutString("-TP1.24");
				break;
			case 14:
				LCDOutString("-PROGND");
				break;
			case 15:
				LCDOutString("-PRO24V");
				break;
			default:
				break;
		}
	}
	else
	{
		USART_SendData(USART2, 99);
		LCDXY(0,0);
		LCDOutString(" Wszystko cacy! ");
	}
}

void TellThatThereIsAProblemWithLine(uint8_t number)
{
	LCDXY(0,0);
	if(number >= 15)
	{
		LCDOutString("Wszystko cacy!");
		LCDXY(0,1);
		LCDOutString("                ");
		return;
	}
	LCDOutString("   Problem z    ");
	LCDXY(0,1);
	switch (number) {
		case 0:
			LCDOutString("Fan1HrtngFrntGND");
			break;
		case 1:
			LCDOutString("Fan1HrtngFrnt24V");
			break;
		case 2:
			LCDOutString("TP HrtngFrnt3V3 ");
			break;
		case 3:
			LCDOutString("TP HrtngFrnt1.24");
			break;
		case 4:
			LCDOutString("PROGND HrtngFrnt");
			break;
		case 5:
			LCDOutString("PRO24V HrtngFrnt");
			break;
		case 6:
			LCDOutString("Fan1GND HrtngBck");
			break;
		case 7:
			LCDOutString("Fan1 24VHrtngBck");
			break;
		case 8:
			LCDOutString("LED GND KK A    ");
			break;
		case 9:
			LCDOutString("LED 24V KK A    ");
			break;
		case 10:
			LCDOutString("Fan1 GND KK A   ");
			break;
		case 11:
			LCDOutString("Fan1 24V KK A   ");
			break;
		case 12:
			LCDOutString("TP 3V3 KK B     ");
			break;
		case 13:
			LCDOutString("TP 1.24 KK B    ");
			break;
		case 14:
			LCDOutString("PRO GND KK B    ");
			break;
		case 15:
			LCDOutString("PRO 24V KK B    ");
			break;
		case 99:
			LCDOutString("     NICZYM     ");
		default:
			break;
	}
}

void wait_ms(uint32_t t)
{
	systick_counter = 0;
	while(systick_counter < t);
}

void UARTSendMessage(char * s)
{
	  while(*s)
	  {
		  while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
		  USART_SendData(USART2, *s++);
	  }
}

void PrepareForConnectionTest()
{
	//transistor 24 off
	GPIO_WriteBit(KEYTRANSISTOR24_PORT, KEYTRANSISTOR24_PIN, Bit_RESET);

	wait_ms(10);

	//transistor sc on
	GPIO_WriteBit(KEYTRANSISTORSC_PORT, KEYTRANSISTORSC_PIN, Bit_SET);
}

int main(void)
{
	RCC_Config();
	GPIO_Config();
	UART_Config();
	SysTick_Config_Mod(SysTick_CLKSource_HCLK_Div8, 6000); //1kHz
	uint8_t i = 0;
//	while(1)
//	{
//	   while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//	   USART_SendData(USART2, i++);
//		GPIO_WriteBit(LED_PORT, LED_PIN, !GPIO_ReadOutputDataBit(LED_PORT, LED_PIN));
////		for(uint32_t i = 0; i < 1000000; i++);
//		wait_ms(200);
//	}
	LCDInit();
	LCDXY(0,0);
	LCDOutString("      ELO!      ");

//	SysTick_Config(6000*8);
//	while(1);
//	{
//		GPIO_WriteBit(LED_PORT, LED_PIN, !GPIO_ReadOutputDataBit(LED_PORT, LED_PIN));
//		for(uint32_t i = 0; i < 1000000; i++);
//	}
	//	wait_ms(1000);


	DefinePins();
	while (1)
	{
		if(button_pressed_flag == 1)
		{
//			UARTSendMessage("Pressed button\r\n");
			button_pressed_flag = 0;
			PrepareForConnectionTest();
			uint8_t shorts_state = CheckAllLines();
			ReportLastTest();
//			USART_SendData(USART2, shorts_state);
//			TellThatThereIsAProblemWithLine(shorts_state);
			if(shorts_state != 99) wait_ms(5000);
			else wait_ms(2000);
			LCDClear();
			LCDXY(0,0);
			LCDOutString("Dawaj nastepny!");

  		}
		if(button_pressed_counter >= 150)
		{
			UARTSendMessage("Rocket incoming, bitches!\r\n");
			play();
		}
	}

}
