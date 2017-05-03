
/*
 * ESP-14 weather module. 
 * temperature/humidity/light/pressure to MQTT
 * HDC1080 - temperature + humidity, 
 * BMP280 - pressure + temperature, 
 * BH1750FVI - light, 
 * ESP-14 - cpu module - ESP8266 + STM8003
 * 
 * original repository here https://github.com/merlokk/SmartHome/tree/master/ESP14MES
 * (c) Oleg Moiseenko 2017
 */
 
 
#include <Arduino.h>
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <xlogger.h>            // logger https://github.com/merlokk/xlogger
// my libraries
#include <etools.h>
#include <pitimer.h>     // timers
#include <eastron.h>
#include <xparam.h>
#include "general.h"

#define               PROGRAM_VERSION   "0.5"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL      logger

// LEDs and pins
#define PIN_PGM  0      // programming pin
#define LED1     12     // led 1
#define LEDON    LOW
#define LEDOFF   HIGH


void setup() {  
  Serial.begin(115200); 
  // ESP.rtcUserMemoryWrite(offset, &data, sizeof(data)) and ESP.rtcUserMemoryRead(offset, &data, sizeof(data)) 512 bytes - live between sleeps

  ESP.deepSleep(60 * 1000000); // 60 sec
}

void loop() {


}
