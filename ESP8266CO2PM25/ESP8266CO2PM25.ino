
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
#include <ClosedCube_HDC1080.h> // https://github.com/closedcube/ClosedCube_HDC1080_Arduino
// my libraries
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
ClosedCube_HDC1080 hdc1080;
char HDCSerial[12] = {0};

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
  hdc1080.begin(0x40);

  hdc1080.GetLastError(); 
  uint16_t HDC1080MID = hdc1080.readManufacturerId();
  uint8_t err = hdc1080.GetLastError(); 
  if (!err && HDC1080MID != 0x0000 && HDC1080MID != 0xFFFF) {
    // sensor online
    DEBUG_PRINTLN(SF("HDC1080 manufacturer ID=0x") + String(HDC1080MID, HEX)); // 0x5449 ID of Texas Instruments
    DEBUG_PRINTLN(SF("HDC1080 device ID=0x") + String(hdc1080.readDeviceId(), HEX)); // 0x1050 ID of the device
    HDC1080_SerialNumber sernum = hdc1080.readSerialNumber();
    sprintf(HDCSerial, "%02X-%04X-%04X", sernum.serialFirst, sernum.serialMid, sernum.serialLast);
    DEBUG_PRINTLN(SF("HDC1080 Serial Number=") + String(HDCSerial));

 //   hdc1080.heatUp(10); // heat by every start
    hdc1080.setResolution(HDC1080_RESOLUTION_11BIT, HDC1080_RESOLUTION_11BIT);
  } else {
    DEBUG_PRINTLN(SF("HDC1080 sensor offline. error:") + String(err));
  }

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
  if (ptimer.isArmed(TID_MES)) {
    HDC1080_Registers reg = hdc1080.readRegister();
    DEBUG_PRINTLN(SF("Heater: ") + String(reg.Heater, BIN));
    mqtt.PublishState("Heater", String(reg.Heater));

 /*   if (reg.Heater) {
      DEBUG_PRINTLN("Try to clear heating state...");
      reg.Heater = 0;
      hdc1080.writeRegister(reg);
    }*/

    double Temp = hdc1080.readTemperature();
    double Hum = hdc1080.readHumidity();
    uint8_t err = hdc1080.GetLastError(); 
    if (!err) {
      DEBUG_PRINTLN(SF("T=") + String(Temp) + SF("C, RH=") + String(Hum) + "%");
      mqtt.PublishState(SF("Temperature"), String(Temp));
      mqtt.PublishState(SF("Humidity"), String(Hum));
      mqtt.PublishState(SF("THConnected"), SF("ON"));
    } else {
      mqtt.PublishState(SF("THConnected"), SF("OFF"));
      DEBUG_PRINTLN("HDC1080 I2C error: " + String(err));
    }

    ptimer.Reset(TID_MES);
  }
 
  
  digitalWrite(LED2, LEDOFF);
  delay(100);
}

