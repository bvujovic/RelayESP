#pragma once

#include <Arduino.h>

// Moguca stanja blinkanja. None - (podrazumevano) nema blinkanja.
enum BlinkMode
{
    None,
    WiFiConnecting,
    OTA,
    NearEnd,
    VeryNearEnd,
    //todo 1/2/3 klik -> potvrda blinkanjem za produzenje moment-on ukljucenosti
};

// Logika paljenja i gasenja LED diode u projektu.
class Blinking
{
private:
    int pin;             // Pin na koji se odnosi ovaj blink signal
    BlinkMode blinkMode; // Ovo bi kasnije moglo da se zameni stekom, tj. sa LinkedList tako bi se izvrsavao
                         // onaj mod koji je na vrhu steka, a kad se on zavrsi, automatski se prelazi na sledeci
    //* ulong modeStarted;   // Trenutak kada je tekuci mod zapocet

public:
    Blinking(int pin);
    void Start(BlinkMode blinkMode);
    void Refresh(ulong ms);
    void RefreshOTA(uint progress, uint total);

};