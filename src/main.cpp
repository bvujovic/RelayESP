// Paljenje i gasenje bojlera:
//    1) u zadato vreme, npr. 07:00-07:30 radi, a inace je iskljucen
//    2) momentalno paljenje, bojler se gasi posle zadatog vremena (npr. 15min)
// Moguca poboljsanja:
// - LED dioda koja je upaljena kada je upaljen bojler i koju vidi korisnik.
// Ovo mozda nije neophodno ako svetlo na bojleru radi tu stvar.
// - Rad sa vremenskim intervalima gde je start veci od end vremena, npr. 23:45 - 01:30
// - Sleep za ESP ili gasenje WiFi-a neko vreme po paljenju aparata
// - Opcija na veb stranici da se, odmah po pamcenju unetih vrednosti, ugasi WiFi

#include <Arduino.h>
#include <WiFiServerBasics.h>
ESP8266WebServer server(80);

#include <ArduinoOTA.h>

#include <SNTPtime.h>
SNTPtime ntp;
strDateTime now;

#include <EasyINI.h>
EasyINI ei("/dat/config.ini");

#include <ClickButton.h>
// taster: startovanje WiFi-a (dugi klik), OTA update-a (dugi klik dok je WiFi ON), moment-on svetlo (kratki klikovi)
ClickButton btn(D4, LOW, CLICKBTN_PULLUP);

const int pinRelay = D1; // pin na koji je povezan relej koji pusta/prekida struju ka kontrolisanom potrosacu
#include <Blinking.h>
Blinking blink(LED_BUILTIN); // prikaz statusa aparata

#define DEBUG true

unsigned long msWiFiStarted;             // vreme ukljucenja WiFi-a
const unsigned long WIFI_ON = 5 * 60000; // wifi ce biti ukljucen 5min, a onda se gasi
bool isWiFiOn;                           // da li je wifi ukljucen ili ne
bool isOtaOn = false;                    // da li je OTA update u toku
const int nearEndMinutes = -20;          // koliko minuta u odnosu na gasenje releja se smatra blizu gasenja
const int veryNearEndMinutes = -5;       // koliko minuta u odnosu na gasenje releja se smatra vrlo blizu gasenja
bool autoOn, momentOn;
int autoStartHour, autoStartMin, autoEndHour, autoEndMin;
int momentStartHour, momentStartMin, momentEndHour, momentEndMin;
String appName; // sta pise u tabu index stranice pored "ESP Relay"
int ipLastNum;  // poslednji deo IP adrese - broj iza 192.168.0.

void GetCurrentTime()
{
  while (!ntp.setSNTPtime())
    if (DEBUG)
      Serial.print("*");
  if (DEBUG)
    Serial.println(" Time set");
}

// "01:08" -> 1, 8
void ParseTime(String s, int &h, int &m)
{
  int idx = s.indexOf(':');
  h = s.substring(0, idx).toInt();
  m = s.substring(idx + 1).toInt();
}

// za dati broj minuta, uz momentStartHour i momentStartMin, izracunavaju se vrednosti momentEndHour i momentEndMin
void CalcMomentEnd(int mins)
{
  momentEndMin = (momentStartMin + mins) % 60;
  int itvHours = (momentStartMin + mins) / 60;
  momentEndHour = (momentStartHour + itvHours) % 24;
}

void ReadConfigFile()
{
  ei.Open(EasyIniFileMode::Read);
  autoOn = ei.GetInt("auto", true);
  if (autoOn)
  {
    ParseTime(ei.GetString("auto_from", "12:00"), autoStartHour, autoStartMin);
    ParseTime(ei.GetString("auto_to", "12:30"), autoEndHour, autoEndMin);
  }
  if (momentOn)
  {
    ParseTime(ei.GetString("moment_from", "13:00"), momentStartHour, momentStartMin);
    CalcMomentEnd(ei.GetInt("moment_mins", 15));
  }
  appName = ei.GetString("app_name");
  ipLastNum = ei.GetInt("ip_last_num");
  ei.Close();

  if (DEBUG)
  {
    Serial.println(autoOn);
    Serial.println(autoStartHour);
    Serial.println(autoStartMin);
    Serial.println(autoEndHour);
    Serial.println(autoEndMin);
    Serial.println(momentStartHour);
    Serial.println(momentStartMin);
    Serial.println(momentEndHour);
    Serial.println(momentEndMin);
    Serial.println(appName);
    Serial.println(ipLastNum);
  }
}

void HandleSaveConfig()
{
  // auto=1&auto_from=06:45&auto_to=07:20&moment=1&moment_from=22:59&moment_mins=5
  ei.Open(EasyIniFileMode::Write);
  ei.SetString("auto", server.arg("auto"));
  ei.SetString("auto_from", server.arg("auto_from"));
  ei.SetString("auto_to", server.arg("auto_to"));
  momentOn = server.arg("moment") == "1";
  ei.SetString("moment_from", server.arg("moment_from"));
  ei.SetString("moment_mins", server.arg("moment_mins"));
  ei.SetString("app_name", server.arg("app_name"));
  ei.SetString("ip_last_num", server.arg("ip_last_num"));
  ei.Close();
  ReadConfigFile();

  server.send(200, "text/plain", "");
}

