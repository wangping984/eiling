#include "pinout.h"
#include <WiFi.h>
#include <AsyncUDP.h> //引用以使用异步UDP
#include <WiFiAP.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <CommandParser.h>
#include <WebServer.h>
#include <Update.h>
#include <Adafruit_GPS.h>
#include <EasyBuzzer.h>
#include "ota.h"
#include <PID_v1.h>

// all of the template arguments below are optional, but it is useful to adjust them to save memory (by lowering the limits) or allow larger inputs (by increasing the limits)
// limit number of commands to at most 5
// limit number of arguments per command to at most 3
// limit length of command names to 10 characters
// limit size of all arguments to 15 bytes (e.g., the argument "\x41\x42\x43" uses 14 characters to represent the string but is actually only 3 bytes, 0x41, 0x42, and 0x43)
// limit size of response strings to 64 bytes
// typedef CommandParser<5, 3, 10, 15, 64> MyCommandParser;
typedef CommandParser<5, 3, 10, 15, 64> MyCommandParser;
MyCommandParser parser;

AsyncUDP Udp;                //创建UDP对象
unsigned int UdpPort = 3333; //本地端口号
IPAddress remoteUDP_Ip(192, 168, 4, 2);
const char *host = "esp32";
const char *ssid = "test";
const char *password = "12345678";
Adafruit_MCP4725 dac;
int dac_val = 2048;
byte key_val = 72; // 01001000

// 5Hz for GPS update rate [0]
// 2Hz detect rate for keypad [1]
unsigned long startMillis[3]; // some global variables available anywhere in the program
unsigned long currentMillis[3];
const unsigned long period[3] = {200, 500, 1000};

int state_cc = 0;
int state_prev_cc = 0;
float speed_set = 0;

bool key_cc = false;
bool key_cancel = false;
bool key_set = false;
bool key_res = false;
bool key_inc = false;
bool key_dec = false;
bool GPS_fixed = false;
bool pedal_down = false;
bool pedal_up = false;

bool debug_sm = false;
bool debug_keypad = false;
bool debug_ota = false;
bool debug_adccal = false;
bool debug_gps = false;
bool debug_pid = false;
bool debug_adda = false;
bool debug_loop = false;

String cstr;
char buf[100];

#define GPSSerial Serial1
// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

float speed_cur = 0;

char terminateChar = '\n';       // 建立终止字符
const int bufferLength = 100;    // 定义缓存大小为10个字节
char serialBuffer[bufferLength]; // 建立字符数组用于缓存

// Define Variables we'll be connecting to
double Setpoint, Input, Output;

// Specify the links and initial tuning parameters
double Kp = 200, Ki = 500, Kd = 100;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

void SM_cc()
{
  state_prev_cc = state_cc;
  switch (state_cc)
  {
  case 0: // INIT state
    state_cc = 1;
    break;

  case 1: // pass thru state
    if (key_cc && GPS_fixed)
    {
      state_cc = 2;
    }
    pedal_down = false;
    pedal_up = false;
    break;

  case 2: // pre_cruise state
    pedal_down = false;
    pedal_up = false;
    if (key_set && GPS_fixed && speed_cur > 4)
    {
      state_cc = 3;
      speed_set = speed_cur;
      EasyBuzzer.beep(
          1000, // Frequency in hertz(HZ).
          200,  // On Duration in milliseconds(ms).
          200,  // Off Duration in milliseconds(ms).
          1,    // The number of beeps per cycle.
          0,    // Pause duration.
          2     // The number of cycle.
                // callback       // [Optional] Function to call when done.
      );
    }
    if (key_res && GPS_fixed && speed_set != 0 && speed_cur > 4)
    {
      state_cc = 3;
      EasyBuzzer.beep(
          1000, // Frequency in hertz(HZ).
          200,  // On Duration in milliseconds(ms).
          200,  // Off Duration in milliseconds(ms).
          1,    // The number of beeps per cycle.
          0,    // Pause duration.
          2     // The number of cycle.
                // callback       // [Optional] Function to call when done.
      );
    }
    if (GPS_fixed == false || key_cc == true)
    {
      state_cc = 1;
    }
    break;

  case 3: // cruising state
    if (GPS_fixed == false)
    {
      state_cc = 1;
      EasyBuzzer.beep(
          1000, // Frequency in hertz(HZ).
          200,  // On Duration in milliseconds(ms).
          200,  // Off Duration in milliseconds(ms).
          1,    // The number of beeps per cycle.
          0,    // Pause duration.
          2     // The number of cycle.
                // callback       // [Optional] Function to call when done.
      );
    }
    if (key_cancel || pedal_down || speed_cur < 4)
    {
      state_cc = 2;
      EasyBuzzer.beep(
          1000, // Frequency in hertz(HZ).
          200,  // On Duration in milliseconds(ms).
          200,  // Off Duration in milliseconds(ms).
          1,    // The number of beeps per cycle.
          0,    // Pause duration.
          2     // The number of cycle.
                // callback       // [Optional] Function to call when done.
      );
    }
    break;
  default: // error state, return to state_cc = 0
    state_cc = 0;
    break;
  }
}

