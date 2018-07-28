
# Projects for smart home. ESP8266 + NanoPi NEO Air
[![Build Status](https://travis-ci.org/merlokk/SmartHome.svg?branch=master)](https://travis-ci.org/merlokk/SmartHome)

### ESP8266AZ7788
Connect Datalogger AZ-7788 to MQTT

### ESP8266EASTRON
Connect Eastron energy meters (SDM220, SDM230, SDM630) to MQTT

[wiki](https://github.com/merlokk/SmartHome/wiki/ESP8266-to-Eastron-energy-meters)

release in the box:
![release](https://github.com/merlokk/SmartHome/blob/master/ESP8266EASTRON/images/DSC_0119.JPG)

### ESP8266DISABLECPU
How do disable CPU and put it into deep sleep mode. It needs for debug ESP-14

### ESP14MES
connect temperature/humidity/light/pressure to MQTT. 

![ESP-14](https://raw.github.com/merlokk/SmartHome/master/ESP14MES/docs/img2.jpg "ESP-14")

**HDC1080** - temp + humidity, **BMP280** - pressure + temp, **BH1750FVI** - light, **ESP-14** - cpu (ESP8266 + STM8003)

board https://www.aliexpress.com/item/3-5V-Multi-HDC1080-Temperature-Humidity-Sensor-BMP280-Pressure-Control-Sensor-ESP8266-WIFI-BH1750FVI-Module/32742652977.html

ESP-14 info https://esp8266.ru/forum/threads/esp-14-chto-ehto.531/

STM8 sensor reading part https://bitbucket.org/hrandib/esp-stm8-sensors

### ESP8266CO2PM25
Air quality monitor. [Code here](https://github.com/merlokk/SmartHome/tree/master/ESP8266CO2PM25)

It measures parameters:
* Humidity
* Temperature
* Pressure
* CO2
* PM1.0/PM2.5/PM10 dust
* Light: Visible / UV / UV index
