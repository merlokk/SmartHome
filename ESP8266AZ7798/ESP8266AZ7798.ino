/*
 * AZ7798 CO2 datalogger bridge. 
 * 
 * AZ7798 -> RS-232 TTL -> esp-8266 -> WiFi -> MQTT
 * 
 * original repository here https://github.com/merlokk/SmartHome/tree/master/ESP8266AZ7788
 * (c) Oleg Moiseenko 2017
 */
#include <Arduino.h>
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <xlogger.h>            // logger https://github.com/merlokk/xlogger
// my libraries
#include <etools.h>
#include <pitimer.h>     // timers
#include <xparam.h>
#include "general.h"

#define               PROGRAM_VERSION   "0.2"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL      logger

// LEDs and pins
#define PIN_PGM  0      // programming pin and jumper
#define LED1     12     // led 1
#define LED2     14     // led 2
#define LEDON    LOW
#define LEDOFF   HIGH

///////////////////////////////////////////////////////////////////////////
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.setDebugOutput(false);
  Serial1.setDebugOutput(true);
  Serial.begin(115200); //74880
  Serial1.begin(230400); // high speed logging port

  generalSetup();

  //timer
  ptimer.Add(TID_POLL, MILLIS_TO_POLL);
  ptimer.Add(TID_HOLD_REG, MILLIS_TO_POLL_HOLD_REG);

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

    // publish some system vars
    mqtt.BeginPublish();
    mqttPublishRegularState();

    // publish vars from configuration
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
  
  }
  
  digitalWrite(LED2, LEDOFF);
  delay(100);
}