void wifi_init(void)
{
  Serial.begin(115200);
  Serial.println("Configuring access point...");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("Wait 100 ms for AP_START...");
  delay(100);

  Serial.println("Set softAPConfig");
  IPAddress Ip(192, 168, 4, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void onPacketCallBack(AsyncUDPPacket packet)
{

  Serial.print("UDP数据包来源类型: ");
  Serial.println(packet.isBroadcast() ? "广播数据" : (packet.isMulticast() ? "组播" : "单播"));
  Serial.print("远端地址及端口号: ");
  Serial.print(packet.remoteIP());
  Serial.print(":");
  Serial.println(packet.remotePort());
  Serial.print("目标地址及端口号: ");
  Serial.print(packet.localIP());
  Serial.print(":");
  Serial.println(packet.localPort());
  Serial.print("数据长度: ");
  Serial.println(packet.length());
  Serial.print("数据内容: ");
  Serial.write(packet.data(), packet.length());
  Serial.println();
  packet.printf("Got %u bytes of data", packet.length());
  Udp.writeTo(packet.data(), packet.length(), remoteUDP_Ip, UdpPort);
  parser.processCommand((const char *)packet.data(), response);
}

void cmd_setdac(MyCommandParser::Argument *args, char *response)
{
  dac_val = args[0].asUInt64;
  dac.setVoltage(dac_val, false);
  cstr = String(dac_val);
  cstr = "dac_val = " + cstr;
  // string to char array, length should increase 1 for null termination
  cstr.toCharArray(buf, cstr.length() + 1);
  // send udp could be length of 4
  Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
}

void cmd_debug(MyCommandParser::Argument *args, char *response)
{
  String arg0;
  int arg1;
  arg0 = args[0].asString;
  arg1 = args[1].asInt64;
  if (arg0 == "kp")
  {
    Kp = (double)arg1 / 10;
  }
  if (arg0 == "ki")
  {
    Ki = (double)arg1 / 10;
  }
  if (arg0 == "kd")
  {
    Kd = (double)arg1 / 10;
  }
  if (arg1 != 0)
  {
    if (arg0 == "SM")
      debug_sm = true;
    if (arg0 == "keyp")
      debug_keypad = true;
    if (arg0 == "ota")
    {
      debug_ota = true;
      ota_start();
    }
    if (arg0 == "adc_cal")
    {
      debug_adccal = true;
    }
    if (arg0 == "gps")
    {
      debug_gps = true;
    }
    if (arg0 == "pid")
    {
      debug_pid = true;
    }
    if (arg0 == "adda")
    {
      debug_adda = true;
    }
    if (arg0 == "loop")
    {
      debug_loop = true;
    }
  }
  else
  {
    if (arg0 == "SM")
      debug_sm = false;
    if (arg0 == "keyp")
      debug_keypad = false;
    if (arg0 == "ota")
    {
      debug_ota = false;
      ota_stop();
    }
    if (arg0 == "adc_cal")
    {
      debug_adccal = false;
    }
    if (arg0 == "gps")
    {
      debug_gps = false;
    }
    if (arg0 == "pid")
    {
      debug_pid = false;
    }
    if (arg0 == "adda")
    {
      debug_adda = false;
    }
    if (arg0 == "loop")
    {
      debug_loop = false;
    }
  }
}

byte shiftIn(int myDataPin, int myClockPin)
{
  int i;
  int temp = 0;
  int pinState;
  byte myDataIn = 0;

  //  pinMode(myClockPin, OUTPUT);
  //
  //  pinMode(myDataPin, INPUT);
  // we will be holding the clock pin high 8 times (0,..,7) at the
  // end of each time through the for loop

  // at the beginning of each loop when we set the clock low, it will
  // be doing the necessary low to high drop to cause the shift
  // register's DataPin to change state based on the value
  // of the next bit in its serial information flow.
  // The register transmits the information about the pins from pin 7 to pin 0
  // so that is why our function counts down
  for (i = 7; i >= 0; i--)
  {
    digitalWrite(myClockPin, 0);
    delayMicroseconds(100);
    temp = digitalRead(myDataPin);
    if (temp)
    {
      pinState = 1;
      // set the bit to 0 no matter what
      myDataIn = myDataIn | (1 << i);
    }
    else
    {
      // turn it off -- only necessary for debugging
      // print statement since myDataIn starts as 0
      pinState = 0;
    }
    // Debugging print statements
    // Serial.print(pinState);
    // Serial.print("     ");
    // Serial.println (dataIn, BIN);
    digitalWrite(myClockPin, 1);
    delayMicroseconds(100);
  }
  // debugging print statements whitespace
  // Serial.println();
  // Serial.println(myDataIn, BIN);
  return myDataIn;
}

void key_pressed_detect()
{
  digitalWrite(KEY_LOAD, 1);
  // set it to 1 to collect parallel data, wait
  delayMicroseconds(20);
  // set it to 0 to transmit data serially
  digitalWrite(KEY_LOAD, 0);
  // while the shift register is in serial mode
  // collect each shift register into a byte
  // the register attached to the chip comes in first
  key_val = shiftIn(KEY_DAT, KEY_CLK);
  key_cc = false;
  key_cancel = false;
  key_set = false;
  key_res = false;
  key_inc = false;
  key_dec = false;

  byte onehot_key = ~key_val;
  // bit 0, 1, left
  // bit 1, 2, right
  // bit 2, 4, down, key_dec
  // bit 3, 8, up, key_inc
  // bit 4, 16, start, key_set
  // bit 5, 32, select, key_cc
  // bit 6, 64, A, key_cancel
  // bit 7, 128, B, key_res
  switch (onehot_key)
  {
  case 32: // cruise
    key_cc = true;
    EasyBuzzer.singleBeep(1000, 50);
    break;
  case 64: // cancel
    key_cancel = true;
    EasyBuzzer.singleBeep(1000, 50);
    break;
  case 16: // set
    key_set = true;
    EasyBuzzer.singleBeep(1000, 50);
    break;
  case 128: // restore
    key_res = true;
    EasyBuzzer.singleBeep(1000, 50);
    break;
  case 8: // increase
    key_inc = true;
    EasyBuzzer.singleBeep(1000, 50);
    break;
  case 4: // decrease
    key_dec = true;
    EasyBuzzer.singleBeep(1000, 50);
    break;
  }
}

void debug_info()
{
  if (debug_sm == true)
  {
    cstr = String(state_cc);
    cstr = "state_cc = " + cstr;

    // string to char array, length should increase 1 for null termination
    cstr.toCharArray(buf, cstr.length() + 1);
    Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
    if (key_cc == true)
    {
      cstr = "key_cc = 1";
    }
    else
    {
      cstr = "key_cc = 0";
    }

    // string to char array, length should increase 1 for null termination
    cstr.toCharArray(buf, cstr.length() + 1);
    Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
  }
  if (debug_keypad == true)
  {
    cstr = String(key_val, BIN);
    cstr = "key val = " + cstr;

    // string to char array, length should increase 1 for null termination
    cstr.toCharArray(buf, cstr.length() + 1);
    Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
  }
  if (debug_adda == true)
  {
    Serial.print("\nAD DA debug: Ain1 = ");
    Serial.print(analogRead(AIN1));
    Serial.print("  Ain2 = ");
    Serial.print(analogRead(AIN2));
    Serial.print("  Aout1 = ");
    Serial.print(analogRead(AOUT1));

    cstr = "\nAD DA debug: Ain1 = " + String(analogRead(AIN1)) + "  Ain2 = " + String(analogRead(AIN2)) + "  Aout1 = " + String(analogRead(AOUT1));
    cstr.toCharArray(buf, cstr.length() + 1);
    Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
  }
  if(debug_loop == true)
  {
    Serial.print("\ndebug loop: speed cur = ");
    Serial.print(speed_cur);
    Serial.print("  set = ");
    Serial.print(speed_set);
    Serial.print("  dac_val = ");
    Serial.print(int(Output));
    Serial.print("\n");

    cstr = "debug loop: speed cur = " + String(speed_cur, 2) + "  set = " + String(speed_set, 2) + "  dac_val = " + String(int(Output));
    cstr.toCharArray(buf, cstr.length() + 1);
    Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
  }
  if (debug_pid == true)
  {
    Serial.print("\nPID debug: Kp = ");
    Serial.print(Kp);
    Serial.print("  Ki = ");
    Serial.print(Ki);
    Serial.print("  Kd = ");
    Serial.print(Kd);
    Serial.print("\n");

    cstr = "\nPID debug: Kp = " + String(Kp, 2) + "  Ki = " + String(Ki, 2) + "  Kd = " + String(Kd, 2) + "\n";
    cstr.toCharArray(buf, cstr.length() + 1);
    Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
  }
  if (debug_gps == true)
  {
    Serial.print("\nTime: ");
    if (GPS.hour < 10)
    {
      Serial.print('0');
    }
    Serial.print(GPS.hour, DEC);
    Serial.print(':');
    if (GPS.minute < 10)
    {
      Serial.print('0');
    }
    Serial.print(GPS.minute, DEC);
    Serial.print(':');
    if (GPS.seconds < 10)
    {
      Serial.print('0');
    }
    Serial.print(GPS.seconds, DEC);
    Serial.print('.');
    if (GPS.milliseconds < 10)
    {
      Serial.print("00");
    }
    else if (GPS.milliseconds > 9 && GPS.milliseconds < 100)
    {
      Serial.print("0");
    }
    Serial.println(GPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC);
    Serial.print('/');
    Serial.print(GPS.month, DEC);
    Serial.print("/20");
    Serial.println(GPS.year, DEC);
    Serial.print("Fix: ");
    Serial.print((int)GPS.fix);
    Serial.print(" quality: ");
    Serial.println((int)GPS.fixquality);
    if (GPS.fix)
    {
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4);
      Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4);
      Serial.println(GPS.lon);
      Serial.print("Speed (knots): ");
      Serial.println(GPS.speed);
      Serial.print("Angle: ");
      Serial.println(GPS.angle);
      Serial.print("Altitude: ");
      Serial.println(GPS.altitude);
      Serial.print("Satellites: ");
      Serial.println((int)GPS.satellites);
    }
  }
}

