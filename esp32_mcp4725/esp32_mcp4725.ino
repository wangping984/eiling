/**************************************************************************/
/*!
    @file     sinewave.pde
    @author   Adafruit Industries
    @license  BSD (see license.txt)

    This example will generate a sine wave with the MCP4725 DAC.

    This is an example sketch for the Adafruit MCP4725 breakout board
    ----> http://www.adafruit.com/products/935

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!
*/
/**************************************************************************/
#include <Wire.h>
#include <Adafruit_MCP4725.h>
// i2c data/clk could be any gpio pins
#define I2C_SDA 0
#define I2C_SCL 1

Adafruit_MCP4725 dac;

void setup(void) {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.println("Hello!");

  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  dac.begin(0x60);

  Serial.println("voltages:0,1,2,3,4,5 volts");
}

uint16_t volt2word(float input) {
  uint16_t dacword;
  if(input >= 0 && input <=5) {
    
  } else {
    return 0;
  }
  
  return dacword;
}

void loop(void) {
  uint16_t DACword;
  float volt = 0;
  DACword = volt2word(volt);
  dac.setVoltage(0, false);
  delay(2000);
  volt = 1;
  DACword = volt2word(volt);
  dac.setVoltage(2048, false);
  delay(2000);
}
