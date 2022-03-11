

#include "pinout.h"
#include <WiFi.h>
#include <AsyncUDP.h> //引用以使用异步UDP
#include <WiFiAP.h>
#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <CommandParser.h>
#include <WebServer.h>
#include <Update.h>

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
// 2hz detect rate for keypad
unsigned long startMillis; // some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 500; // the value is a number of milliseconds

unsigned long startMillis2; // some global variables available anywhere in the program
unsigned long currentMillis2;
const unsigned long period2 = 1000; // the value is a number of milliseconds

int state_cc = 0;
int state_prev_cc = 0;
byte set_speed = 0;

bool key_cc = false;
bool key_cancel = false;
bool key_set = false;
bool key_res = false;
bool key_inc = false;
bool key_dec = false;
bool GPS_OK = false;

bool debug_sm = false;
bool debug_keypad = false;
bool debug_ota = false;
bool debug_adccal = false;

String cstr;
char buf[50];
char response[64];

WebServer server(80);

/*
 * Login page
 */

const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

/*
 * Server Index Page
 */

const char *serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

void SM_cc()
{
  state_prev_cc = state_cc;
  switch (state_cc)
  {
  case 0: // INIT state
    state_cc = 1;
    break;

  case 1: // pass thru state
    if (key_cc && GPS_OK)
    {
      state_cc = 2;
    }
    break;

  case 2: // pre_cruise state
    if (key_set && GPS_OK)
    {
      state_cc = 3;
    }
    if (key_res && GPS_OK && set_speed != 0)
    {
      state_cc = 3;
    }
    if (GPS_OK == false || key_cc == true)
    {
      state_cc = 1;
    }
    break;

  case 3: // cruising state
    if (GPS_OK == false)
    {
      state_cc = 1;
    }
    if (key_cancel)
    {
      state_cc = 2;
    }
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
  buf[cstr.length() + 1];
  // string to char array, length should increase 1 for null termination
  cstr.toCharArray(buf, cstr.length() + 1);
  // send udp could be length of 4
  Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);
}

void ota_start()
{
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex); });
  server.on("/serverIndex", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex); });
  /*handling uploading firmware file */
  server.on(
      "/update", HTTP_POST, []()
      {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });
  server.begin();
}

void ota_stop()
{
  server.close();
}

void cmd_debug(MyCommandParser::Argument *args, char *response)
{
  String arg0;
  int arg1;
  arg0 = args[0].asString;
  arg1 = args[1].asInt64;
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
    break;
  case 64: // cancel
    key_cancel = true;
    break;
  case 16: // set
    key_set = true;
    break;
  case 128: // restore
    key_res = true;
    break;
  case 8: // increase
    key_inc = true;
    break;
  case 4: // decrease
    key_dec = true;
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
}

void pass_thru()
{
  unsigned int A1 = analogRead(AIN1);
  unsigned int A2 = analogRead(AIN2);
  float ratio = float(A1) / float(A2);
  if (ratio > 1.05 || ratio < 0.95)
  {
    // beep(1000, 2);
    dac.setVoltage(622, false); // 0.76V/5V*(2^12-1)=622
  }
  else
  {
    dac.setVoltage(A1 * 2, false);
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
}

void setup()
{
  // put your setup code here, to run once:

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

  startMillis = millis();  // initial start time
  startMillis2 = millis(); // initial start time
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
      if (state_cc != 3)
        pass_thru();
    }
  }
  currentMillis = millis();                  // get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= period) // test whether the period has elapsed
  {
    // GPS_OK = true;
    key_pressed_detect();
    SM_cc();
    debug_info();

    startMillis = currentMillis; // IMPORTANT to save the start time of the current LED state.
  }

  currentMillis2 = millis();
  if (currentMillis2 - startMillis2 >= period2) // test whether the period has elapsed
  {
    //    unsigned int A1 = analogRead(AIN1);
    //    float volt = (float)A1 / 4095 * 2.5;
    //    cstr = String(volt, 2);
    //    cstr = "Ain1 = " + cstr + " V";
    //    buf[cstr.length() + 1];
    //    // string to char array, length should increase 1 for null termination
    //    cstr.toCharArray(buf, cstr.length() + 1);
    //    // send udp could be length of 4
    //    Udp.writeTo((const uint8_t *)buf, cstr.length(), remoteUDP_Ip, UdpPort);

    startMillis2 = currentMillis2; // IMPORTANT to save the start time of the current LED state.
  }
}
