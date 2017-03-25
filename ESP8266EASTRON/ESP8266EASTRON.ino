/*

*/

#include <ESP8266WiFi.h>    // https://github.com/esp8266/Arduino
#include <DNSServer.h>      //Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>   // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <Ticker.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>

#define               DEBUG                       // enable debugging
#define               STRUCT_CHAR_ARRAY_SIZE 24   // size of the arrays for MQTT username, password, etc.

// macros for debugging
#ifdef DEBUG
  #define             DEBUG_PRINT(x)    Serial.print(x)
  #define             DEBUG_PRINTLN(x)  Serial.println(x)
#else
  #define             DEBUG_PRINT(x)
  #define             DEBUG_PRINTLN(x)
#endif

// LEDs
#define LED1 12
#define LED2 14
#define LEDON  LOW
#define LEDOFF HIGH

// MQTT ID and topics
char                  MQTT_CLIENT_ID[7]                                           = {0};
char                  MQTT_STATE_TOPIC[STRUCT_CHAR_ARRAY_SIZE] = {0};
const char*           MQTT_ON_PAYLOAD                                             = "ON";
const char*           MQTT_OFF_PAYLOAD                                            = "OFF";

// MQTT settings
typedef struct {
  char                mqttUser[STRUCT_CHAR_ARRAY_SIZE]      = {0};
  char                mqttPassword[STRUCT_CHAR_ARRAY_SIZE]  = {0};
  char                mqttServer[STRUCT_CHAR_ARRAY_SIZE]    = {0};
  char                mqttPort[5]                           = {0};
} Settings;

Settings      settings;
Ticker        ticker;
WiFiClient    wifiClient;
PubSubClient  mqttClient(wifiClient);

///////////////////////////////////////////////////////////////////////////
//   eastron   TODO: put it into separate module!
///////////////////////////////////////////////////////////////////////////

