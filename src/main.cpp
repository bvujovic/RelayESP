// Paljenje i gasenje nekog aparata koji radi na 220V (svetlo, bojler...):
//    1) u zadato vreme, npr. 07:00-07:30 radi, a inace je iskljucen
//    2) momentalno paljenje, bojler se gasi posle zadatog vremena (npr. 15min)
// Koriscenje:
// Paljenje i gasenje el. uredjaja kontrolisanog ovim aparatom se podesava na veb stranici http://192.168.0.x/
// gde je x definisano u config.ini (treba da bude izmedju 30 i 39).
// Kratki klikovi na tasteru: 1 klik -> 10 min moment-on, 2 klika -> 20 min ...
// Dugi klik: ako WiFi nije upaljen -> pali se, ako je WiFi vec upaljen -> enable-uje se OTA updates

#include <Arduino.h>
typedef unsigned long ulong;
bool DEBUG = false;
#include <WiFiServerBasics.h>
ESP8266WebServer server(80);
bool isServerSet = false; // da li su nakaceni handler-i na server: zvana on() metoda za sve veb stranice

#include <UtilsCommon.h>
#include <ArduinoOTA.h>

// B
// #include <SNTPtime.h> // https://github.com/SensorsIot/SNTPtime/
//  SNTPtime *ntp = NULL;
//  StrDateTime now;
#define MY_NTP_SERVER "rs.pool.ntp.org"
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03"
#include <time.h>
time_t now;   // this is the epoch
struct tm tm; // the structure tm holds time information in a more convient way

#include <EasyINI.h>
EasyINI ei("/dat/config.ini");

#include <EasyFS.h>

#include <ClickButton.h>
// taster: startovanje WiFi-a (dugi klik), OTA update-a (dugi klik dok je WiFi ON), moment-on svetlo (kratki klikovi)
ClickButton btn(D3, LOW, CLICKBTN_PULLUP);

const int pinRelay = D1; // pin na koji je povezan relej koji pusta/prekida struju ka kontrolisanom potrosacu
#include <Blinking.h>
// B Blinking blink(D2); // prikaz statusa aparata
Blinking *blink; // prikaz statusa aparata

const ulong SEC = 1000;            // broj milisekundi u sekundi
const ulong MIN = 60 * SEC;        // broj milisekundi u minutu
ulong msWiFiStarted;               // vreme ukljucenja WiFi-a
ulong lastWebReq;                  // vreme poslednjeg cimanja veb servera
ulong itvWiFiOn = (DEBUG ? 1 : 5); // broj minuta koliko ce wifi/server biti ukljucen, a onda se gasi
bool isWiFiOn;                     // da li je wifi ukljucen ili ne
bool isOtaOn = false;              // da li je aparat spreman za OTA update
ulong msOtaStarted;                // vreme enable-ovanja OTA update-a
const ulong noOtaTime = 5 * SEC;   // vreme (u ms) koje mora proteci od startovanja WiFi-a do enable-a OTA update-a
const ulong maxOtaTime = MIN;      // vreme (u ms) koje ce aparat provesti u cekanju da pocne OTA update
const int nearEndMinutes = -20;    // koliko minuta u odnosu na gasenje releja se smatra blizu gasenja
const int veryNearEndMinutes = -5; // koliko minuta u odnosu na gasenje releja se smatra vrlo blizu gasenja
bool autoOn, momentOn;
int autoStartHour, autoStartMin, autoEndHour, autoEndMin;
int momentStartHour, momentStartMin, momentEndHour, momentEndMin;
String appName; // sta pise u tabu index stranice pored "ESP Relay"
int ipLastNum;  // poslednji deo IP adrese - broj iza 192.168.0.
bool isTimeSet;
ulong cntTrySetTime = 0;
const ulong maxTrySetTime = 3;

void GetTime()
{
  time(&now);             // read the current time
  localtime_r(&now, &tm); // update the structure tm with the current time
}

bool IsItTime(int hour, int min, int sec)
{
  return tm.tm_hour == hour && tm.tm_min == min && tm.tm_sec == sec;
}

int HourMinToSecs(byte h, byte m)
{
  return ((h * 60) + m) * 60;
}

bool IsInIntervalSecs(int startSecs, int nowSecs, int endSecs)
{
  if (startSecs < endSecs)
    return startSecs <= nowSecs && nowSecs < endSecs;
  else
    return startSecs <= nowSecs || nowSecs < endSecs;
}

