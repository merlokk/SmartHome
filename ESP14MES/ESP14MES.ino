
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

#define USE_DEEPSLEEP

// device modbus address
#define               MODBUS_ADDRESS       42
#define               MODBUS_OBJ_NAME      esp14

// max deep sleep time ~71 minutes.
#define               SLEEP_INTERVAL   60  // 10 sec
#define               LIGHTSLEEP_INTERVAL   SLEEP_INTERVAL*1000 
#define               DEEPSLEEP_INTERVAL   LIGHTSLEEP_INTERVAL*1000  

// poll
#define MILLIS_TO_POLL          15*1000       //max time to wait for poll
// timers
#define TID_POLL                0x0001        // timer UID for poll 

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
piTimer          ptimer;

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
  if (withInit) {
    esp14.SetDeviceAddress(MODBUS_ADDRESS);
    esp14.SetLogger(&logger);
    DEBUG_PRINT(F("DeviceType: "));
    DEBUG_PRINTLN(params[F("device_type")]);
    esp14.ModbusSetup(params[F("device_type")].c_str());

    String str;
    esp14.getStrModbusConfig(str);
    DEBUG_PRINTLN(F("Modbus config:"));
    DEBUG_PRINTLN(str);
  }

  // poll
  esp14.Poll(POLL_INPUT_REGISTERS);
  if (!esp14.Connected) {
    DEBUG_EPRINTLN(F("Modbus device is not connected."));

    delay(1500);
    // if cant connect to STM8 - do setup
    esp14.Poll(POLL_INPUT_REGISTERS);
    if (!esp14.Connected) {
      DEBUG_EPRINTLN(F("Modbus device is not connected. Start confog portal..."));
      wifiSetup(false);
      delay(1000);
      restart();
    }
    
    if (!esp14.Connected) 
      return;
  }

  // get values and normalize
  var.vLight    = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x00);
  var.vPressure = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x01) + 60000;
  var.vPrTemp   = (float)esp14.getWordValue(POLL_INPUT_REGISTERS, 0x02) / 100;
  var.vHumidity = (float)esp14.getWordValue(POLL_INPUT_REGISTERS, 0x03) / 10;
  var.vTemp     = (float)esp14.getWordValue(POLL_INPUT_REGISTERS, 0x04) / 10;
  var.vDI       = esp14.getWordValue(POLL_INPUT_REGISTERS, 0x05);

  DEBUG_PRINTLN(SF("Temp=") + String(var.vTemp));

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
    DEBUG_PRINTLN(SF("Wifi is connected. Timeout ms=") + String(cnt * 100));
  }

  // mqtt init and pub
  if (withInit) {
    initMQTT("esp14");
  }
  mqtt.handle();
  
  // publish some system vars
  mqtt.BeginPublish();
  mqttPublishRegularState();

  // publish vars from configuration
  if (esp14.Connected) {
    mqtt.PublishState(SF("Light"), String(var.vLight));        
    mqtt.PublishState(SF("Pressure"), String(var.vPressure));        
    mqtt.PublishState(SF("PrTemp"), String(var.vPrTemp));        
    mqtt.PublishState(SF("Humidity"), String(var.vHumidity));        
    mqtt.PublishState(SF("Temp"), String(var.vTemp));        
    mqtt.PublishState(SF("DI"), String(var.vDI, HEX));        
  };
  mqtt.PublishState(SF("Uptimems"), String(millis()));        
  mqtt.Commit();
}

void setup() {  
  Serial.setDebugOutput(false);
  Serial1.setDebugOutput(true);
  Serial.begin(115200); 
  Serial1.begin(230400); 

  pinMode(PIN_PGM, OUTPUT);    
  digitalWrite(PIN_PGM, 1);
  
  sprintf(HARDWARE_ID, "%06X", ESP.getChipId());
  bool goSleep = true;

  // serial ports delay
  delay(100);
  
  // start logger
  initLogger();
  
  initPrintStartDebugInfo();

  WiFi.hostname(HARDWARE_ID);

  // setup xparam lib
  initXParam();
  
  // connect to default wifi
  WiFi.begin();

  // delay for rs-485 ready
  delay(600);

  PollAndPublish(true);

  // disable sleep
  goSleep = (params[F("device_type")].length() != 0) && (params[F("mqtt_server")].length() != 0) &&
    ((var.vDI & DI_NODEEPSLEEP) != 0);
  
  digitalWrite(PIN_PGM, 0);
  if (goSleep) {
#ifdef USE_DEEPSLEEP
    DEBUG_PRINTLN(SF("Time from start ms=") + String(millis()));
    DEBUG_PRINTLN(F("Go to deep sleep..."));
    ESP.deepSleep(DEEPSLEEP_INTERVAL);
    delay(1000);
#else
    DEBUG_PRINTLN(F("Go to light sleep..."));
    //wifi_station_disconnect();
    //wifi_set_opmode(NULL_MODE);
    wifi_set_sleep_type(LIGHT_SLEEP_T);
    delay(LIGHTSLEEP_INTERVAL); 
    wifi_set_sleep_type(NONE_SLEEP_T);
    delay(100);
    ESP.reset();
    delay(200);
#endif
  }

  ptimer.Add(TID_POLL, MILLIS_TO_POLL);
  
  // ArduinoOTA
  setupArduinoOTA();
}

void loop() {
  digitalWrite(PIN_PGM, 1);

  ArduinoOTA.handle();
  logger.handle();

  yield();

  if (ptimer.isArmed(TID_POLL)) {
    PollAndPublish();

    // check if we need wifi setup
    if ((var.vDI & DI_WIFISETUP) == 0) {
      wifiSetup(false);
      delay(1000);
      restart();
    }

    ptimer.Reset(TID_POLL);
  }

  digitalWrite(PIN_PGM, 0);

  delay(50);

/*
  if ((var.vDI & DI_NODEEPSLEEP) != 0) {
    wifi_set_sleep_type(LIGHT_SLEEP_T);
    delay(LIGHTSLEEP_INTERVAL); 
    wifi_set_sleep_type(NONE_SLEEP_T);
    delay(100);
  }*/
}
