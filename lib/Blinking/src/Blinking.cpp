#include "Blinking.h"

Blinking::Blinking(byte pin, bool onValue)
{
    this->pin = pin;
    this->onValue = onValue;
    pinMode(pin, OUTPUT);
    Start(None);
}

void Blinking::Start(BlinkMode blinkMode)
{
    if (this->blinkMode != blinkMode)
    {
        this->blinkMode = blinkMode;
        if (blinkMode == None)
            dWrite(pin, true); // LED ugasen
        else if (blinkMode == WiFiConnecting || blinkMode == NearEnd)
            dWrite(pin, false); // LED upaljen
    }
}

void Blinking::Refresh(ulong ms)
{
    if (blinkMode == VeryNearEnd)
        dWrite(pin, ms % 4000 > 200);
    if (blinkMode == EnabledOTA)
        dWrite(pin, ms % 2000 > 1000);
}

void Blinking::RefreshProgressOTA(ulong progress, ulong total)
{
    ulong p = 10 * progress / total; // [0, 9]
    dWrite(pin, p % 2);
}