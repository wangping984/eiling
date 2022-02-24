/*
  WiFiAccessPoint.ino creates a WiFi access point and provides a web server on it.

  Steps:
  1. Connect to the access point "yourAp"
  2. Point your web browser to http://192.168.4.1/H to turn the LED on or http://192.168.4.1/L to turn it off
     OR
     Run raw TCP "GET /H" and "GET /L" on PuTTY terminal with 192.168.4.1 as IP address and 80 as port

  Created for arduino-esp32 on 04 July, 2018
  by Elochukwu Ifediora (fedy0)
*/

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>


#define LED_BUILTIN 2   // Set the GPIO pin where you connected your test LED or comment this line out if your dev board has a built-in LED

// Set these to your desired credentials.
const char *ssid = "test";
const char *password = "12345678";

const char * udpAddress = "192.168.4.2";
const int udpPort = 3333;

//Are we currently connected?
boolean connected = false;

//The udp library class
WiFiUDP udp;
WiFiUDP udp2;
const int rxPort = 3333;
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Configuring access point...");

  // You can remove the password parameter if you want the AP to be open.
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void loop() {
  //data will be sent to server
  uint8_t buffer[50] = "hello world";
  //Send a packet
  udp.beginPacket(udpAddress, udpPort);
  udp.printf("Seconds since boot: %lu", millis() / 1000);
  udp.endPacket();
  //Wait for 1 second

//  udp2.begin(rxPort);
//  memset(buffer, 0, 50);
//  //processing incoming packet, must be called before reading the buffer
//  int packetsize = udp2.parsePacket();
//  Serial.print("packetsize rx:");
//  Serial.printf("%d\n",packetsize);
//  //receive response from server, it will be HELLO WORLD
//  if (udp2.read(buffer, 50) > 0) {
//    Serial.print("Server to client: ");
//    Serial.println((char *)buffer);
//  }
  delay(1000);
}
