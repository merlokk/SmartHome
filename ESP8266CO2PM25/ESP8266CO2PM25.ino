
/*
 * Measurement unit
 * CO2, Temperature, Humidity sensors communication module. 
 * Sends sensor measurements to MQTT
 * Senseair S8 (modbus) - CO2, 
 * MH-Z19(B) - CO2, 
 * Plantower PMS5003, PMS7003, PMSA003 - particle concentration sensors
 * HDC1080 - temperature + humidity, 
 * BMP280 - pressure + temperature, 
 * 
 * original repository here https://github.com/merlokk/SmartHome/tree/master/ESP8266CO2PM25
 * (c) Oleg Moiseenko 2017
 */
 
#include <Arduino.h>
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <xlogger.h>            // logger https://github.com/merlokk/xlogger
#include <SoftwareSerial.h>
#include <Wire.h>

// my libraries
#include <HDC1080.h>
#include <BME280.h>
#include <etools.h>
#include <pitimer.h>     // timers
#include <modbuspoll.h>
#include <xparam.h>
#include "general.h"

#define               PROGRAM_VERSION   "0.01"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL      logger

// device modbus address
#define               MODBUS_ADDRESS    0xfe
#define               CONNECTED_OBJ     eastron.Connected

#define               MQTT_DEFAULT_TOPIC "MesUnit"

// poll
#define MILLIS_TO_POLL          10*1000       //max time to wait for poll input registers (regular poll)
#define MILLIS_TO_POLL_HOLD_REG 10*60*1000    //max time to wait for poll all 15*60
#define MILLIS_TO_MES           10*1000       //max time to measurements
// timers
#define TID_POLL                0x0001        // timer UID for regular poll 
#define TID_HOLD_REG            0x0002        // timer UID for poll all the registers
#define TID_MES                 0x0003        // timer UID for measurements

// LEDs and pins
#define PIN_PGM  0      // programming pin and jumper
#define LED1     12     // led 1
#define LED2     14     // led 2
#define LEDON    LOW
#define LEDOFF   HIGH

// objects
SoftwareSerial mSerial1(13, 15); // RX, TX (13,15)
ModbusPoll       eastron;
piTimer          ptimer;
hdc1080          hdc;
bme280           bme;

///////////////////////////////////////////////////////////////////////////
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.setDebugOutput(false);
  Serial1.setDebugOutput(true);
  Serial.begin(9600);
  Serial1.begin(230400); // high speed logging port

  mSerial1.begin(9600);

  generalSetup();

  //timer
  ptimer.Add(TID_POLL, MILLIS_TO_POLL);
  ptimer.Add(TID_HOLD_REG, MILLIS_TO_POLL_HOLD_REG);
  ptimer.Add(TID_MES, MILLIS_TO_MES);

  // LED init
  pinMode(LED1, OUTPUT);    
  pinMode(LED2, OUTPUT);   
  digitalWrite(LED1, LEDOFF);
  digitalWrite(LED2, LEDOFF);

  // eastron setup
  eastron.SetDeviceAddress(MODBUS_ADDRESS);
  eastron.SetLogger(&logger);
  eastron.SetSerial(&mSerial1);
  DEBUG_PRINT(F("DeviceType: "));
  DEBUG_PRINTLN(params[F("device_type")]);
  eastron.ModbusSetup(params[F("device_type")].c_str());

  String str;
  eastron.getStrModbusConfig(str);
  DEBUG_PRINTLN(F("Modbus config:"));
  DEBUG_PRINTLN(str);

  // hdc1080
  Wire.begin(4, 5); // (SDA, SCL)
  hdc.begin(&logger);
  hdc.SetMQTT(&mqtt, SF("THConnected"), SF("Temperature"), SF("Humidity"), SF("Heater"));

  // BME280 i2c
  bme.begin(&logger);
  bme.SetMQTT(&mqtt, SF("THConnected"), SF("Temperature"), SF("Humidity"), SF("Pressure"));

  // set password in work mode
  if (params[F("device_passwd")].length() > 0)
    logger.setPassword(params[F("device_passwd")].c_str());

  ticker.detach();

  delay(2000); // start sensors
  
  digitalWrite(LED1, LEDOFF);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED2, LEDON);
  if (!generalLoop()) {
    digitalWrite(LED2, LEDOFF);
    return;
  }

  if (ptimer.isArmed(TID_POLL)) {
    // modbus poll function
    if (ptimer.isArmed(TID_HOLD_REG)) {
      if (eastron.getWordValue(POLL_INPUT_REGISTERS, 0x1e) == 0) {
        eastron.PollAddress(0x19);
      }
      ptimer.Reset(TID_HOLD_REG);
    } else {
      // it needs because time from time seanseair s8 returns timeout.
      for(int i = 0; i < 4; i++) {
        eastron.PollAddress(0x00);
        if(eastron.Connected) break;
        delay(100);
      }
    };

    yield();

    // publish some system vars
    mqtt.BeginPublish();
    mqttPublishRegularState();

    // publish vars from configuration
    if (eastron.mapConfigLen && eastron.mapConfig && eastron.Connected) {
      String str;
      for(int i = 0; i < eastron.mapConfigLen; i++) {
        eastron.getValue(
          str,
          eastron.mapConfig[i].command,
          eastron.mapConfig[i].modbusAddress,
          eastron.mapConfig[i].valueType);
        mqtt.PublishState(eastron.mapConfig[i].mqttTopicName, str);        
      }    
    };
    mqtt.Commit();
  
    ptimer.Reset(TID_POLL);
  }

  // HDC 1080
  hdc.handle();

  // BME280
  //bme.handle();
  
  digitalWrite(LED2, LEDOFF);
  delay(100);
}