bool IsInInterval(byte startHour, byte startMin, byte endHour, byte endMin)
{
  int startSecs = HourMinToSecs(startHour, startMin);
  int endSecs = HourMinToSecs(endHour, endMin);
  int nowSecs = HourMinToSecs(tm.tm_hour, tm.tm_min);
  // T printf("start: %d, now: %d, end: %d \n", startSecs, nowSecs, endSecs);
  return IsInIntervalSecs(startSecs, nowSecs, endSecs);
}

bool IsInInterval(byte refHour, byte refMin, int secs)
{
    int startSecs = HourMinToSecs(refHour, refMin);
    int endSecs = startSecs + secs;
    int nowSecs = HourMinToSecs(tm.tm_hour, tm.tm_min);
    const int daySecs = 24 * 60 * 60;
    if (endSecs > daySecs)
        endSecs -= daySecs;
    if (endSecs < 0)
        endSecs += daySecs;
    if (secs < 0)
        SWAP(startSecs, endSecs, int);
    // T printf("start: %d, end: %d, now: %d \n", startSecs, endSecs, nowSecs);
    return IsInIntervalSecs(startSecs, nowSecs, endSecs);
}

void LogMessage(const String &msg)
{
  char strDateTime[32];
  sprintf(strDateTime, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  EasyFS::addf(msg);
  EasyFS::addf(strDateTime);
}

// B
//  void GetCurrentTime()
//  {
//    ulong start = millis();
//    if (ntp != NULL)
//      delete ntp;
//    ntp = new SNTPtime();
//    isTimeSet = false;
//    cntTrySetTime = 0;
//    while (!ntp->setSNTPtime() && cntTrySetTime++ < maxTrySetTime)
//      Serial.print("*");
//    Serial.println();
//    if (cntTrySetTime < maxTrySetTime)
//    {
//      isTimeSet = true;
//      Serial.println("Time set");
//    }
//    ulong msec = millis() - start;
//    now = ntp->getTime(1.0, 1);
//    LogMessage(String("GetCurrentTime: ") + msec + " ms, free: " + ESP.getFreeHeap() + " bytes");
//  }

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
  ei.open(EasyIniFileMode::FMOD_READ);
  autoOn = ei.getInt("auto", true);
  if (autoOn)
  {
    ParseTime(ei.getString("auto_from", "12:00"), autoStartHour, autoStartMin);
    ParseTime(ei.getString("auto_to", "12:30"), autoEndHour, autoEndMin);
  }
  if (momentOn)
  {
    ParseTime(ei.getString("moment_from", "13:00"), momentStartHour, momentStartMin);
    CalcMomentEnd(ei.getInt("moment_mins", 15));
  }
  itvWiFiOn = ei.getInt("wifi_on", 5);
  appName = ei.getString("app_name");
  appName.trim();

  if (appName == "lil aq")
  {
    ipLastNum = 31;
    blink = new Blinking(LED_BUILTIN, LOW);
  }
  else if (appName == "BIG AQ")
  {
    ipLastNum = 32;
    blink = new Blinking(D2, HIGH);
  }
  else
  {
    ipLastNum = 30;
    blink = new Blinking(LED_BUILTIN, LOW);
  }
  ei.close();

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
    Serial.println(itvWiFiOn);
    Serial.println(appName);
    Serial.println(ipLastNum);
  }
}

void StartOtaUpdate()
{
  isOtaOn = true;
  blink->Start(BlinkMode::EnabledOTA);
  ArduinoOTA.begin();
  msWiFiStarted = msOtaStarted = millis();
}

void HandleGetCurrentTime()
{
  // B GetCurrentTime();
  SendEmptyText(server);
}