// original here https://github.com/beireken/SDM220t/blob/master/SDM.h thanks!
//SDM 220 registers
#define SDM220T_VOLTAGE                     0x0000                              //V
#define SDM220T_CURRENT                     0x0006                              //A
#define SDM220T_POWER                       0x000C                              //W
#define SDM220T_ACTIVE_APPARENT_POWER       0x0012                              //VA
#define SDM220T_REACTIVE_APPARENT_POWER     0x0018                              //VAR
#define SDM220T_POWER_FACTOR                0x001E                              //
#define SDM220T_PHASE_ANGLE                 0x0024                              //DEGREE
#define SDM220T_FREQUENCY                   0x0046                              //Hz
#define SDM220T_IMPORT_ACTIVE_ENERGY        0x0048                              //Wh
#define SDM220T_EXPORT_ACTIVE_ENERGY        0x004A                              //Wh
#define SDM220T_IMPORT_REACTIVE_ENERGY      0x004C                              //VARh
#define SDM220T_EXPORT_REACTIVE_ENERGY      0x004E                              //VARh
#define SDM220T_TOTAL_ACTIVE_ENERGY         0x0156                              //Wh
#define SDM220T_TOTAL_REACTIVE_ENERGY   0x0158                              //VARh
//SDM 630 registers
#define SDM630_VOLTAGE1                     0x0000                              //V
#define SDM630_VOLTAGE2                     0x0002                              //V
#define SDM630_VOLTAGE3                     0x0004                              //V
#define SDM630_CURRENT1                     0x0006                              //A
#define SDM630_CURRENT2                     0x0008                              //A
#define SDM630_CURRENT3                     0x000A                              //A
#define SDM630_CURRENTSUM                   0x0030                              //A
#define SDM630_POWER1                       0x000C                              //W
#define SDM630_POWER2                       0x000E                              //W
#define SDM630_POWER3                       0x0010                              //W
#define SDM630_POWERTOTAL                   0x0034                              //W
#define SDM630_VOLT_AMPS1                   0x0012                              //VA
#define SDM630_VOLT_AMPS2                   0x0014                              //VA
#define SDM630_VOLT_AMPS3                   0x0016                              //VA
#define SDM630_VOLT_AMPS_TOTAL              0x0038                              //VA
#define SDM630_VOLT_AMPS_REACTIVE1          0x0018                              //VAr
#define SDM630_VOLT_AMPS_REACTIVE2          0x001A                              //VAr
#define SDM630_VOLT_AMPS_REACTIVE3          0x001C                              //VAr
#define SDM630_VOLT_AMPS_REACTIVE_TOTAL     0x003C                              //VAr
#define SDM630_POWER_FACTOR1                0x001E
#define SDM630_POWER_FACTOR2                0x0020
#define SDM630_POWER_FACTOR3                0x0022
#define SDM630_POWER_FACTOR_TOTAL           0x003E
#define SDM630_PHASE_ANGLE1                 0x0024                              //Degrees
#define SDM630_PHASE_ANGLE2                 0x0026                              //Degrees
#define SDM630_PHASE_ANGLE3                 0x0028                              //Degrees
#define SDM630_PHASE_ANGLE_TOTAL            0x0042                              //Degrees
#define SDM630_VOLTAGE_AVERAGE              0x002A                              //V
#define SDM630_CURRENT_AVERAGE              0x002E                              //A
#define SDM630_FREQUENCY                    0x0046                              //HZ
#define SDM630_IMPORT_ACTIVE_ENERGY         0x0048                              //Wh
#define SDM630_EXPORT_ACTIVE_ENERGY         0x004A                              //Wh
#define SDM630_IMPORT_REACTIVE_ENERGY       0x004C                              //VARh
#define SDM630_EXPORT_REACTIVE_ENERGY       0x004E                              //VARh
#define SDM630_TOTAL_SYSTEM_POWER_DEMAND    0x0054                              //W
#define SDM630_MAXIMUM_TOTAL_SYSTEM_POWER   0x0056                              //W






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
    DEBUG_PRINTLN(F("ERROR: MQTT message publish failed, either connection lost, or message too large"));
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
      DEBUG_PRINTLN(settings.mqttServer);
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
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(74880);
  
  pinMode(LED1, OUTPUT);    
  pinMode(LED2, OUTPUT);   
  digitalWrite(LED1, LEDOFF);
  digitalWrite(LED2, LEDOFF);

  sprintf(MQTT_CLIENT_ID, "%06X", ESP.getChipId());
  DEBUG_PRINT(F("INFO: MQTT client ID/Hostname: "));
  DEBUG_PRINTLN(MQTT_CLIENT_ID);

  sprintf(MQTT_STATE_TOPIC, "%s/power_meter/state", MQTT_CLIENT_ID);
  DEBUG_PRINT(F("INFO: MQTT command topic: "));
  DEBUG_PRINTLN(MQTT_STATE_TOPIC);

  // load custom params
  EEPROM.begin(512);
  EEPROM.get(0, settings);
  EEPROM.end();

  WiFiManagerParameter custom_mqtt_user("mqtt-user", "MQTT User", settings.mqttUser, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_password("mqtt-password", "MQTT Password", settings.mqttPassword, STRUCT_CHAR_ARRAY_SIZE, "type = \"password\"");
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker IP", settings.mqttServer, STRUCT_CHAR_ARRAY_SIZE);
  WiFiManagerParameter custom_mqtt_port("mqtt-port", "MQTT Broker Port", settings.mqttPort, 5);

  WiFiManager wifiManager;

  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);
  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (!wifiManager.autoConnect()) { 
    ESP.reset();
    delay(1000);
  }
  if (shouldSaveConfig) {
    strcpy(settings.mqttServer,   custom_mqtt_server.getValue());
    strcpy(settings.mqttPort,     custom_mqtt_port.getValue());
    strcpy(settings.mqttUser,     custom_mqtt_user.getValue());
    strcpy(settings.mqttPassword, custom_mqtt_password.getValue());

    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  }

  // configure MQTT
  mqttClient.setServer(settings.mqttServer, atoi(settings.mqttPort));

  // connect to the MQTT broker
  reconnect();

  ArduinoOTA.setHostname(MQTT_CLIENT_ID);
  ArduinoOTA.begin();

  ticker.detach();

  mqttPublishState();
}

// the loop function runs over and over again forever
void loop() {
  ArduinoOTA.handle();

  yield();

  // keep the MQTT client connected to the broker
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();

  yield();

  if (!mqttClient.connected()){
    return;
  }

  mqttPublishState();

 
}