void pass_thru()
{
  unsigned int A1 = analogRead(AIN1);
  unsigned int A2 = analogRead(AIN2);
  float ratio = float(A1) / float(A2);
  // if (ratio > 1.2 || ratio < 0.8)
  if (false)
  {
    // beep(1000, 2);
    dac.setVoltage(622, false); // 0.76V/5V*(2^12-1)=622
  }
  else
  {
    float dacvolt = float(A1) * 1.122 + 10.8;
    if (dacvolt > 3767)
      dac.setVoltage(3767, false);
    else if (dacvolt < 622)
      dac.setVoltage(622, false);
    else
      dac.setVoltage((unsigned int)dacvolt, false);
    // unsigned int Ao = analogRead(Aout1);
    // ratio = float(Ao) / float(A1);
    // if (ratio > 1.05 || ratio < 0.95)
    // {
    //   beep(500, 2);
    // }
  }
}

void adc_cal()
{
  unsigned int avg = 10;
  unsigned int A1 = 0;
  unsigned int sum = 0;
  Serial.println("start ADC calibration...");
  for (dac_val = 0; dac_val <= 4095; dac_val++)
  {
    dac.setVoltage(dac_val, false); // 0.76V/5V*(2^12-1)=622
    delay(1);
    sum = 0;
    for (int index = 0; index < avg; index++)
    {
      A1 = analogRead(AIN1);
      sum = sum + A1;
    }

    Serial.print("DAC = ");
    Serial.print(dac_val, DEC);
    Serial.print(" ");
    Serial.print("ADC = ");
    Serial.print(sum, DEC);
    Serial.println();
  }
  for (dac_val = 4095; dac_val >= 0; dac_val--)
  {
    dac.setVoltage(dac_val, false); // 0.76V/5V*(2^12-1)=622
    delay(1);
    sum = 0;
    for (int index = 0; index < avg; index++)
    {
      A1 = analogRead(AIN1);
      sum = sum + A1;
    }

    Serial.print("DAC = ");
    Serial.print(dac_val, DEC);
    Serial.print(" ");
    Serial.print("ADC = ");
    Serial.print(sum, DEC);
    Serial.println();
  }
}
void GPS_init()
{
  GPSSerial.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
  //   set GPS 115200 baud rate
  byte message[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x31, 0x2C, 0x35, 0x2A, 0x31, 0x39, 0x0D, 0x0A};
  GPSSerial.write(message, sizeof(message));
  GPSSerial.flush();
  GPSSerial.begin(115200, SERIAL_8N1, GPS_TX, GPS_RX);
  // set GPS 5Hz update rate
  byte message0[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x32, 0x2C, 0x32, 0x30, 0x30, 0x2A, 0x31, 0x44, 0x0D, 0x0A};
  GPSSerial.write(message0, sizeof(message0));

  //   关闭GLL
  // 0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A
  // 关闭GSA
  // 0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A
  // 关闭GSV
  // 0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A
  // 关闭VGT
  // 0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A
  // 关闭ZDA
  // 0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A
  // 关闭TXT
  // 0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A
  byte message1[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A};
  GPSSerial.write(message1, sizeof(message1));
  byte message2[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A};
  GPSSerial.write(message2, sizeof(message2));
  byte message3[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A};
  GPSSerial.write(message3, sizeof(message3));
  byte message4[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A};
  GPSSerial.write(message4, sizeof(message4));
  byte message5[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A};
  GPSSerial.write(message5, sizeof(message5));
  byte message6[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A};
  GPSSerial.write(message6, sizeof(message6));
  //   关闭GGA
  // 0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A
  //  byte message7[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x33, 0x2C, 0x30, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2C, 0x2A, 0x33, 0x32, 0x0D, 0x0A};
  //  GPSSerial.write(message7, sizeof(message7));

  //  Serial1.setRxBufferSize(1024);
}