// Konektovanje na WiFi, uzimanje tacnog vremena, postavljanje IP adrese i startovanje veb servera.
void WiFiOn()
{
  blink.Start(BlinkMode::WiFiConnecting);
  Serial.println("Turning WiFi ON...");
  WiFi.mode(WIFI_STA);
  ConnectToWiFi();
  GetCurrentTime();
  SetupIPAddress(ipLastNum);
  server.on("/", []() { HandleDataFile(server, "/index.html", "text/html"); });
  server.on("/inc/comp_on-off_btn1.png", []() { HandleDataFile(server, "/inc/comp_on-off_btn1.png", "image/png"); });
  server.on("/inc/script.js", []() { HandleDataFile(server, "/inc/script.js", "text/javascript"); });
  server.on("/inc/style.css", []() { HandleDataFile(server, "/inc/style.css", "text/css"); });
  server.on("/dat/config.ini", []() { HandleDataFile(server, "/dat/config.ini", "text/x-csv"); });
  server.on("/save_config", HandleSaveConfig);
  server.begin();
  isWiFiOn = true;
  msWiFiStarted = millis();
  Serial.println("WiFi ON");
  blink.Start(BlinkMode::None);
}

// Diskonektovanje sa WiFi-a.
void WiFiOff()
{
  isWiFiOn = false;
  Serial.println("Turning WiFi OFF...");
  server.stop();
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(100);
  Serial.println("WiFi OFF");
}

void setup()
{
  pinMode(pinRelay, OUTPUT);
  btn.multiclickTime = 500;

  Serial.begin(115200);
  SPIFFS.begin();
  ReadConfigFile();

  WiFiOn();

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    blink.RefreshOTA(progress, total);
  });
}

void loop()
{
  delay(10);

  if (isOtaOn)
  {
    ArduinoOTA.handle();
    return;
  }

  now = ntp.getTime(1.0, 1);
  if (DEBUG && now.second == 0)
  {
    ntp.printDateTime(now);
    delay(1000);
  }

  btn.Update();
  if (btn.clicks >= 1) // 1 ili vise kratkih klikova - moment-on paljenje/produzavanje svetla
  {
    momentOn = true;
    momentStartHour = now.hour;
    momentStartMin = now.minute;
#ifdef DEBUG
    CalcMomentEnd(btn.clicks);
#else
    CalcMomentEnd(btn.clicks * 10); // svaki klik vredi 10min trajanja moment-on svetla
#endif
  }

  // ukljucivanje/iskljucivanje releja
  bool isItOn = false;
  if (autoOn)
    isItOn = now.IsInInterval(autoStartHour, autoStartMin, autoEndHour, autoEndMin);
  if (!isItOn && momentOn)
    isItOn = now.IsInInterval(momentStartHour, momentStartMin, momentEndHour, momentEndMin);
  digitalWrite(pinRelay, isItOn);

  // (automatsko) iskljucivanje Moment opcije odmah po isteku moment intervala
  if (momentOn && now.IsItTime(momentEndHour, momentEndMin, 00))
    momentOn = false;

  blink.Refresh(millis());

  // LED signal da je uskoro kraj perioda ukljucenog svetla
  if (isItOn)
  {
    BlinkMode blinkMode = BlinkMode::None;
    if ((autoOn && now.IsInInterval(autoEndHour, autoEndMin, nearEndMinutes * 60)) || (momentOn && now.IsInInterval(momentEndHour, momentEndMin, nearEndMinutes * 60)))
      blinkMode = BlinkMode::NearEnd;
    if (blinkMode == BlinkMode::NearEnd) // blizu je kraj, ispitati da li je kraj jako blizu
      if ((autoOn && now.IsInInterval(autoEndHour, autoEndMin, veryNearEndMinutes * 60)) || (momentOn && now.IsInInterval(momentEndHour, momentEndMin, veryNearEndMinutes * 60)))
        blinkMode = BlinkMode::VeryNearEnd;

    // ako su auto i moment ukljuceni
    if (autoOn && momentOn && blinkMode != BlinkMode::None)
    {
      // mozda bi trebalo videti koliko je preostalo vremena do daljeg kraja
      int laterEndHour = autoEndHour;
      int laterEndMin = autoEndMin;
      if (now.IsInInterval(laterEndHour, laterEndMin, nearEndMinutes * 60))
        blinkMode = BlinkMode::NearEnd;
      if (now.IsInInterval(laterEndHour, laterEndMin, veryNearEndMinutes * 60))
        blinkMode = BlinkMode::VeryNearEnd;
    }

    blink.Start(blinkMode);
  }
  else
    blink.Start(BlinkMode::None);

  // serverova obrada eventualnog zahteva klijenta i gasenje WiFi-a x minuta posle paljenja aparata
  if (isWiFiOn)
  {
    server.handleClient();
    if (millis() > msWiFiStarted + WIFI_ON)
      WiFiOff();

    if (btn.clicks == -1) // dugacak klik - startovanje OTA update-a
    {
      isOtaOn = true;
      ArduinoOTA.begin();
      msWiFiStarted = millis();
    }
  }
  else // WiFi OFF
  {
    // periodicno uzimanje tacnog vremena, tacno u ponoc
    if (now.IsItTime(00, 00, 00))
      WiFiOn();

    if (btn.clicks == -1) // dugacak klik - paljenje WiFi-a
      WiFiOn();
  }
}