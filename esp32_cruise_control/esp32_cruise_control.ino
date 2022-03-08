

#include "pinout.h"
#include <WiFi.h>
#include <AsyncUDP.h> //引用以使用异步UDP
#include <WiFiAP.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <CommandParser.h>

// all of the template arguments below are optional, but it is useful to adjust them to save memory (by lowering the limits) or allow larger inputs (by increasing the limits)
// limit number of commands to at most 5
// limit number of arguments per command to at most 3
// limit length of command names to 10 characters
// limit size of all arguments to 15 bytes (e.g., the argument "\x41\x42\x43" uses 14 characters to represent the string but is actually only 3 bytes, 0x41, 0x42, and 0x43)
// limit size of response strings to 64 bytes
//typedef CommandParser<5, 3, 10, 15, 64> MyCommandParser;
typedef CommandParser<5, 3, 10, 15, 64> MyCommandParser;
MyCommandParser parser;

AsyncUDP  Udp;                      //创建UDP对象
unsigned int UdpPort = 3333; //本地端口号
IPAddress remoteUDP_Ip(192, 168, 4, 2);
const char *ssid = "test";
const char *password = "12345678";
Adafruit_MCP4725 dac;
unsigned int dac_val=2048;

void wifi_init(void) {
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
  char response[MyCommandParser::MAX_RESPONSE_SIZE];
  parser.processCommand((const char*)packet.data(), response);
}

void cmd_setdac(MyCommandParser::Argument *args, char *response) {
  dac_val = args[0].asUInt64;
  dac.setVoltage(dac_val, false);
  String cstr = String(dac_val);
  cstr = "dac_val = " + cstr ;
  char buf[cstr.length()+1];
  // string to char array, length should increase 1 for null termination
  cstr.toCharArray(buf, cstr.length()+1);
  // send udp could be length of 4
  Udp.writeTo((const uint8_t*)buf, cstr.length(), remoteUDP_Ip, UdpPort);
}

void setup() {
  // put your setup code here, to run once:
  //set the resolution to 12 bits (0-4096)
  analogReadResolution(12);
  wifi_init();
  while (!Udp.listen(UdpPort)) //等待udp监听设置成功
  {
  }
  Udp.onPacket(onPacketCallBack); //注册收到数据包事件
  Wire.begin(SDA, SCL);
  dac.begin(0x60);
  dac.setVoltage(2048, false);
  
  parser.registerCommand("setdac", "u", &cmd_setdac); // two int64_t arguments
  char response[MyCommandParser::MAX_RESPONSE_SIZE];
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned int A1 = analogRead(AIN1);
  float volt = (float)A1 / 4095 * 2.5;
  //  float volt = 1.23;
  //  this string has length of 4
  String cstr = String(volt, 2);
  cstr = "Ain1 = " + cstr + " V";
  char buf[cstr.length()+1];
  // string to char array, length should increase 1 for null termination
  cstr.toCharArray(buf, cstr.length()+1);
  // send udp could be length of 4
  Udp.writeTo((const uint8_t*)buf, cstr.length(), remoteUDP_Ip, UdpPort);
  delay(1000);
//  char response[MyCommandParser::MAX_RESPONSE_SIZE];
//  parser.processCommand("setdac 1024", response);
}