void GPS_parse()
{
  if (GPSSerial.available())
  {
    if (debug_gps == true)
    {
      Serial.print(millis(), DEC);
      Serial.print("   availabe: ");
      Serial.print(GPSSerial.available());
      Serial.print("    ");

      cstr = String(millis()) + "   availabe: " + String(GPSSerial.available()) + "    ";
      cstr.toCharArray(buf, cstr.length() + 1);
      Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
    }

    size_t len = GPSSerial.readBytesUntil(terminateChar, serialBuffer, bufferLength); // 将接收到的信息使用read读取
    if (debug_gps == true)
    {
      for (int i = 0; i < bufferLength; i++)
      {                                // 然后通过串口监视器输出readBytesUntil
        Serial.print(serialBuffer[i]); // 函数所读取的信息
      }
      Serial.print("\n");

      Udp.writeTo((const uint8_t *)serialBuffer, len, remoteUDP_Ip, UdpPort);
    }

    GPS.parse(serialBuffer);
  }
  if (GPS.fix)
  {
    GPS_fixed = true;
    speed_cur = (GPS.speed) * 1.852; // knot to km/s
  }
}

void cruising_control()
{
  // set up PID loop,
  // input parameters: speed current, speed set
  // output parameters: dac voltage
  Setpoint = (double)speed_set;
  Input = (double)speed_cur;
  // PID parameters could adjusted by debug command
  myPID.SetTunings(Kp, Ki, Kd);
  // turn the PID on
  myPID.SetMode(AUTOMATIC);
  myPID.Compute();
  // Output;
  dac.setVoltage((unsigned int)Output, false);
  pedal_down_dect();
}

