
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
#include <modbuspoll.h>
#include <xparam.h>
#include "general.h"

#define               PROGRAM_VERSION      "0.5"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL         logger

// device modbus address
#define               MODBUS_ADDRESS       42
#define               MODBUS_OBJ_NAME      esp14

#define               DEEPSLEEP_INTERVAL   10*1000000  // 60 sec

// LEDs and pins
#define PIN_PGM  0      // programming pin
#define LED1     12     // led 1 TODO: delete leds
#define LED2     13     // led 2
#define LEDON    LOW
#define LEDOFF   HIGH

#define DI_PD2         0x04 
#define DI_PD3         0x08
#define DI_WIFISETUP   DI_PD2
#define DI_NODEEPSLEEP DI_PD3

// objects
ModbusPoll       esp14;

struct Esp14Var {
  int vLight;
  int vPressure;
  float vPrTemp;
  float vHumidity;
  float vTemp;
  uint16_t vDI;
} var {};

void PollAndPublish(bool withInit = false) {
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

  // poll
  esp14.Poll(POLL_INPUT_REGISTERS);
  if (!esp14.Connected) {
    DEBUG_EPRINTLN(F("Modbus device is not connected."));
    return;
  }

  // get values and normalize
  var.vLight    = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x00);
  var.vPressure = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x01) + 60000;
  var.vPrTemp   = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x02) / 100;
  var.vHumidity = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x03) / 10;
  var.vTemp     = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x04) / 10;
  var.vDI       = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x05);


  // wait wifi to connect   
  int cnt = 0;
  while (WiFi.status() != WL_CONNECTED && cnt < 60) { // 6 sec for connect
    delay(100);
    cnt ++;
  }
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_WPRINTLN(F("Wifi is not connected."));
    return;
  } else {
    DEBUG_EPRINTLN(SF("Wifi is connected. Timeout ms=") + String(cnt * 100));
  }

  // mqtt init and pub
  if (withInit) {
    initMQTT("esp14");
  }

  // publish some system vars
  if (withInit) {
    mqttPublishInitialState();
  }
  mqttPublishRegularState();

  // publish vars from configuration
  if (esp14.Connected) {
    String str;
    str = String(var.vLight);
    mqttPublishState("Light", str.c_str());        
    str = String(var.vPressure);
    mqttPublishState("Pressure", str.c_str());        
    str = String(var.vPrTemp);
    mqttPublishState("PrTemp", str.c_str());        
    str = String(var.vHumidity);
    mqttPublishState("Humidity", str.c_str());        
    str = String(var.vTemp);
    mqttPublishState("Temp", str.c_str());        
    str = String(var.vDI, HEX);
    mqttPublishState("DI", str.c_str());        
  };
}

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

  PollAndPublish(true);
  
  if ((var.vDI & DI_NODEEPSLEEP) != 0) {
    DEBUG_PRINTLN(F("Go to deep sleep..."));
    ESP.deepSleep(DEEPSLEEP_INTERVAL);
    delay(1000);
  }

  // config here
  
  // ArduinoOTA
  setupArduinoOTA();
}

void loop() {
  ArduinoOTA.handle();
  logger.handle();

  yield();

  PollAndPublish();

}
