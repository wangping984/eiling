#define BUZZER 10

const int PWM_Ch = 0;
const int PWM_Res = 8;
const int PWM_Freq = 1000;
const int DutyCycle = 50;

void setup()
{
  ledcAttachPin(BUZZER, PWM_Ch);
  ledcSetup(PWM_Ch, PWM_Freq, PWM_Res);
}

void loop()
{
  ledcAttachPin(BUZZER, PWM_Ch);
//
//  
//  ledcWrite(PWM_Ch, 10);
//  delay(300);
//  ledcWrite(PWM_Ch, 50);
//  delay(300);
//  ledcWrite(PWM_Ch, 90);
//  delay(300);
//
//  ledcWrite(PWM_Ch, 0);
//  delay(3000);
//
//  ledcSetup(PWM_Ch, 5000, PWM_Res);
//  ledcWrite(PWM_Ch, 10);
//  delay(300);
//  ledcWrite(PWM_Ch, 50);
//  delay(300);
//  ledcWrite(PWM_Ch, 90);
//  delay(300);
//
//  ledcWrite(PWM_Ch, 0);
//  delay(3000);
//
//  ledcSetup(PWM_Ch, PWM_Freq, PWM_Res);
//  ledcWrite(PWM_Ch, 50);
//  delay(300);
//  ledcSetup(PWM_Ch, 2000, PWM_Res);
//  ledcWrite(PWM_Ch, 50);
//  delay(300);

  ledcWriteTone(PWM_Ch, 1000);
//  delay(1000);
//  ledcWriteTone(PWM_Ch, 2000);
//  delay(1000);
//
//  ledcDetachPin(BUZZER);
//  delay(6000);
}
