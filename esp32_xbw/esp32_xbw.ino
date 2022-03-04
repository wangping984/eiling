
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


//Define variables to hold the data

//for shift register.

//starting with a non-zero numbers can help

//troubleshoot

byte switchVar1 = 72;  //01001000

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello!");

  //define pin modes
  pinMode(key_load, OUTPUT);
  pinMode(key_clk, OUTPUT);
  pinMode(key_data, INPUT);
  digitalWrite(key_load, 1);
  digitalWrite(key_clk, 0);
}

void loop() {
  // put your main code here, to run repeatedly:

  //Pulse the latch pin:

  //set it to 1 to collect parallel data

  digitalWrite(key_load, 1);

  //set it to 1 to collect parallel data, wait

  delayMicroseconds(20);

  //set it to 0 to transmit data serially

  digitalWrite(key_load, 0);
  //while the shift register is in serial mode

  //collect each shift register into a byte

  //the register attached to the chip comes in first

  switchVar1 = shiftIn(key_data, key_clk);

  //Print out the results.

  //leading 0's at the top of the byte

  //(7, 6, 5, etc) will be dropped before

  //the first pin that has a high input

  //reading

  Serial.println(switchVar1, BIN);
  //white space
  Serial.println("-------------------");
  //delay so all these print statements can keep up.
  delay(500);
}

byte shiftIn(int myDataPin, int myClockPin) {

  int i;

  int temp = 0;

  int pinState;

  byte myDataIn = 0;

  //  pinMode(myClockPin, OUTPUT);
  //
  //  pinMode(myDataPin, INPUT);
  //we will be holding the clock pin high 8 times (0,..,7) at the
  //end of each time through the for loop

  //at the beginning of each loop when we set the clock low, it will
  //be doing the necessary low to high drop to cause the shift
  //register's DataPin to change state based on the value
  //of the next bit in its serial information flow.
  //The register transmits the information about the pins from pin 7 to pin 0
  //so that is why our function counts down

  for (i = 7; i >= 0; i--)

  {

    digitalWrite(myClockPin, 0);

    delayMicroseconds(100);

    temp = digitalRead(myDataPin);

    if (temp) {

      pinState = 1;

      //set the bit to 0 no matter what

      myDataIn = myDataIn | (1 << i);

    }

    else {

      //turn it off -- only necessary for debugging

      //print statement since myDataIn starts as 0

      pinState = 0;

    }

    //Debugging print statements

    //Serial.print(pinState);

    //Serial.print("     ");

    //Serial.println (dataIn, BIN);

    digitalWrite(myClockPin, 1);
    delayMicroseconds(100);

  }

  //debugging print statements whitespace

  //Serial.println();

  //Serial.println(myDataIn, BIN);

  return myDataIn;
}
