# RelayESP

Turning on/off some electrical appliance that works on 110/220V (e.g. aquarium light):
1. Every day at some set time (e.g. it's ON 07:00 - 07:45 and the rest of the day is OFF).
2. Turn ON on click (web page button or device button). Appliance is ON for some specified time (e.g. 15min).

Usage:
Turning ON and OFF an electrical appliance with this device is set via web page found on http://192.168.0.x/ where x is defined in config.ini and in ReadConfigFile() in main.cpp.
- Short/normal device button clicks:
    - 1 click -> applience is ON for 10min
    - 2 clicks -> 20min ...
- Long click
    - if WiFi is OFF -> it will be turned ON
    - if WiFi is ON -> Over The Air update is enabled


## Web App
![RelayESP: Web App](https://github.com/bvujovic/RelayESP/blob/master/docs/pics/web_app.png)

## Device - Aquarium Light
![RelayESP: Device - Aquarium Light](https://github.com/bvujovic/RelayESP/blob/master/docs/pics/big_aq.jpg)

![RelayESP: Inside Device](https://github.com/bvujovic/RelayESP/blob/master/docs/pics/big_aq_inside.jpg)
