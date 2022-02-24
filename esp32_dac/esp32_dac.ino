#define DAC1 17
#define DAC2 18


void setup()
{
    //setup on pin 18, channel 0 with frequency 312500 Hz
    sigmaDeltaSetup(DAC1,0, 312500);
    //initialize channel 0 to off
    sigmaDeltaWrite(0, 0);
}

void loop()
{
    static uint8_t i = 0;
    sigmaDeltaWrite(0, i++);
    delay(100);
}
