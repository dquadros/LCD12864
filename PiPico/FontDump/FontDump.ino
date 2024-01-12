/*
  Test access to the font ROM in the display
  See the Font ROM datasheet for explanations
  Uses Pico board support by Earle F. Phillhoower III
*/

#include <SPI.h>

// Connections
const int pinROM_CS = 22;
const int pinROM_SCL = 18;
const int pinROM_SI = 19;
const int pinROM_SO = 16;
#define ROM_SPI SPI

// Initial Address of 7x8 ASCII Font
const uint32_t ADDR_ASCII_7x8 = 0x66C0;

// Read Data command
const byte ROM_READ_DATA = 0x03;

// Chars to Dump
const int FIRST_CHAR = '0';
const int LAST_CHAR  = '9';

// SPI Configuration
SPISettings SPI_CONFIG(10000000, MSBFIRST, SPI_MODE0);

// Initialization
void setup() {
  // Set up pins
  ROM_SPI.setSCK(pinROM_SCL);
  ROM_SPI.setRX(pinROM_SO);
  ROM_SPI.setTX(pinROM_SI);
  ROM_SPI.begin();
  pinMode(pinROM_CS, OUTPUT);
  digitalWrite(pinROM_CS, HIGH);

  // Init serial
  while (!Serial) {
    delay(100);
  }
  Serial.begin(115200);

  // Dump font
  Serial.println("\nDumping font...");
  for (int ch = FIRST_CHAR; ch <= LAST_CHAR; ch++) {
    byte ch_data[7];
    uint32_t addr = ADDR_ASCII_7x8 + 8*(ch - ' ');  // 8 bytes per char, last is empty
    ROM_read (addr, ch_data, 7);
    Serial.println();
    // Each byte is a column, from left to right
    // Least significant bit corresponds to upper pixel
    for (int mask = 0x01; mask < 0x100; mask = mask << 1) {
      for (int c = 0; c < 7; c++) {
        Serial.print ((ch_data[c] & mask) ? '*' : '.');
      }
      Serial.println();
    }
  }
}

// Main loop
void loop() {
  delay(100); // nothing to do
}

// Read size bytes form the ROM, starting at addr
void ROM_read (uint32_t addr, byte *dest, int size) {
  // Start transaction
  ROM_SPI.beginTransaction(SPI_CONFIG);
  digitalWrite(pinROM_CS, LOW);

  // Send read data command and address
  ROM_SPI.transfer(ROM_READ_DATA);
  ROM_SPI.transfer(addr >> 16);
  ROM_SPI.transfer((addr >> 8) & 0xFF);
  ROM_SPI.transfer(addr & 0xFF);

  // Read the bytes
  for (int i = 0; i < size; i++) {
    dest[i] = ROM_SPI.transfer(0);
  }

  // End transaction
  digitalWrite(pinROM_CS, HIGH);
  ROM_SPI.endTransaction();
}
