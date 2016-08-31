
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
// lcd_hd44780.h
// Dopasowane do zestawu uruchomieniowego STM32F0DISCOVERY by Mesho
//
// Zajrzyj na: atmegan.blogspot.com
//##############################################################################

#include <stm32f0xx_gpio.h>
#include <stm32f0xx_rcc.h>

//**********  Ustawienia pin�w wyswietlacza     **********//
#define LCD_GPIO GPIOB //port
#define LCD_CLK_LINE RCC_AHBPeriph_GPIOB //linia zegarowa, zmienic litere portu

#define LCD_D4 GPIO_Pin_6
#define LCD_D5 GPIO_Pin_5
#define LCD_D6 GPIO_Pin_4
#define LCD_D7 GPIO_Pin_3

#define LCD_RS GPIO_Pin_9
#define LCD_RW GPIO_Pin_8
#define LCD_EN GPIO_Pin_7
//********** Koniec ustawien pin�w wyswietlacza **********//


#define HD44780_CLEAR                    0x01

#define HD44780_HOME                     0x02

#define HD44780_ENTRY_MODE               0x04
#define HD44780_EM_SHIFT_CURSOR          0
#define HD44780_EM_SHIFT_DISPLAY         1
#define HD44780_EM_DECREMENT             0
#define HD44780_EM_INCREMENT             2

#define HD44780_DISPLAY_ONOFF            0x08
#define HD44780_DISPLAY_OFF              0
#define HD44780_DISPLAY_ON               4
#define HD44780_CURSOR_OFF               0
#define HD44780_CURSOR_ON                2
#define HD44780_CURSOR_NOBLINK     		 0
#define HD44780_CURSOR_BLINK           	 1

#define HD44780_DISPLAY_CURSOR_SHIFT 	 0x10
#define HD44780_SHIFT_CURSOR             0
#define HD44780_SHIFT_DISPLAY            8
#define HD44780_SHIFT_LEFT               0
#define HD44780_SHIFT_RIGHT              4

#define HD44780_FUNCTION_SET       		 0x20
#define HD44780_FONT5x7                  0
#define HD44780_FONT5x10                 4
#define HD44780_ONE_LINE                 0
#define HD44780_TWO_LINE                 8
#define HD44780_4_BIT                    0
#define HD44780_8_BIT                    16

#define HD44780_CGRAM_SET                0x40

#define HD44780_DDRAM_SET                0x80

//############# Komendy uzywane w glownym programie #############
void LCDInit(void);
void LCDClear(void);
void LCDOutString(unsigned char * text);
void LCDXY(unsigned char x, unsigned char y);
void LCDOutInt(int n);
void LCDOutuint32(uint32_t u, uint8_t n);
void LCDOutint32(int32_t u, uint8_t n);
void LCDOutuint16(uint16_t u, uint8_t n);
void LCDOutint16(int16_t u, uint8_t n);
void LCDOutuint8(uint8_t u, uint8_t n);
void LCDOutint8(int8_t u, uint8_t n);
void LCDOutFloat(float f, uint8_t calk, uint8_t ulamk);
void LCDOutUFloat(float f, uint8_t calk, uint8_t ulamk);
void LCDOutDouble(double d, uint8_t calk, uint8_t ulamk);

//###############################################################

void LCDSendByte(unsigned char dataToWrite);
void LCDAddress(unsigned char commandToWrite);
void lcd_addchar (unsigned char chrNum, unsigned char n, const unsigned char *p);
