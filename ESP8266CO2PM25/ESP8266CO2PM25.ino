
/*
 * Measurement unit
 * CO2, Temperature, Humidity sensors communication module. 
 * Sends sensor measurements to MQTT
 * Senseair S8 (modbus) - CO2, 
 * MH-Z19(B) - CO2, 
 * Plantower PMS5003, PMS7003, PMSA003 - particle concentration sensors
 * HDC1080 - temperature + humidity, 
 * BMP280 - pressure + temperature, 
 * SI1145 - light sensor
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
#include <si1145.h>
#include <pmsx003.h>
#include <etools.h>
#include <pitimer.h>     // timers
#include <modbuspoll.h>
#include <xparam.h>
#include "general.h"

#define               PROGRAM_VERSION   "0.02"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL      logger

// device modbus address
#define               MODBUS_ADDRESS    0xfe
#define               CONNECTED_OBJ     modbus.Connected

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
#define SDAPIN   4
#define CLKPIN   5

// objects
SoftwareSerial mSerial1(13, 15); // RX, TX (13,15)
ModbusPoll       modbus;
piTimer          ptimer;
hdc1080          hdc;
bme280           bme;
si1145           si;
pmsx003          pms;

///////////////////////////////////////////////////////////////////////////
//   i2c
///////////////////////////////////////////////////////////////////////////
void i2cInit() {
  pinMode(SDAPIN, INPUT_PULLUP);
  pinMode(CLKPIN, INPUT_PULLUP);
  delay(200);
  bool isok = digitalRead(SDAPIN) && digitalRead(CLKPIN);
  if (!isok) {
    DEBUG_PRINTLN(F("I2C bus recovery..."));
    delay(500);
    //try i2c bus recovery at 100kHz = 5uS high, 5uS low
    pinMode(SDAPIN, OUTPUT_OPEN_DRAIN);//keeping SDA high during recovery
    pinMode(CLKPIN, OUTPUT_OPEN_DRAIN);
    digitalWrite(SDAPIN, HIGH);
    digitalWrite(CLKPIN, HIGH);
  
    for (int i = 0; i < 10; i++) { //9nth cycle acts as NACK
      digitalWrite(CLKPIN, HIGH);
      delayMicroseconds(5);
      digitalWrite(CLKPIN, LOW);
      delayMicroseconds(5);
    }

    //a STOP signal (SDA from low to high while CLK is high)
    digitalWrite(SDAPIN, LOW);
    delayMicroseconds(5);
    digitalWrite(CLKPIN, HIGH);
    delayMicroseconds(2);
    digitalWrite(SDAPIN, HIGH);
    delayMicroseconds(2);
    //bus status is now : FREE

    //return to power up mode
    pinMode(SDAPIN, INPUT_PULLUP);
    pinMode(CLKPIN, INPUT_PULLUP);
    delay(500);

    isok = digitalRead(SDAPIN) && digitalRead(CLKPIN);
    if (!digitalRead(SDAPIN))
      DEBUG_PRINTLN(F("I2C bus recovery  error: SDA still LOW."));  
    if (!digitalRead(CLKPIN))
      DEBUG_PRINTLN(F("I2C bus recovery error: CLK still LOW."));  
    
    if (isok)
      DEBUG_PRINTLN(F("I2C bus recovery complete."));  
  }
  
  Wire.pins(SDAPIN, CLKPIN); 
  Wire.begin(SDAPIN, CLKPIN);
}

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
  modbus.SetDeviceAddress(MODBUS_ADDRESS);
  modbus.SetLogger(&logger);
  modbus.SetSerial(&mSerial1); 
  DEBUG_PRINT(F("DeviceType: "));
  DEBUG_PRINTLN(params[F("device_type")]);
  modbus.ModbusSetup(params[F("device_type")].c_str());

  String str;
  modbus.getStrModbusConfig(str);
  DEBUG_PRINTLN(F("Modbus config:"));
  DEBUG_PRINTLN(str);

  //i2c
  i2cInit();

  // hdc1080
  hdc.begin(&logger);
  hdc.SetMQTT(&mqtt, SF("THConnected"), SF("Temperature"), SF("Humidity"), SF("Heater"));

  // BME280 i2c
  bme.begin(&logger);
  bme.SetMQTT(&mqtt, SF("TH2Connected"), SF("Temperature2"), SF("Humidity2"), SF("Pressure2"));

  // si1145 i2c
  si.begin(&logger);
  si.SetMQTT(&mqtt, SF("LightConnected"), SF("Visible"), SF("IR"), SF("UV"));

  // pmsX003 serial mode
  pms.begin(&logger, &Serial);
  pms.SetMQTT(&mqtt, SF("PMSConnected"), SF("PM1.0"), SF("PM2.5"), SF("PM10"));

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
      if (modbus.getWordValue(POLL_INPUT_REGISTERS, 0x1e) == 0) {
        modbus.PollAddress(0x19);
      }
      ptimer.Reset(TID_HOLD_REG);
    } else {
      // it needs because time from time seanseair s8 returns timeout.
      for(int i = 0; i < 4; i++) {
        modbus.PollAddress(0x00);
        if(modbus.Connected) break;
        delay(150);
      }
    };

    yield();

    // publish some system vars
    mqtt.BeginPublish();
    mqttPublishRegularState();

    // publish vars from configuration
    if (modbus.mapConfigLen && modbus.mapConfig && modbus.Connected) {
      String str;
      for(int i = 0; i < modbus.mapConfigLen; i++) {
        modbus.getValue(
          str,
          modbus.mapConfig[i].command,
          modbus.mapConfig[i].modbusAddress,
          modbus.mapConfig[i].valueType);
        mqtt.PublishState(modbus.mapConfig[i].mqttTopicName, str);
      }    
    };
    mqtt.Commit();
  
    ptimer.Reset(TID_POLL);
  }

  // HDC 1080
  hdc.handle();

  // BME280
  bme.handle();

  // si1145
  si.handle();

  // pmsX003 
  pms.handle();
  
  digitalWrite(LED2, LEDOFF);
  delay(100);
}

