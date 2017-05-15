/*
 * AZ7798 CO2 datalogger bridge. 
 * 
 * AZ7798 -> RS-232 TTL -> esp-8266 -> WiFi -> MQTT
 * 
 * original repository here https://github.com/merlokk/SmartHome/tree/master/ESP8266AZ7788
 * (c) Oleg Moiseenko 2017
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>  // https://github.com/esp8266/Arduino
#include <xlogger.h>      // logger https://github.com/merlokk/xlogger
// my libraries
#include <etools.h>
#include <pitimer.h>      // timers
#include <xparam.h>
#include <az7798.h>
#include "general.h"

#define               PROGRAM_VERSION   "0.2"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL      logger

// for "connected"
#define               CONNECTED_OBJ      az.Connected()

// LEDs and pins
#define PIN_PGM  0      // programming pin and jumper
#define LED1     12     // led 1
#define LED2     14     // led 2
#define LEDON    LOW
#define LEDOFF   HIGH

az7798 az;

piTimer stimer;
// poll
#define MILLIS_TO_SEND          15*1000       //max time to wait 
// timers
#define TID_SEND                0x0001        // timer UID for send data to mqtt 

///////////////////////////////////////////////////////////////////////////
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.setDebugOutput(false);
  Serial1.setDebugOutput(true);
  Serial.begin(9600); 
  Serial1.begin(115200); // high speed logging port

  generalSetup();

  az.begin(&Serial, &logger);

  //timer
  stimer.Add(TID_SEND, MILLIS_TO_SEND);

  // LED init
  pinMode(LED1, OUTPUT);    
  pinMode(LED2, OUTPUT);   
  digitalWrite(LED1, LEDOFF);
  digitalWrite(LED2, LEDOFF);


  // set password in work mode
  if (params[F("device_passwd")].length() > 0)
    logger.setPassword(params[F("device_passwd")].c_str());

  ticker.detach();
  digitalWrite(LED1, LEDOFF);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED2, LEDON);
  if (!generalLoop()) {
    digitalWrite(LED2, LEDOFF);
    return;
  }

  yield();

  az.handle();

  yield();
  
  if (stimer.isArmed(TID_SEND)) {
    // publish some system vars
    mqtt.BeginPublish();
    mqttPublishRegularState();

    // publish vars 
    if (az.Connected()) {
      mqtt.PublishState(SF("Temp"), String(az.GetTemperature()));        
      mqtt.PublishState(SF("Humidity"), String(az.GetHumidity()));        
      mqtt.PublishState(SF("CO2"), String(az.GetCO2()));        
    };
/*    if (eastron.mapConfigLen && eastron.mapConfig && eastron.Connected) {
      String str;
      for(int i = 0; i < eastron.mapConfigLen; i++) {
        eastron.getValue(
          str,
          eastron.mapConfig[i].command,
          eastron.mapConfig[i].modbusAddress,
          eastron.mapConfig[i].valueType);
        mqtt.PublishState(eastron.mapConfig[i].mqttTopicName, str);        
      }    
    };*/
    mqtt.Commit();
    stimer.Reset(TID_SEND);
  }

  digitalWrite(LED2, LEDOFF);
  delay(100);
}


