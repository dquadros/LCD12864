/*
  Simple font use example for LCD12864-06D display connected to a Raspberry Pi Pico
  Write text using the 
  Uses Pico board support by Earle F. Phillhoower III
*/


#include <SPI.h>

// We are using the same SPI for the ST controller and the ROM
const int pinSCL = 18;
const int pinSI = 19;
const int pinSO = 16;
#define LCD_SPI SPI

// Connections
const int pinST_CS = 17;
const int pinST_RES = 20;
const int pinST_RS = 21;
const int pinROM_CS = 22;

// RS pin convention
#define DATA HIGH
#define CMD  LOW

// SPI Configurations
SPISettings SPI_CONFIG_ST(10000000, MSBFIRST, SPI_MODE3);
SPISettings SPI_CONFIG_ROM(10000000, MSBFIRST, SPI_MODE0);

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

// Contrast (0 to 63)
#define LCD_CONTRAST 10

// Display initialization commands
byte cmdInit[] =
{
  // cmd                                      // delay
  ST7565R_SET_BIAS_7,                         0, 
  ST7565R_SET_ADC_NORMAL,                     0,
  ST7565R_SET_COM_NORMAL,                     0,
  ST7565R_SET_DISP_START_LINE,                0,
  ST7565R_SET_POWER_CONTROL | 0x4,            50,
  ST7565R_SET_POWER_CONTROL | 0x6,            50,
  ST7565R_SET_POWER_CONTROL | 0x7,            10,
  ST7565R_SET_RESISTOR_RATIO | 0x6,           0,
  ST7565R_DISPLAY_ON,                         0,
  ST7565R_SET_ALLPTS_NORMAL,                  0,
  ST7565R_SET_VOLUME_FIRST,                   0,
  ST7565R_SET_VOLUME_SECOND | LCD_CONTRAST,   0
};

// Initial Address of 7x8 ASCII Font
const uint32_t ADDR_ASCII_7x8 = 0x66C0;

// Read Data command
const byte ROM_READ_DATA = 0x03;

// Texto para teste
const char *texto = "DQSoft 2024 ";

// Initialization
void setup() {
  LCD_SPI.setSCK(pinSCL);
  LCD_SPI.setTX(pinSI);
  LCD_SPI.setRX(pinSO);
  LCD_SPI.begin();

  pinMode(pinROM_CS, OUTPUT);
  digitalWrite(pinROM_CS, HIGH);

  pinMode(pinST_RES, OUTPUT);
  digitalWrite(pinST_RES, LOW);
  pinMode(pinST_RS, OUTPUT);
  digitalWrite(pinST_RS, CMD);
  pinMode(pinST_CS, OUTPUT);
  digitalWrite(pinST_CS, HIGH);
  Display_init();
}

// Main Loop
void loop() {
  static int i = 0;
  static int l = 0;
  static int c = 0;

  Display_write (l, c, texto[i]);
  delay(300);
  if (texto[++i] == 0) {
    i = 0;
  }
  if (++c == 16) {
    c = 0;
    if (++l == 8) {
      l = 0;
      Display_clear();
    }
  }
}

// Initialize the display
void Display_init()
{
  // Reset controller
  digitalWrite (pinST_RES, LOW);
  delay (500);
  digitalWrite (pinST_RES, HIGH);
  delay (100);
  
  // Configure controller and clean the screen
  Display_sendcmd (cmdInit, sizeof(cmdInit)/2);
  Display_clear();
}

// Clear display
void Display_clear()
{
  LCD_SPI.beginTransaction(SPI_CONFIG_ST);

  // write zeroes in all columns in all pages
  for (byte p = 0; p < LCD_HEIGHT/8; p++) {
    Display_sendcmd(ST7565R_SET_PAGE | p);
    Display_sendcmd(ST7565R_SET_COLUMN_UPPER | 0);
    Display_sendcmd(ST7565R_SET_COLUMN_LOWER | 0);
    digitalWrite(pinST_CS, LOW);
    digitalWrite(pinST_RS, DATA);
    for (byte c = 0; c < LCD_WIDTH; c++) {
      LCD_SPI.transfer(0);
    }
    digitalWrite(pinST_CS, HIGH);
  }

  LCD_SPI.endTransaction();
}

// Writes a char at line l (0 to 7), colunm  c (0 to 15)
void Display_write (byte l, byte c, byte ch)
{
  byte gc[7];
  c = c << 3; // 8 bytes per char

  // Get char from ROM
  uint32_t addr = ADDR_ASCII_7x8 + 8*(ch - ' ');  // 8 bytes per char, last is empty
  ROM_read (addr, gc, 7);

  // Flip vertically
  for (byte i = 0; i < 7; i++) {
    byte x = 0;
    for (byte j = 0; j < 8; j++) {
      if (gc[i] & (1 << j)) {
        x |= (1 << (7-j));
      }
    }
    gc[i] = x;
  }

  // Write in the display controller RAM
  LCD_SPI.beginTransaction(SPI_CONFIG_ST);

  Display_sendcmd(ST7565R_SET_PAGE | (7-l));  // page numbered from bottom to top
  Display_sendcmd(ST7565R_SET_COLUMN_UPPER | (c >> 4));
  Display_sendcmd(ST7565R_SET_COLUMN_LOWER | (c & 0x0F));
  digitalWrite(pinST_CS, LOW);
  digitalWrite(pinST_RS, DATA);
  for (byte x = 0; x < 7; x++) {
    LCD_SPI.transfer(gc[x]);
  }
  digitalWrite(pinST_CS, HIGH);

  LCD_SPI.endTransaction();
}

// Send a sequence of commands to the display, with delays
void Display_sendcmd (byte *cmd, int nCmds)
{
  LCD_SPI.beginTransaction(SPI_CONFIG_ST);
  digitalWrite(pinST_CS, LOW);
  digitalWrite(pinST_RS, CMD);
  for (int i = 0; i < nCmds*2; i+=2)
  {
    LCD_SPI.transfer(cmd[i]);
    if (cmd[i+1] != 0) {
      delay(cmd[i+1]);
    }
  }
  digitalWrite(pinST_CS, HIGH);
  LCD_SPI.endTransaction();
}

// Send a command byte to the display
void Display_sendcmd (byte cmd)
{
  digitalWrite(pinST_CS, LOW);
  digitalWrite(pinST_RS, CMD);
  LCD_SPI.transfer(cmd);
  digitalWrite(pinST_CS, HIGH);
}

// Read size bytes form the ROM, starting at addr
void ROM_read (uint32_t addr, byte *dest, int size) {
  // Start transaction
  LCD_SPI.beginTransaction(SPI_CONFIG_ROM);
  digitalWrite(pinROM_CS, LOW);

  // Send read data command and address
  LCD_SPI.transfer(ROM_READ_DATA);
  LCD_SPI.transfer(addr >> 16);
  LCD_SPI.transfer((addr >> 8) & 0xFF);
  LCD_SPI.transfer(addr & 0xFF);

  // Read the bytes
  for (int i = 0; i < size; i++) {
    dest[i] = LCD_SPI.transfer(0);
  }

  // End transaction
  digitalWrite(pinROM_CS, HIGH);
  LCD_SPI.endTransaction();
}

