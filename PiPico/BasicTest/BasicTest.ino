/*
  Simple test for LCD12864-06D display connected to a Raspberry Pi Pico
  Uses Pico board support by Earle F. Phillhoower III
*/

#include <SPI.h>

// Connections
const int pinCS = 17;
const int pinRES = 20;
const int pinRS = 21;
const int pinSCL = 18;
const int pinSI = 19;
#define LCD_SPI SPI

// RS pin convention
#define DATA HIGH
#define CMD  LOW

// SPI Configuration
SPISettings SPI_CONFIG(10000000, MSBFIRST, SPI_MODE3);

// ST7565R Commands
#define ST7565R_DISPLAY_OFF 0xAE
#define ST7565R_DISPLAY_ON  0xAF
#define ST7565R_SET_DISP_START_LINE 0x40
#define ST7565R_SET_PAGE    0xB0
#define ST7565R_SET_COLUMN_UPPER  0x10
#define ST7565R_SET_COLUMN_LOWER  0x00
#define ST7565R_SET_ADC_NORMAL  0xA0
#define ST7565R_SET_ADC_REVERSE 0xA1
#define ST7565R_SET_ALLPTS_NORMAL 0xA4
#define ST7565R_SET_ALLPTS_ON  0xA5
#define ST7565R_SET_BIAS_9 0xA2 
#define ST7565R_SET_BIAS_7 0xA3
#define ST7565R_RMW  0xE0
#define ST7565R_RMW_CLEAR 0xEE
#define ST7565R_INTERNAL_RESET  0xE2
#define ST7565R_SET_COM_NORMAL  0xC0
#define ST7565R_SET_COM_REVERSE  0xC8
#define ST7565R_SET_POWER_CONTROL  0x28
#define ST7565R_SET_RESISTOR_RATIO  0x20
#define ST7565R_SET_VOLUME_FIRST  0x81
#define ST7565R_SET_VOLUME_SECOND  0
#define ST7565R_SET_STATIC_OFF  0xAC
#define ST7565R_SET_STATIC_ON  0xAD
#define ST7565R_SET_STATIC_REG  0x0
#define ST7565R_SET_BOOSTER_FIRST  0xF8
#define ST7565R_SET_BOOSTER_234  0
#define ST7565R_SET_BOOSTER_5  1
#define ST7565R_SET_BOOSTER_6  3
#define ST7565R_NOP  0xE3
#define ST7565R_TEST  0xF0

// Screen size
#define LCD_WIDTH    128
#define LCD_HEIGHT   64

// Display initialization commands
byte cmdInit[] =
{
  // cmd                              // delay
  ST7565R_SET_BIAS_7,                 0, 
  ST7565R_SET_ADC_NORMAL,             0,
  ST7565R_SET_COM_NORMAL,             0,
  ST7565R_SET_DISP_START_LINE,        0,
  ST7565R_SET_POWER_CONTROL | 0x4,    50,
  ST7565R_SET_POWER_CONTROL | 0x6,    50,
  ST7565R_SET_POWER_CONTROL | 0x7,    10,
  ST7565R_SET_RESISTOR_RATIO | 0x6,   0,
  ST7565R_DISPLAY_ON,                 0,
  ST7565R_SET_ALLPTS_NORMAL,          0,
  ST7565R_SET_VOLUME_FIRST,           0,
  ST7565R_SET_VOLUME_SECOND | 13,     0
};

// Digits-only (0 to 9) font
byte gc[][7] =
{
  {0x7C, 0x86, 0x8A, 0x92, 0xA2, 0xC2, 0x7C}  // 0
 ,{0x02, 0x02, 0x42, 0xFE, 0x02, 0x02, 0x02}  // 1
 ,{0x42, 0x86, 0x8A, 0x92, 0xA2, 0x42, 0x02}  // 2
 ,{0x44, 0x82, 0x92, 0x92, 0x92, 0x92, 0x6C}  // 3
 ,{0x08, 0x18, 0x28, 0x48, 0x88, 0xFE, 0x0A}  // 4
 ,{0xE4, 0xA2, 0xA2, 0xA2, 0xA2, 0x92, 0x8C}  // 5
 ,{0x3C, 0x52, 0x92, 0x92, 0x92, 0x92, 0x4C}  // 6
 ,{0x82, 0x84, 0x88, 0x90, 0xA0, 0xC0, 0x80}  // 7
 ,{0x6C, 0x92, 0x92, 0x92, 0x92, 0x92, 0x6C}  // 8
 ,{0x64, 0x92, 0x92, 0x92, 0x92, 0x94, 0x78}  // 9
};

