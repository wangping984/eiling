
#include <Wire.h>
#include <Adafruit_MCP4725.h>

// pinout for esp32c3 mother board
#define I2C_SDA 7
#define I2C_SCL 6
#define Ain1 0
#define Ain2 1
#define gps_tx 12
#define gps_rx 18
#define Aout1 2
#define buzzer 10
#define key_data 4
#define key_clk 8
#define key_load 9


Adafruit_MCP4725 dac;

void setup(void) {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  analogReadResolution(12);

  Serial.println("Hello!");

  // For Adafruit MCP4725A1 the address is 0x62 (default) or 0x63 (ADDR pin tied to VCC)
  // For MCP4725A0 the address is 0x60 or 0x61
  // For MCP4725A2 the address is 0x64 or 0x65
  dac.begin(0x60);
  digitalWrite(buzzer, 0);
  dac.setVoltage(622, false); // 0.76V/5V*(2^12-1)=622

}

void beep(unsigned int duration, byte repeat) {
  for (byte i = 0; i < repeat; i++) {
    digitalWrite(buzzer, 1);
    delay(duration);
    digitalWrite(buzzer, 0);
    delay(duration);
  }
}

void loop(void) {
  unsigned int A1 = analogRead(Ain1);
  unsigned int A2 = analogRead(Ain2);
  float ratio = float(A1) / float(A2);
  if (ratio > 1.05 || ratio < 0.95) {
    beep(1000, 2);
    dac.setVoltage(622, false); // 0.76V/5V*(2^12-1)=622
  } else {
    dac.setVoltage(A1 * 2, false);
    unsigned int Ao = analogRead(Aout1);
    ratio = float(Ao) / float(A1);
    if (ratio > 1.05 || ratio < 0.95) {
      beep(500,2);
    }
  }
}
