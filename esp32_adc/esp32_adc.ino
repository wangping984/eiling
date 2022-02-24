#define ADC1_1 2

void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  
  //set the resolution to 12 bits (0-4096)
  analogReadResolution(12);
}

void loop() {
  // read the analog / millivolts value for pin 2:
  int analogValue = analogRead(ADC1_1);
  
  // print out the values you read:
  Serial.printf("ADC analog value = %d\n",analogValue);
  
  delay(100);  // delay in between reads for clear read from serial
}
