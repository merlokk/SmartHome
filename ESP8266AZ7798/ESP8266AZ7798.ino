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

#define               PROGRAM_VERSION   "1.0"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL      logger

#define USE_SOFTWARE_SERIAL
#ifdef USE_SOFTWARE_SERIAL
#include <SoftwareSerial.h>
SoftwareSerial azSerial(13, 15); // RX, TX (13,15)
#endif

// for "connected"
#define               CONNECTED_OBJ      az.Connected()
#define               MQTT_DEFAULT_TOPIC "az7798"

// LEDs and pins
#define PIN_PGM  0      // programming pin and jumper
#define LED1     5      // led 1
#define LED2     4      // led 2
#define LEDON    LOW
#define LEDOFF   HIGH

az7798 az;
bool sentVersion = false;

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
  Serial1.begin(115200); // high speed logging port
  
#ifdef USE_SOFTWARE_SERIAL
  azSerial.begin(9600);
  Stream *ser = &azSerial;
#else
  Serial.begin(9600); 
  Serial.swap();
  Stream *ser = &Serial;
#endif
  
  delay(1000); // wait for serial

  generalSetup();

  //timer
  stimer.Add(TID_SEND, MILLIS_TO_SEND);

  // LED init
  pinMode(LED1, OUTPUT);    
  pinMode(LED2, OUTPUT);   
  digitalWrite(LED1, LEDOFF);
  digitalWrite(LED2, LEDOFF);

  az.begin(ser, &logger);

  // set password in work mode
  if (params[F("device_passwd")].length() > 0)
    logger.setPassword(params[F("device_passwd")].c_str());

  ticker.detach();
  digitalWrite(LED1, LEDOFF);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED1, LEDON);
  digitalWrite(LED2, LEDOFF);
  if (!generalLoop()) {
    digitalWrite(LED1, LEDOFF);
    return;
  }

  if (az.Connected()) {
    digitalWrite(LED2, LEDON);
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
      if (!sentVersion && az.GetVersion() != "") {
        mqtt.PublishState(SF("AZversion"), String(az.GetVersion()));        
        sentVersion = true;
      }
      
      mqtt.PublishState(SF("Temp"), String(az.GetTemperature()));        
      mqtt.PublishState(SF("Humidity"), String(az.GetHumidity()));        
      mqtt.PublishState(SF("CO2"), String(az.GetCO2()));        
    };
    mqtt.Commit();
    stimer.Reset(TID_SEND);
  }

  digitalWrite(LED1, LEDOFF);
  delay(200);
}


