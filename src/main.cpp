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

#include <SNTPtime.h>
SNTPtime ntp;
strDateTime now;

const int pinRelay = D1; // pin na koji je povezan relej koji pusta/prekida struju ka kontrolisanom potrosacu
const int pinBtn = D2;   // (INPUT_PULLUP) pin na koji je vezan taster za startovanje WiFi-a

#define DEBUG true

unsigned long msWiFiStarted;                  // vreme ukljucenja WiFi-a
const unsigned long WIFI_ON_INIT = 5 * 60000; // wifi ce biti ukljucen 5min po paljenju aparata, a onda se gasi
bool isWiFiOn;                                // da li je wifi ukljucen ili ne
bool autoOn, momentOn;
int autoStartHour, autoStartMin, autoEndHour, autoEndMin;
int momentStartHour, momentStartMin, momentEndHour, momentEndMin;
String appName; // sta pise u tabu index stranice pored "ESP Relay"
int ipLastNum;  // poslednji deo IP adrese - broj iza 192.168.0.
char configFilePath[] = "/dat/config.ini";

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

void ReadConfigFile()
{
  File fp = SPIFFS.open(configFilePath, "r");
  if (fp)
  {
    while (fp.available())
    {
      String l = fp.readStringUntil('\n');
      l.trim();
      if (l.length() == 0 || l.charAt(0) == ';') // preskacemo prazne stringove i komentare
        continue;                                //
      int idx = l.indexOf('=');                  // parsiranje reda u formatu "name=value", npr: moment_mins=10
      if (idx == -1)
        break;
      String name = l.substring(0, idx);
      String value = l.substring(idx + 1);

      if (name.equals("auto"))
        autoOn = value.equals("1");
      if (autoOn && name.equals("auto_from"))
        ParseTime(value, autoStartHour, autoStartMin);
      if (autoOn && name.equals("auto_to"))
        ParseTime(value, autoEndHour, autoEndMin);

      //B moment se vise nece pamtiti trajno
      // if (name.equals("moment"))
      //   momentOn = value.equals("1");
      if (momentOn && name.equals("moment_from"))
        ParseTime(value, momentStartHour, momentStartMin);
      if (momentOn && name.equals("moment_mins"))
      {
        int mins = value.toInt();
        momentEndMin = (momentStartMin + mins) % 60;
        int itvHours = (momentStartMin + mins) / 60;
        momentEndHour = (momentStartHour + itvHours) % 24;
      }
      Serial.println(1);
      if (name.equals("app_name"))
        appName = value;
      if (name.equals("ip_last_num"))
        ipLastNum = value.toInt();
      Serial.println(2);
    }
    fp.close();
  }
  else
    Serial.println("config.ini open (r) faaail.");
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

void WriteParamToFile(File &fp, const char *pname)
{
  fp.print(pname);
  fp.print('=');
  fp.println(server.arg(pname));
}

void HandleSaveConfig()
{
  // auto=1&auto_from=06:45&auto_to=07:20&moment=1&moment_from=22:59&moment_mins=5
  File fp = SPIFFS.open(configFilePath, "w");
  if (fp)
  {
    WriteParamToFile(fp, "auto");
    WriteParamToFile(fp, "auto_from");
    WriteParamToFile(fp, "auto_to");
    //B WriteParamToFile(fp, "moment");
    // moment (momentOn) se ne cuva trajno vec samo u radnoj memoriji
    momentOn = server.arg("moment") == "1";
    WriteParamToFile(fp, "moment_from");
    WriteParamToFile(fp, "moment_mins");
    WriteParamToFile(fp, "app_name");
    WriteParamToFile(fp, "ip_last_num");
    fp.close();
    ReadConfigFile();
  }
  else
    Serial.println("config.ini open (w) faaail.");

  server.send(200, "text/plain", "");
}

// Konektovanje na WiFi, uzimanje tacnog vremena, postavljanje IP adrese i startovanje veb servera.
void WiFiOn()
{
  Serial.println("Turning WiFi ON...");
  WiFi.mode(WIFI_STA);
  ConnectToWiFi();
  GetCurrentTime();
  SetupIPAddress(ipLastNum);
  server.on("/", []() { HandleDataFile(server, "/index.html", "text/html"); });
  server.on("/inc/favicon.ico", []() { HandleDataFile(server, "/inc/favicon.ico", "image/x-icon"); });
  server.on("/inc/script.js", []() { HandleDataFile(server, "/inc/script.js", "text/javascript"); });
  server.on("/inc/style.css", []() { HandleDataFile(server, "/inc/style.css", "text/css"); });
  server.on("/dat/config.ini", []() { HandleDataFile(server, "/dat/config.ini", "text/x-csv"); });
  server.on("/save_config", HandleSaveConfig);
  server.begin();
  isWiFiOn = true;
  msWiFiStarted = millis();
  Serial.println("WiFi ON");
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
  digitalWrite(LED_BUILTIN, true);
  pinMode(pinRelay, OUTPUT);
  pinMode(pinBtn, INPUT_PULLUP);

  Serial.begin(115200);
  SPIFFS.begin();
  ReadConfigFile();

  WiFiOn();
}

bool btnUp = true;

void loop()
{
  now = ntp.getTime(1.0, 1);
  if (DEBUG && now.second == 0)
  {
    ntp.printDateTime(now);
    delay(1000);
  }

  //T
  // if (now.second % 20 == 0)
  // {
  //   Serial.print("moment on: ");
  //   Serial.println(momentOn);
  //   delay(1000);
  // }

  // ukljucivanje/iskljucivanje releja
  bool isItOn = false;
  if (autoOn)
    isItOn = ntp.IsInInterval(now, autoStartHour, autoStartMin, autoEndHour, autoEndMin);
  if (!isItOn && momentOn)
    isItOn = ntp.IsInInterval(now, momentStartHour, momentStartMin, momentEndHour, momentEndMin);
  digitalWrite(pinRelay, isItOn);

  // (automatsko) iskljucivanje Moment opcije odmah po isteku moment intervala
  if (momentOn && ntp.IsItTime(now, momentEndHour, momentEndMin, 00))
    momentOn = false;

  // serverova obrada eventualnog zahteva klijenta i gasenje WiFi-a x minuta posle paljenja aparata
  if (isWiFiOn)
  {
    server.handleClient();
    if (millis() > msWiFiStarted + WIFI_ON_INIT)
      WiFiOff();
  }
  else // WiFi OFF
  {
    // periodicno uzimanje tacnog vremena, tacno u ponoc
    if (ntp.IsItTime(now, 00, 00, 00))
      WiFiOn();

    // na pritisnut taster pali se WiFi
    bool btnUpNew = digitalRead(pinBtn);
    if (btnUpNew != btnUp)
    {
      btnUp = btnUpNew;
      if (!btnUp)
        WiFiOn();
    }
  }

  delay(200);
}