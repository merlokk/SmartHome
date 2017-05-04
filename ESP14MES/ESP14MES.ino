
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

// device modbus address
#define               MODBUS_ADDRESS    42
#define               MODBUS_OBJ_NAME   esp14

// LEDs and pins
#define PIN_PGM  0      // programming pin
#define LED1     12     // led 1 TODO: delete leds
#define LED2     13     // led 2
#define LEDON    LOW
#define LEDOFF   HIGH

// objects
ModbusPoll       esp14;

void setup() {  
  Serial.setDebugOutput(false);
  Serial1.setDebugOutput(true);
  Serial.begin(115200); 
  Serial1.begin(115200); 

  pinMode(PIN_PGM, OUTPUT);    
  digitalWrite(PIN_PGM, LEDOFF);
  
  sprintf(HARDWARE_ID, "%06X", ESP.getChipId());

  // start logger
  initLogger();
  
  delay(200);
  initPrintStartDebugInfo();

  WiFi.hostname(HARDWARE_ID);

  // setup xparam lib
  initXParam();
  
  // connect to default wifi
  WiFi.begin();

  // modbus poll
  esp14.SetDeviceAddress(MODBUS_ADDRESS);
  esp14.SetLogger(&logger);
  DEBUG_PRINT(F("DeviceType: "));
  DEBUG_PRINTLN(params[F("device_type")]);
  esp14.ModbusSetup(params[F("device_type")].c_str());

  String str;
  esp14.getStrModbusConfig(str);
  DEBUG_PRINTLN(F("Modbus config:"));
  DEBUG_PRINTLN(str);

  // wait wifi to connect   
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_WPRINTLN(F("Wifi is not connected. Wait..."));
    delay(5000);
  }

  // mqtt init and pub
  initMQTT("esp14");



  ESP.deepSleep(60 * 1000000); // 60 sec

  // config here
}

void loop() {


}
