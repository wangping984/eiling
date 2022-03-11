#include <Adafruit_GPS.h>

#define GPS_TX 12
#define GPS_RX 18
#define GPSSerial Serial1
// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  while (!Serial)
  {
    ; // wait for serial port to connect. Needed for Native USB only
  }

  Serial.println("Goodnight moon!");

  GPSSerial.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
  //   set GPS 115200 baud rate
  byte message[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x31, 0x2C, 0x35, 0x2A, 0x31, 0x39, 0x0D, 0x0A};
  GPSSerial.write(message, sizeof(message));
  GPSSerial.flush();
  GPSSerial.begin(115200, SERIAL_8N1, GPS_TX, GPS_RX);
  // set GPS 5Hz update rate
  byte message0[] = {0x24, 0x50, 0x43, 0x41, 0x53, 0x30, 0x32, 0x2C, 0x32, 0x30, 0x30, 0x2A, 0x31, 0x44, 0x0D, 0x0A};
  GPSSerial.write(message0, sizeof(message0));

  // // These lines configure the GPS Module
  // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); // Sets output to only RMC and GGA sentences
  // GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);    // Sets the output to 1/second. If you want you can go higher/lower
}

void loop() // run over and over
{
  if (GPSSerial.available())
    Serial.write(GPSSerial.read());

  // // Now we will start our GPS module, parse (break into parts) the Last NMEA sentence
  // GPS.parse(GPS.lastNMEA()); // This is going to parse the last NMEA sentence the Arduino has received, breaking it down into its constituent parts.
  // GPS.newNMEAreceived();     // This will return a boolean TRUE/FALSE depending on the case.
  //                            // Print the current date/time/etc
  // Serial.print("\nTime: ");
  // Serial.print(GPS.hour, DEC);
  // Serial.print(':');
  // Serial.print(GPS.minute, DEC);
  // Serial.print(':');
  // Serial.print(GPS.seconds, DEC);
  // Serial.print('.');
  // Serial.println(GPS.milliseconds);
  // Serial.print("Date: ");
  // Serial.print(GPS.day, DEC);
  // Serial.print('/');
  // Serial.print(GPS.month, DEC);
  // Serial.print("/20");
  // Serial.println(GPS.year, DEC);
  // Serial.print("Fix: ");
  // Serial.print((int)GPS.fix);
  // Serial.print(" quality: ");
  // Serial.println((int)GPS.fixquality);
  // if (mySerial.available())
  //   Serial.write(mySerial.read());
  // // If GPS module has a fix, line by line prints the GPS information
  // if (GPS.fix)
  // {
  //   Serial.print("Location: ");
  //   Serial.print(GPS.latitude, 4);
  //   Serial.print(GPS.lat);
  //   Serial.print(", ");
  //   Serial.print(GPS.longitude, 4);
  //   Serial.println(GPS.lon);
  //   Serial.print("Location (in degrees, works with Google Maps): ");
  //   Serial.print(GPS.latitudeDegrees, 4);
  //   Serial.print(", ");
  //   Serial.println(GPS.longitudeDegrees, 4);
  //   Serial.print("Speed (knots): ");
  //   Serial.println(GPS.speed);
  //   Serial.print("Angle: ");
  //   Serial.println(GPS.angle);
  //   Serial.print("Altitude: ");
  //   Serial.println(GPS.altitude);
  //   Serial.print("Satellites: ");
  //   Serial.println((int)GPS.satellites);
  // }
}
