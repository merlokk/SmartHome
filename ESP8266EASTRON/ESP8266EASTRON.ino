/*

*/

#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino
#include <DNSServer.h>      //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <Ticker.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

#include "eastron.h"

#define               DEBUG                            // enable debugging

// macros for debugging
#ifdef DEBUG
  #define             DEBUG_PRINT(x)    Serial.print(x)
  #define             DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define             DEBUG_PRINT(x)
  #define             DEBUG_PRINTLN(x)
#endif

// LEDs and pins
#define PIN_PGM 0
#define LED1 12
#define LED2 14
#define LEDON  LOW
#define LEDOFF HIGH

// MQTT defines
#define               MQTT_CFG_CHAR_ARRAY_SIZE 24      // size of the arrays for MQTT username, password, etc.
#define               MQTT_CFG_CHAR_ARRAY_SIZE_PORT 6  // size of the arrays for MQTT port

// MQTT ID and topics
char                  MQTT_CLIENT_ID[7]                                           = {0};
char                  MQTT_STATE_TOPIC[MQTT_CFG_CHAR_ARRAY_SIZE] = {0};
const char*           MQTT_ON_PAYLOAD                                             = "ON";
const char*           MQTT_OFF_PAYLOAD                                            = "OFF";

// MQTT settings
#define EEPROM_SALT 123321
typedef struct {
  char                mqttUser[MQTT_CFG_CHAR_ARRAY_SIZE]        = {0};
  char                mqttPassword[MQTT_CFG_CHAR_ARRAY_SIZE]    = {0};
  char                mqttServer[MQTT_CFG_CHAR_ARRAY_SIZE]      = {0};
  char                mqttPort[MQTT_CFG_CHAR_ARRAY_SIZE_PORT]   = {0};
  char                mqttPath[MQTT_CFG_CHAR_ARRAY_SIZE]        = {0};
  char                deviceType[4]                             = {0};
  int                 salt                                      = EEPROM_SALT; 
} EEPROM_settings;

// vars
bool inProgrammingMode = false;

// objects
EEPROM_settings  settings;
Ticker           ticker;
WiFiClient       wifiClient;
PubSubClient     mqttClient(wifiClient);
Eastron          eastron;

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////

/*
  Function called to publish the state of 
*/
void mqttPublishState() {
  if (mqttClient.publish(MQTT_STATE_TOPIC, MQTT_ON_PAYLOAD, true)) {
    DEBUG_PRINT(F("INFO: MQTT message publish succeeded. Topic: "));
    DEBUG_PRINT(MQTT_STATE_TOPIC);
    DEBUG_PRINT(F(". Payload: "));
    DEBUG_PRINTLN(MQTT_ON_PAYLOAD);
  } else {
    DEBUG_PRINTLN(F("ERROR: MQTT message publish failed"));
  }

}

/*
  Function called to connect/reconnect to the MQTT broker
*/
void reconnect() {
  uint8_t i = 0;
  while (!mqttClient.connected()) {
    if (mqttClient.connect(MQTT_CLIENT_ID, settings.mqttUser, settings.mqttPassword)) {
      DEBUG_PRINTLN(F("INFO: The client is successfully connected to the MQTT broker"));
    } else {
      DEBUG_PRINTLN(F("ERROR: The connection to the MQTT broker failed"));
      DEBUG_PRINT(F("Username: "));
      DEBUG_PRINTLN(settings.mqttUser);
      DEBUG_PRINT(F("Password: "));
      DEBUG_PRINTLN(settings.mqttPassword);
      DEBUG_PRINT(F("Broker: "));
      DEBUG_PRINT(settings.mqttServer);
      DEBUG_PRINT(F(":"));
      DEBUG_PRINTLN(settings.mqttPort);
      delay(1000);
      if (i == 3) {
        reset();
      }
      i++;
    }
  }
}

///////////////////////////////////////////////////////////////////////////
//   WiFiManager
///////////////////////////////////////////////////////////////////////////
/*
  Function called to toggle the state of the LED
*/
void tick() {
  digitalWrite(LED1, !digitalRead(LED1));
}

// flag for saving data
bool shouldSaveConfig = false;

// callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  ticker.attach(0.2, tick);
}

