#include <ESP8266httpUpdate.h>  // http updater https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266httpUpdate/src/ESP8266httpUpdate.h
#include <DNSServer.h>          // Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>        // https://github.com/merlokk/WiFiManager original:https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <Ticker.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time 
#include <NtpClientLib.h>       // https://github.com/gmag11/NtpClient

// macros for debugging
xLogger               logger;
#ifdef DEBUG
  // log level: info
  #define             DEBUG_PRINT(...)    logger.print(__VA_ARGS__)
  #define             DEBUG_PRINTLN(...)  logger.println(__VA_ARGS__)
  // log level: warning
  #define             DEBUG_WPRINT(...)   logger.print(llWarning, __VA_ARGS__)
  #define             DEBUG_WPRINTLN(...) logger.println(llWarning, __VA_ARGS__)
  // log level: error
  #define             DEBUG_EPRINT(...)   logger.print(llError, __VA_ARGS__)
  #define             DEBUG_EPRINTLN(...) logger.println(llError, __VA_ARGS__)
#else
  #define             DEBUG_PRINT(...)
  #define             DEBUG_PRINTLN(...)
  #define             DEBUG_WPRINT(...)
  #define             DEBUG_WPRINTLN(...)
  #define             DEBUG_EPRINT(...)
  #define             DEBUG_EPRINTLN(...)
#endif

// MQTT ID and topics
char                  HARDWARE_ID[7]        = {0};
char                  MQTT_STATE_TOPIC[30]  = {0};
const char*           MQTT_ON_PAYLOAD       = "ON";
const char*           MQTT_OFF_PAYLOAD      = "OFF";

// vars
bool inProgrammingMode = false;

// objects
Ticker           ticker;
WiFiClient       wifiClient;
PubSubClient     mqttClient(wifiClient);
xParam           params;



