
 //******************************************************************************
//    THE SOFTWARE INCLUDED IN THIS FILE IS FOR GUIDANCE ONLY.
//    AUTHOR SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT
//    OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
//    FROM USE OF THIS SOFTWARE.
//
//    PROGRAM ZAWARTY W TYM PLIKU PRZEZNACZONY JEST WYLACZNIE
//    DO CELOW SZKOLENIOWYCH. AUTOR NIE PONOSI ODPOWIEDZIALNOSCI
//    ZA ZADNE EWENTUALNE, BEZPOSREDNIE I POSREDNIE SZKODY
//    WYNIKLE Z JEGO WYKORZYSTANIA.
//******************************************************************************

//##############################################################################
// lcd_hd44780.c
// Dopasowane do zestawu uruchomieniowego STM32F0DISCOVERY by Mesho
//
// Zajrzyj na: atmegan.blogspot.com
//##############################################################################


#include "hd44780.h"

GPIO_InitTypeDef GPIO_InitStructure;

//-----------------------------------------------------------------------------
void lcd_writenibble(unsigned char nibbleToWrite)
{
  GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_SET);
  GPIO_WriteBit(LCD_GPIO, LCD_D4, (nibbleToWrite & 0x01));
  GPIO_WriteBit(LCD_GPIO, LCD_D5, (nibbleToWrite & 0x02));
  GPIO_WriteBit(LCD_GPIO, LCD_D6, (nibbleToWrite & 0x04));
  GPIO_WriteBit(LCD_GPIO, LCD_D7, (nibbleToWrite & 0x08));
  GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_RESET);
}

//-----------------------------------------------------------------------------
unsigned char LCD_ReadNibble(void)
{
  unsigned char tmp = 0;
  GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_SET);
  tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D4) << 0);
  tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D5) << 1);
  tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D6) << 2);
  tmp |= (GPIO_ReadInputDataBit(LCD_GPIO, LCD_D7) << 3);
  GPIO_WriteBit(LCD_GPIO, LCD_EN, Bit_RESET);
  return tmp;
}

//-----------------------------------------------------------------------------
unsigned char LCD_ReadStatus(void)
{
  unsigned char status = 0;

    GPIO_InitStructure.GPIO_Pin   =  LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7;
    GPIO_InitStructure.GPIO_Mode  =  GPIO_Mode_IN;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LCD_GPIO, &GPIO_InitStructure);

    GPIO_WriteBit(LCD_GPIO, LCD_RW, Bit_SET);
    GPIO_WriteBit(LCD_GPIO, LCD_RS, Bit_RESET);

    status |= (LCD_ReadNibble() << 4);
    status |= LCD_ReadNibble();

    GPIO_InitStructure.GPIO_Pin   =  LCD_D4 | LCD_D5 | LCD_D6 | LCD_D7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_Init(LCD_GPIO, &GPIO_InitStructure);

  return status;
}

//-----------------------------------------------------------------------------
void LCDSendByte(unsigned char dataToWrite)
{
  GPIO_WriteBit(LCD_GPIO, LCD_RW, Bit_RESET);
  GPIO_WriteBit(LCD_GPIO, LCD_RS, Bit_SET);

  lcd_writenibble(dataToWrite >> 4);
  lcd_writenibble(dataToWrite & 0x0F);

  while(LCD_ReadStatus() & 0x80);
}

//-----------------------------------------------------------------------------
void LCDAddress(unsigned char commandToWrite)
{
  GPIO_WriteBit(LCD_GPIO, LCD_RW | LCD_RS, Bit_RESET);
  lcd_writenibble(commandToWrite >> 4);
  lcd_writenibble(commandToWrite & 0x0F);

  while(LCD_ReadStatus() & 0x80);
}

//-----------------------------------------------------------------------------
void LCDOutString(unsigned char * text)
{
  while(*text)
    LCDSendByte(*text++);
}

//-----------------------------------------------------------------------------
void LCDXY(uint8_t x, uint8_t y)
{
	switch(y)
	{
		case 0:
			LCDAddress(128+x);
			break;
		case 1:
			LCDAddress(192+x);
			break;
		case 2:
			LCDAddress(148+x);
			break;
		case 3:
			LCDAddress(212+x);
			break;
	}
}

//-----------------------------------------------------------------------------
void LCD_ShiftLeft(void)
{
  LCDAddress(HD44780_DISPLAY_CURSOR_SHIFT | HD44780_SHIFT_LEFT | HD44780_SHIFT_DISPLAY);
}

