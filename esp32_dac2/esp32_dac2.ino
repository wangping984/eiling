#define DAC1 17
#define DAC2 18


void setup()
{

}

void loop()
{
  dacWrite(DAC1, 0 );
  dacWrite(DAC2, 255 );
  delay(4000);
  dacWrite(DAC1, 255 );
  dacWrite(DAC2, 0 );
  delay(2000);
}