// Initialization
void setup() {
  LCD_SPI.setSCK(pinSCL);
  LCD_SPI.setTX(pinSI);
  LCD_SPI.begin();
  pinMode(pinRES, OUTPUT);
  digitalWrite(pinRES, LOW);
  pinMode(pinRS, OUTPUT);
  digitalWrite(pinRS, CMD);
  pinMode(pinCS, OUTPUT);
  digitalWrite(pinCS, HIGH);
  Display_init();
}

// Main Loop
void loop() {
  int d = 0;
  for (int l = 0; l < 8; l++)
  {
    for (int c = 0; c < 16; c++)
    {
      Display_write (l, c, d);
      if (++d == 10)
        d = 0;
      delay (500);
    }
  }
  delay (3000);
  Display_clear();
}

// Initialize the display
void Display_init()
{
  // Reset controller
  digitalWrite (pinRES, LOW);
  delay (500);
  digitalWrite (pinRES, HIGH);
  delay (100);
  
  // Configure controller and clean the screen
  Display_sendcmd (cmdInit, sizeof(cmdInit)/2);
  Display_clear();
}

// Clear display
void Display_clear()
{
  LCD_SPI.beginTransaction(SPI_CONFIG);

  // write zeroes in all columns in all pages
  for (byte p = 0; p < LCD_HEIGHT/8; p++) {
    Display_sendcmd(ST7565R_SET_PAGE | p);
    Display_sendcmd(ST7565R_SET_COLUMN_UPPER | 0);
    Display_sendcmd(ST7565R_SET_COLUMN_LOWER | 0);
    digitalWrite(pinCS, LOW);
    digitalWrite(pinRS, DATA);
    for (byte c = 0; c < LCD_WIDTH; c++) {
      LCD_SPI.transfer(0);
    }
    digitalWrite(pinCS, HIGH);
  }

  LCD_SPI.endTransaction();
}

// Writes a char at line l (0 to 7), colunm  c (0 to 15)
void Display_write (byte l, byte c, byte car)
{
  byte *pc = gc[car];
  c = c << 3; // 8 bytes per char

  LCD_SPI.beginTransaction(SPI_CONFIG);

  Display_sendcmd(ST7565R_SET_PAGE | (7-l));  // page numbered from bottom to top
  Display_sendcmd(ST7565R_SET_COLUMN_UPPER | (c >> 4));
  Display_sendcmd(ST7565R_SET_COLUMN_LOWER | (c & 0x0F));
  digitalWrite(pinCS, LOW);
  digitalWrite(pinRS, DATA);
  for (byte x = 0; x < 7; x++) {
    LCD_SPI.transfer(*pc++);
  }
  digitalWrite(pinCS, HIGH);

  LCD_SPI.endTransaction();
}

// Send a sequence of commands to the display, with delays
void Display_sendcmd (byte *cmd, int nCmds)
{
  LCD_SPI.beginTransaction(SPI_CONFIG);
  digitalWrite(pinCS, LOW);
  digitalWrite(pinRS, CMD);
  for (int i = 0; i < nCmds*2; i+=2)
  {
    LCD_SPI.transfer(cmd[i]);
    if (cmd[i+1] != 0) {
      delay(cmd[i+1]);
    }
  }
  digitalWrite(pinCS, HIGH);
  LCD_SPI.endTransaction();
}

// Send a command byte to the display
void Display_sendcmd (byte cmd)
{
  digitalWrite(pinCS, LOW);
  digitalWrite(pinRS, CMD);
  LCD_SPI.transfer(cmd);
  digitalWrite(pinCS, HIGH);
}