void wifiSetup(bool withAutoConnect) {
  shouldSaveConfig = false;
  ticker.attach(0.1, tick);

  // load custom params
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();

  if (settings.salt != EEPROM_SALT) {
    DEBUG_PRINTLN(F("ERROR: Invalid settings in EEPROM, settings was cleared."));
    EEPROM_settings defaults;
    settings = defaults;
  }

  WiFiManager wifiManager;

  WiFiManagerParameter custom_mqtt_text("<br/>MQTT config: <br/>");
  wifiManager.addParameter(&custom_mqtt_text);
  WiFiManagerParameter custom_mqtt_user("mqtt-user", "MQTT User", settings.mqttUser, MQTT_CFG_CHAR_ARRAY_SIZE);
  wifiManager.addParameter(&custom_mqtt_user);
  WiFiManagerParameter custom_mqtt_password("mqtt-password", "MQTT Password", settings.mqttPassword, MQTT_CFG_CHAR_ARRAY_SIZE, "type = \"password\"");
  wifiManager.addParameter(&custom_mqtt_password);
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker Address", settings.mqttServer, MQTT_CFG_CHAR_ARRAY_SIZE);
  wifiManager.addParameter(&custom_mqtt_server);
  WiFiManagerParameter custom_mqtt_port("mqtt-port", "MQTT Broker Port", settings.mqttPort, MQTT_CFG_CHAR_ARRAY_SIZE_PORT);
  wifiManager.addParameter(&custom_mqtt_port);
  WiFiManagerParameter custom_mqtt_path("mqtt-path", "MQTT Path", settings.mqttPath, MQTT_CFG_CHAR_ARRAY_SIZE);
  wifiManager.addParameter(&custom_mqtt_path);
  WiFiManagerParameter custom_device_type("device-type", "Device type", settings.deviceType, 4);
  wifiManager.addParameter(&custom_device_type);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);
  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (withAutoConnect){
    if (!wifiManager.autoConnect()) { 
      ESP.reset();
      delay(1000);
    }
  } else {
    String ssid = "ESP" + String(ESP.getChipId());
    if (!wifiManager.startConfigPortal(ssid.c_str())) { 
      ESP.reset();
      delay(1000);
    }
  }
  
  if (shouldSaveConfig) {
    strcpy(settings.mqttServer,   custom_mqtt_server.getValue());
    strcpy(settings.mqttPort,     custom_mqtt_port.getValue());
    strcpy(settings.mqttUser,     custom_mqtt_user.getValue());
    strcpy(settings.mqttPassword, custom_mqtt_password.getValue());
    strcpy(settings.mqttPath,     custom_mqtt_path.getValue());
    strcpy(settings.deviceType,   custom_device_type.getValue());

    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  }

  ticker.detach();
}

///////////////////////////////////////////////////////////////////////////
//   ESP
///////////////////////////////////////////////////////////////////////////
/*
  Function called to restart the switch
*/
void restart() {
  DEBUG_PRINTLN(F("INFO: Restart..."));
  ESP.reset();
  delay(1000);
}

/*
  Function called to reset the configuration of the switch
*/
void reset() {
  DEBUG_PRINTLN(F("INFO: Reset..."));
  WiFi.disconnect();
  delay(1000);
  ESP.reset();
  delay(1000);
}

///////////////////////////////////////////////////////////////////////////
//   ArduinoOTA
///////////////////////////////////////////////////////////////////////////

void setupArduinoOTA() {
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINT(F("ERROR: OTA error: "));
    DEBUG_PRINTLN(error);
    switch (error) {
      case OTA_AUTH_ERROR:    DEBUG_PRINTLN(F("OTA ERROR: Auth Failed")); break;
      case OTA_BEGIN_ERROR:   DEBUG_PRINTLN(F("OTA ERROR: Begin Failed")); break;
      case OTA_CONNECT_ERROR: DEBUG_PRINTLN(F("OTA ERROR: Connect Failed")); break;
      case OTA_RECEIVE_ERROR: DEBUG_PRINTLN(F("OTA ERROR: Receive Failed")); break;
      case OTA_END_ERROR:     DEBUG_PRINTLN(F("OTA ERROR: End Failed")); break;
    }

    ESP.restart();
  });
  
  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN(F("INFO: Start OTA"));
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN(F("INFO: End OTA"));
  });
  ArduinoOTA.setHostname(MQTT_CLIENT_ID);
  ArduinoOTA.begin();
}

///////////////////////////////////////////////////////////////////////////
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200); //74880
//  Serial1.setDebugOutput(true);

  // for debug
  delay(200);
  DEBUG_PRINTLN(F(" "));
  DEBUG_PRINTLN(F("Starting..."));

  // LED init
  pinMode(LED1, OUTPUT);    
  pinMode(LED2, OUTPUT);   
  digitalWrite(LED1, LEDOFF);
  digitalWrite(LED2, LEDOFF);

  ticker.attach(0.1, tick);

  // 0 = programming mode
  pinMode(PIN_PGM, INPUT);   
  inProgrammingMode = !digitalRead(PIN_PGM);
  if (inProgrammingMode) {
    DEBUG_PRINTLN(F("WARNINIG: Programming mode active!"));
  }

  // client ID
  sprintf(MQTT_CLIENT_ID, "%06X", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT client ID/Hostname: "));
  DEBUG_PRINTLN(MQTT_CLIENT_ID);

  // wifi setup with autoconnect
  wifiSetup(true);

  // configure mqtt topic
  if (strlen(settings.mqttPath) == 0) {
    sprintf(MQTT_STATE_TOPIC, "%s/power_meter/", MQTT_CLIENT_ID);
  } else {
    sprintf(MQTT_STATE_TOPIC, "%s/", settings.mqttPath);
  }
  DEBUG_PRINT(F("INFO: MQTT command topic: "));
  DEBUG_PRINTLN(MQTT_STATE_TOPIC);


  // configure MQTT
  mqttClient.setServer(settings.mqttServer, atoi(settings.mqttPort));

  // connect to the MQTT broker
  reconnect();


  // ArduinoOTA
  setupArduinoOTA();

  ticker.detach();

  mqttPublishState();
}

// the loop function runs over and over again forever
void loop() {
  // Programming pin activates setup
  if (!inProgrammingMode && (!digitalRead(PIN_PGM))) {
    DEBUG_PRINTLN(F("Wifi setup activated"));

    wifiSetup(false);
    delay(1000);
  }
  
  digitalWrite(LED2, LEDON);
  ArduinoOTA.handle();

  yield();

  // keep the MQTT client connected to the broker
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  yield();

  if (!mqttClient.connected()){
    digitalWrite(LED2, LEDOFF);
    return;
  }

  eastron.Poll(POLL_ALL);

  if (eastron.Connected) {
    
  };
  
  //mqttPublishState();

  delay(10000);
 
  digitalWrite(LED2, LEDOFF);
}