//-----------------------------------------------------------------------------
void LCD_ShiftRight(void)
{
  LCDAddress(HD44780_DISPLAY_CURSOR_SHIFT | HD44780_SHIFT_RIGHT | HD44780_SHIFT_DISPLAY);
}

//-----------------------------------------------------------------------------
void LCDInit(void)
{
  volatile unsigned char i = 0;
  volatile unsigned int delayCnt = 0;
	RCC_AHBPeriphClockCmd(LCD_CLK_LINE, ENABLE);
	GPIO_InitStructure.GPIO_Pin = LCD_D4|LCD_D5|LCD_D6|LCD_D7|LCD_RS|LCD_RW|LCD_EN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;

GPIO_Init(LCD_GPIO, &GPIO_InitStructure);

  GPIO_ResetBits(LCD_GPIO, LCD_RS | LCD_EN | LCD_RW);

  for(delayCnt = 0; delayCnt < 300000; delayCnt++);

  for(i = 0; i < 3; i++)
  {
    lcd_writenibble(0x03);
    for(delayCnt = 0; delayCnt < 30000; delayCnt++);
  }

  lcd_writenibble(0x02);

  for(delayCnt = 0; delayCnt < 6000; delayCnt++);

  LCDAddress(HD44780_FUNCTION_SET |
                   HD44780_FONT5x7 |
                   HD44780_TWO_LINE |
                   HD44780_4_BIT);

  LCDAddress(HD44780_DISPLAY_ONOFF |
                   HD44780_DISPLAY_OFF);

  LCDAddress(HD44780_CLEAR);

  LCDAddress(HD44780_ENTRY_MODE |
                   HD44780_EM_SHIFT_CURSOR |
                   HD44780_EM_INCREMENT);

  LCDAddress(HD44780_DISPLAY_ONOFF |
                   HD44780_DISPLAY_ON |
                   HD44780_CURSOR_OFF |
                   HD44780_CURSOR_NOBLINK);
}

//-----------------------------------------------------------------------------
void lcd_addchar (unsigned char chrNum, unsigned char n, const unsigned char *p)
{         //chrNum  - character number (code) to be registered (0..7)
          //n       - number of characters to register
          //*p      - pointer to the character pattern (8 * n bytes)
        LCDAddress(HD44780_CGRAM_SET | chrNum * 8);
        n *= 8;
        do
                LCDSendByte(*p++);
        while (--n);
}

//-----------------------------------------------------------------------------
void LCDClear(void)
{
        LCDAddress(HD44780_CLEAR);
}

//-----------------------------------------------------------------------------
void LCDOutFloat(float f, uint8_t calk, uint8_t ulamk)
{
	uint8_t ujemna;
	uint32_t dzielnik=1;
	if(f<0)
	{
		ujemna = 1;
		f=-f;
	}
	else ujemna = 0;

	for(uint8_t i=1; i<calk; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=calk; i++)
	{
		if (((uint16_t)(f/dzielnik)%10)>0 || (dzielnik==1 && f < 10))
		{
			if(ujemna&&(!juzwystapilacyfra))LCDSendByte('-');
			else if(!juzwystapilacyfra) LCDSendByte(' ');
			juzwystapilacyfra=1;
		}
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(f/dzielnik)%10+48);
		else
		if(f<1&&i==calk) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
	LCDSendByte('.');
	dzielnik=1;
	for(uint8_t i=1; i<=ulamk; i++)
	{
		dzielnik*=10;
		LCDSendByte((uint32_t)(f*dzielnik)%10+48);
	}
}

void LCDOutUFloat(float f, uint8_t calk, uint8_t ulamk)
{
	uint32_t dzielnik=1;
	if(f<0)
	{
		f=-f;
	}

	for(uint8_t i=1; i<calk; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=calk; i++)
	{
		if (((uint16_t)(f/dzielnik)%10)>0 || (dzielnik==1 && f < 10))
		{

			juzwystapilacyfra=1;
		}
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(f/dzielnik)%10+48);
		else
		if(f<1&&i==calk) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
	LCDSendByte('.');
	dzielnik=1;
	for(uint8_t i=1; i<=ulamk; i++)
	{
		dzielnik*=10;
		LCDSendByte((uint32_t)(f*dzielnik)%10+48);
	}
}
//-----------------------------------------------------------------------------
void LCDOutDouble(double f, uint8_t calk, uint8_t ulamk)
{
	uint8_t ujemna;
	uint32_t dzielnik=1;
	if(f<0)
	{
		ujemna = 1;
		f=-f;
	}
	else ujemna = 0;

	for(uint8_t i=1; i<calk; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=calk; i++)
	{
		if (((uint16_t)(f/dzielnik)%10)>0 || (dzielnik==1 && f < 10))
		{
			if(ujemna&&(!juzwystapilacyfra))LCDSendByte('-');
			else if(!juzwystapilacyfra) LCDSendByte(' ');
			juzwystapilacyfra=1;
		}
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(f/dzielnik)%10+48);
		else
		if(f<1&&i==calk) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
	LCDSendByte('.');
	dzielnik=1;
	for(uint8_t i=1; i<=ulamk; i++)
	{
		dzielnik*=10;
		LCDSendByte((uint32_t)(f*dzielnik)%10+48);
	}
}