// Server vraca tekuce vreme sa kojim radi aparat.
void HandleGetDeviceTime()
{
  char str[12];
  sprintf(str, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
  server.send(200, "text/plain", str);
  lastWebReq = millis();
}

// Cuvanje config podataka prosledjenih u query stringu.
void HandleSaveConfig()
{
  // auto=1&auto_from=06:45&auto_to=07:20&moment=1&moment_from=22:59&moment_mins=5
  ei.open(EasyIniFileMode::FMOD_WRITE);
  ei.setString("auto", server.arg("auto"));
  ei.setString("auto_from", server.arg("auto_from"));
  ei.setString("auto_to", server.arg("auto_to"));
  momentOn = server.arg("moment") == "1";
  ei.setString("moment_from", server.arg("moment_from"));
  ei.setString("moment_mins", server.arg("moment_mins"));
  ei.setString("wifi_on", server.arg("wifi_on"));
  ei.setString("app_name", server.arg("app_name"));
  // B ei.setString("ip_last_num", server.arg("ip_last_num"));
  ei.close();
  ReadConfigFile();
  lastWebReq = millis();
  SendEmptyText(server);
}

// Konektovanje na WiFi, uzimanje tacnog vremena, postavljanje IP adrese i startovanje veb servera.
void WiFiOn()
{
  blink->Start(BlinkMode::WiFiConnecting);
  Serial.println("Turning WiFi ON...");
  WiFi.mode(WIFI_STA);
  while (!ConnectToWiFi())
  {
    Serial.println("Connecting to WiFi failed. Device will try to connect again...");
    for (ulong i = 0; i < 10; i++)
    {
      digitalWrite(blink->GetPin(), i % 2);
      delay(500);
    }
    delay((DEBUG ? 1 : 10) * MIN);
  }
  // B GetCurrentTime();
  SetupIPAddress(ipLastNum);
  if (!isServerSet)
  {
    server.on("/", []()
              {
              HandleDataFile(server, "/index.html", "text/html");
              lastWebReq = millis(); });
    server.on("/inc/comp_on-off_btn1.png", []()
              { HandleDataFile(server, "/inc/comp_on-off_btn1.png", "image/png"); });
    server.on("/inc/script.js", []()
              { HandleDataFile(server, "/inc/script.js", "text/javascript"); });
    server.on("/inc/style.css", []()
              { HandleDataFile(server, "/inc/style.css", "text/css"); });
    server.on("/dat/config.ini", []()
              { HandleDataFile(server, "/dat/config.ini", "text/x-csv"); });
    server.on("/dat/msg.log", []()
              { HandleDataFile(server, "/dat/msg.log", "text/plain"); });
    server.on("/save_config", HandleSaveConfig);
    server.on("/getDeviceTime", HandleGetDeviceTime);
    server.on("/getCurrentTime", HandleGetCurrentTime);
    server.on("/otaUpdate", []()
              {
              server.send(200, "text/plain", "ESP is waiting for OTA updates...");
              StartOtaUpdate();
              lastWebReq = millis(); });
    isServerSet = true;
  }
  server.begin();
  isWiFiOn = true;
  msWiFiStarted = lastWebReq = millis();
  Serial.println("WiFi ON");
  blink->Start(BlinkMode::None);
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
  LittleFS.begin();
  ReadConfigFile();
  EasyFS::setFileName("/dat/msg.log");
  configTime(MY_TZ, MY_NTP_SERVER);
  WiFiOn();
  LogMessage("SETUP");

  ArduinoOTA.onProgress([](ulong progress, ulong total)
                        { blink->RefreshProgressOTA(progress, total); });
}

void loop()
{
  delay(10);
  ulong ms = millis();

  if (isOtaOn)
  {
    // ako istekne maxOtaTime pre nego sto pocne OTA update, aparat se vraca u redovan rezim rada
    if (ms > msOtaStarted + maxOtaTime)
    {
      isOtaOn = false;
      blink->Start(BlinkMode::None);
    }

    blink->Refresh(ms);
    ArduinoOTA.handle();
    return;
  }

  // B now = ntp->getTime(1.0, 1);
  GetTime();
  // B if (DEBUG && now.second == 0)
  if (DEBUG && tm.tm_sec == 0)
  {
    Serial.print("Hour:");
    Serial.print(tm.tm_hour); // hours since midnight  0-23
    Serial.print("\tmin:");
    Serial.print(tm.tm_min); // minutes after the hour  0-59
    Serial.print("\tsec:");
    Serial.print(tm.tm_sec); // seconds after the minute  0-61*
    delay(1000);
  }
  // if (!isTimeSet && tm.tm_sec == 0 && tm.tm_min % 10 == 0)
  // {
  //   if (isWiFiOn)
  //     GetCurrentTime();
  //   else
  //     WiFiOn();
  // }

  btn.Update();
  if (btn.clicks >= 1) // 1 ili vise kratkih klikova - moment-on paljenje/produzavanje svetla
  {
    tprintln(btn.clicks);
    momentOn = true;
    momentStartHour = tm.tm_hour;
    momentStartMin = tm.tm_min;
    CalcMomentEnd(btn.clicks * (DEBUG ? 1 : 10)); // nije DEBUG -> svaki klik vredi 10min trajanja moment-on svetla
  }

  // ukljucivanje/iskljucivanje releja
  bool isRelayOn = false;
  if (autoOn)
    isRelayOn = IsInInterval(autoStartHour, autoStartMin, autoEndHour, autoEndMin);
  if (!isRelayOn && momentOn)
    isRelayOn = IsInInterval(momentStartHour, momentStartMin, momentEndHour, momentEndMin);
  digitalWrite(pinRelay, isRelayOn);

  // (automatsko) iskljucivanje Moment opcije odmah po isteku moment intervala
  if (momentOn && IsItTime(momentEndHour, momentEndMin, 00))
    momentOn = false;

  blink->Refresh(ms);

  // LED signal da je uskoro kraj perioda ukljucenog svetla
  if (isRelayOn)
  {
    BlinkMode blinkMode = BlinkMode::None;
    if ((autoOn && IsInInterval(autoEndHour, autoEndMin, nearEndMinutes * 60)) || (momentOn && IsInInterval(momentEndHour, momentEndMin, nearEndMinutes * 60)))
      blinkMode = BlinkMode::NearEnd;
    if (blinkMode == BlinkMode::NearEnd) // blizu je kraj, ispitati da li je kraj jako blizu
      if ((autoOn && IsInInterval(autoEndHour, autoEndMin, veryNearEndMinutes * 60)) || (momentOn && IsInInterval(momentEndHour, momentEndMin, veryNearEndMinutes * 60)))
        blinkMode = BlinkMode::VeryNearEnd;

    // ako su auto i moment ukljuceni
    if (autoOn && momentOn && blinkMode != BlinkMode::None)
    {
      blinkMode = BlinkMode::None;
      int laterEndHour = autoEndHour;
      int laterEndMin = autoEndMin;
      if (momentEndHour > autoEndHour || (momentEndHour == autoEndHour && momentEndMin > autoEndMin))
      {
        laterEndHour = momentEndHour;
        laterEndMin = momentEndMin;
      }
      if (IsInInterval(laterEndHour, laterEndMin, nearEndMinutes * 60))
        blinkMode = BlinkMode::NearEnd;
      if (blinkMode == BlinkMode::NearEnd && IsInInterval(laterEndHour, laterEndMin, veryNearEndMinutes * 60))
        blinkMode = BlinkMode::VeryNearEnd;
    }

    blink->Start(blinkMode);
  }
  else
    blink->Start(BlinkMode::None);

  // serverova obrada eventualnog zahteva klijenta i gasenje WiFi-a x minuta posle paljenja aparata
  if (isWiFiOn)
  {
    server.handleClient();
    if (itvWiFiOn > 0 && ms > lastWebReq + itvWiFiOn * MIN)
      WiFiOff();

    // dugacak klik - startovanje OTA update-a
    // da se OTA ne bi slucajno enable-ovao uz startovanje WiFi-a, mora proci min 5 secs od ukljucivanja WiFi-a
    if (btn.clicks == -1 && ms > msWiFiStarted + noOtaTime)
      StartOtaUpdate();
  }
  else // WiFi OFF
  {
    // // uzimanje tacnog vremena npr 5min pre paljenja svetla
    // int timeCheckHour = autoStartHour;
    // int timeCheckMin = autoStartMin;
    // //* random(10) umesto fiksnih 5min
    // StrDateTime::TimeAddMin(timeCheckHour, timeCheckMin, -5);
    // if (now.IsItTime(timeCheckHour, timeCheckMin, 00))
    //   WiFiOn();
    
    if (tm.tm_yday % 1 == 0 && tm.tm_hour == 18 && tm.tm_sec == 0)
    {
        if (tm.tm_min == 45)
            WiFiOn();
        // if (tm.tm_min == 53)
        //     WiFiOff();
    }

    if (btn.clicks == -1) // dugacak klik - paljenje WiFi-a
      WiFiOn();
  }
}