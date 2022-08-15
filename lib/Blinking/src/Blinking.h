#pragma once

#include <Arduino.h>
typedef unsigned long ulong;

// Moguca stanja blinkanja. None - (podrazumevano) nema blinkanja.
enum BlinkMode
{
    None,
    WiFiConnecting,
    EnabledOTA,
    ProgressOTA,
    NearEnd,
    VeryNearEnd,
    // todo 1/2/3 klik -> potvrda blinkanjem za produzenje moment-on ukljucenosti
};

// Logika paljenja i gasenja LED diode u projektu.
class Blinking
{
private:
    byte pin;            // Pin na koji se odnosi ovaj blink signal
    BlinkMode blinkMode; // Ovo bi kasnije moglo da se zameni stekom, tj. sa LinkedList tako bi se izvrsavao
                         // onaj mod koji je na vrhu steka, a kad se on zavrsi, automatski se prelazi na sledeci
    //* ulong modeStarted;   // Trenutak kada je tekuci mod zapocet
    bool onValue; // Vrednost koja odgovara upaljenoj LED diodi: HIGH/LOW tj. true/false tj. 1/0.

    // B void dWrite(byte pin, byte val) { digitalWrite(pin, !val); }
    void dWrite(byte pin, byte val) { digitalWrite(pin, (val ^ onValue)); }

public:
    Blinking(byte pin, bool onValue = true);
    void Start(BlinkMode blinkMode);
    void Refresh(ulong ms);
    void RefreshProgressOTA(ulong progress, ulong total);
    byte GetPin() { return pin; }
};