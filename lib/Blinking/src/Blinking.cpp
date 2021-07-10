#include "Blinking.h"

Blinking::Blinking(int pin)
{
    this->pin = pin;
    pinMode(pin, OUTPUT);
    Start(None);
}

void Blinking::Start(BlinkMode blinkMode)
{
    if (this->blinkMode != blinkMode)
    {
        this->blinkMode = blinkMode;
        if (blinkMode == None)
            digitalWrite(pin, true); // LED ugasen
        else if (blinkMode == WiFiConnecting || blinkMode == NearEnd)
            digitalWrite(pin, false); // LED upaljen
    }
}

void Blinking::Refresh(ulong ms)
{
    if (blinkMode == VeryNearEnd)
        digitalWrite(pin, ms % 4000 > 200);
    if (blinkMode == EnabledOTA)
        digitalWrite(pin, ms % 2000 > 1000);
}

void Blinking::RefreshProgressOTA(ulong progress, ulong total)
{
    ulong p = 10 * progress / total; // [0, 9]
    digitalWrite(pin, p % 2);
}