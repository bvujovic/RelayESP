#include <Arduino.h>

const int pinLed = D1;

void setup()
{
  digitalWrite(LED_BUILTIN, true);
  pinMode(pinLed, OUTPUT);
  digitalWrite(pinLed, true);
}

void loop()
{
  delay(100);
}