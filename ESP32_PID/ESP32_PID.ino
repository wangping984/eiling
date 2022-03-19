/********************************************************
   PID Basic Example
   Reading analog input 0 to control analog PWM output 3
 ********************************************************/

#include <PID_v1.h>

//Define Variables we'll be connecting to
double Setpoint = 1, Input = 0, Output = 0;

//Specify the links and initial tuning parameters
double Kp = 10, Ki = 1, Kd = 0;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

void setup()
{
  Serial.begin(115200);
  Serial.println("PID test...");
  Setpoint = 1;
  Input = 0;
  Output = 0;

  myPID.SetOutputLimits(-3000, 3000);
  myPID.SetSampleTime(2000); // 2000ms PID update rate
  // PID parameters could adjusted by debug command
  myPID.SetTunings(Kp, Ki, Kd);
  //turn the PID on
  myPID.SetMode(AUTOMATIC);
}

void loop()
{
  myPID.SetMode(AUTOMATIC);
  myPID.Compute();
  Serial.print("\ndebug loop: Input = ");
  Serial.print((int)Input);
  Serial.print("  set = ");
  Serial.print((int)Setpoint);
  Serial.print("  output = ");
  Serial.print(int(Output));
  delay(1000);
}
