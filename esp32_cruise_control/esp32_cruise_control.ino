

#include "pinout.h"
#include <WiFi.h>
#include <AsyncUDP.h> //引用以使用异步UDP
#include <WiFiAP.h>

AsyncUDP  Udp;                      //创建UDP对象
unsigned int UdpPort = 3333; //本地端口号
IPAddress remoteUDP_Ip(192, 168, 4, 2);
const char *ssid = "test";
const char *password = "12345678";

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

  //  packet.print("reply data");
  //  broadcastPort = packet.remotePort();
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
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned int A1 = analogRead(AIN1);
  //  float volt = (float)A1/4095*2.5;
  float volt = 1.23;
//  this string has length of 4
  String cstr = String(volt, 2);
  char buf[5];
  // string to char array, length should increase 1 for null termination
  cstr.toCharArray(buf, 5);
  //  char cstr[4];
  //  utoa(A1, cstr, 10);
// send udp could be length of 4
  Udp.writeTo((const uint8_t*)buf, 4, remoteUDP_Ip, UdpPort);
  delay(1000);
}