//-----------------------------------------------------------------------------
void LCDOutuint32(uint32_t u, uint8_t n)
{
	uint32_t dzielnik=1;
	for(uint8_t i=1; i<n; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=n; i++)
	{
		if (((uint16_t)(u/dzielnik)%10)>0) juzwystapilacyfra=1;
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(u/dzielnik)%10+48);
		else
		if(u<1&&dzielnik==1) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
}

//-----------------------------------------------------------------------------
void LCDOutint32(int32_t f, uint8_t n)
{
	uint8_t ujemna;
	uint32_t dzielnik=1;
	if(f<0)
	{
		ujemna = 1;
		n--;
		f=-f;
	}
	else ujemna = 0;

	for(uint8_t i=1; i<n; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=n; i++)
	{
		if (((uint16_t)(f/dzielnik)%10)>0 || (dzielnik==1 && f < 10))
		{
			if(ujemna&&(!juzwystapilacyfra))LCDSendByte('-');
			else if(!juzwystapilacyfra) LCDSendByte(' ');
			juzwystapilacyfra=1;
		}
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(f/dzielnik)%10+48);
		else
		if(f<1&&i==n) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
}

//-----------------------------------------------------------------------------
void LCDOutuint16(uint16_t u, uint8_t n)
{
	uint32_t dzielnik=1;
	for(uint8_t i=1; i<n; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=n; i++)
	{
		if (((uint16_t)(u/dzielnik)%10)>0) juzwystapilacyfra=1;
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(u/dzielnik)%10+48);
		else
		if(u<1&&dzielnik==1) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
}

//-----------------------------------------------------------------------------
void LCDOutint16(int16_t f, uint8_t n)
{
	uint8_t ujemna;
	uint32_t dzielnik=1;
	if(f<0)
	{
		ujemna = 1;
		n--;
		f=-f;
	}
	else ujemna = 0;

	for(uint8_t i=1; i<n; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=n; i++)
	{
		if (((uint16_t)(f/dzielnik)%10)>0 || (dzielnik==1 && f < 10))
		{
			if(ujemna&&(!juzwystapilacyfra))LCDSendByte('-');
			else if(!juzwystapilacyfra) LCDSendByte(' ');
			juzwystapilacyfra=1;
		}
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(f/dzielnik)%10+48);
		else
		if(f<1&&i==n) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
}

//-----------------------------------------------------------------------------
void LCDOutuint8(uint8_t u, uint8_t n)
{
	uint32_t dzielnik=1;
	for(uint8_t i=1; i<n; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=n; i++)
	{
		if (((uint16_t)(u/dzielnik)%10)>0) juzwystapilacyfra=1;
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(u/dzielnik)%10+48);
		else
		if(u<1&&dzielnik==1) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
}

//-----------------------------------------------------------------------------
void LCDOutint8(int8_t f, uint8_t n)
{
	uint8_t ujemna;
	uint32_t dzielnik=1;
	if(f<0)
	{
		ujemna = 1;
		n--;
		f=-f;
	}
	else ujemna = 0;

	for(uint8_t i=1; i<n; i++)
	{
		dzielnik*=10;
	}
	uint8_t juzwystapilacyfra=0;
	for(uint8_t i=1; i<=n; i++)
	{
		if (((uint16_t)(f/dzielnik)%10)>0 || (dzielnik==1 && f < 10))
		{
			if(ujemna&&(!juzwystapilacyfra))LCDSendByte('-');
			else if(!juzwystapilacyfra) LCDSendByte(' ');
			juzwystapilacyfra=1;
		}
		if (juzwystapilacyfra==1) LCDSendByte((uint16_t)(f/dzielnik)%10+48);
		else
		if(f<1&&i==n) LCDSendByte('0');
		else LCDSendByte(' ');
		dzielnik/=10;
	}
}