void pedal_down_dect()
{
  if (analogRead(AIN1) < 700)
  {
    pedal_up = true;
    pedal_down = false;
  }

  if (analogRead(AIN1) > 1000 && pedal_up == true)
  {
    pedal_up = false;
    pedal_down = true;
  }
}

void PID_init()
{
  myPID.SetOutputLimits(622, 3200);
  myPID.SetSampleTime(200); // 200ms PID update rate
}

void PID_off()
{
  // turn off PID control
  myPID.SetMode(MANUAL);
}

void setup()
{
  // put your setup code here, to run once:
  pinMode(AOUT1, INPUT); // AOUT1 is pin on toECU header6 and pin on ESP ADin for monitor DAC output
  pinMode(AIN1, INPUT);
  pinMode(AIN2, INPUT);
  EasyBuzzer.setPin(BUZZER);
  // beep 0.1s when boot up
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, 1);
  delay(50);
  digitalWrite(BUZZER, 0);
  // set the resolution to 12 bits (0-4095)
  analogReadResolution(12);
  wifi_init();
  while (!Udp.listen(UdpPort)) //等待udp监听设置成功
  {
  }
  Udp.onPacket(onPacketCallBack); //注册收到数据包事件

  Wire.begin(SDA, SCL);
  dac.begin(0x60);
  dac.setVoltage(622, false); // 0.76V/5V*(2^12-1)=622

  parser.registerCommand("setdac", "u", &cmd_setdac);
  parser.registerCommand("debug", "si", &cmd_debug);

  // define pin modes
  pinMode(KEY_LOAD, OUTPUT);
  pinMode(KEY_CLK, OUTPUT);
  pinMode(KEY_DAT, INPUT);
  digitalWrite(KEY_LOAD, 1);
  digitalWrite(KEY_CLK, 0);

  GPS_init();
  PID_init();

  startMillis[0] = millis(); // initial start time
  startMillis[1] = millis(); // initial start time
}

void loop()
{

  if (debug_ota == true)
  {
    server.handleClient();
  }
  else
  {
    if (debug_adccal == true)
    {
      adc_cal();
    }
    else
    {
      /* Always call this function in the loop for EasyBuzzer to work. */
      EasyBuzzer.update();

      GPS_parse();
      if (state_cc != 3)
      {
        PID_off();
        pass_thru();
      }
      else // state_cc == 3, cruising
      {
        cruising_control();
      }

      // debug info update rate
      if (millis() - startMillis[0] >= 1000) // test whether the period has elapsed
      {
        startMillis[0] = millis();
        debug_info();
        EasyBuzzer.stopBeep();
      }
      // [1] for keypad detect rate
      if (millis() - startMillis[1] >= period[1]) // test whether the period has elapsed
      {
        startMillis[1] = millis();
        key_pressed_detect();
        SM_cc();
      }
    }
  }
}
