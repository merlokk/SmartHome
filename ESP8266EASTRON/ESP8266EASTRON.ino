/*
 * Eastron energy counter bridge. 
 * SDM 230, SDM 630
 * Eastron -> RS-485 -> esp-8266 -> WiFi -> MQTT
 * 
 * original repository here https://github.com/merlokk/SmartHome/tree/master/ESP8266EASTRON
 * (c) Oleg Moiseenko 2017
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>        // https://github.com/esp8266/Arduino
#include <DNSServer.h>          // Local DNS Server used for redirecting all requests to the configuration portal
#include <WiFiManager.h>        // https://github.com/merlokk/WiFiManager original:https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>       // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <Ticker.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time 
#include <NtpClientLib.h>       // https://github.com/gmag11/NtpClient
#include <xlogger.h>            // logger https://github.com/merlokk/xlogger
// my libraries
#include <etools.h>
#include <pitimer.h>     // timers
#include <eastron.h>
#include <xparam.h>

#define               PROGRAM_VERSION   "0.95"

#define               DEBUG                            // enable debugging
#define               DEBUG_SERIAL      logger

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

// vcc measurements
ADC_MODE(ADC_VCC);                            // set ADC to meassure esp8266 VCC

// poll
#define MILLIS_TO_POLL          15*1000       //max time to wait for poll input registers (regular poll)
#define MILLIS_TO_POLL_HOLD_REG 15*60*1000    //max time to wait for poll all
// timers
#define TID_POLL                0x0001        // timer UID for regular poll 
#define TID_HOLD_REG            0x0002        // timer UID for poll all the registers

// LEDs and pins
#define PIN_PGM  0      // programming pin and jumper
#define LED1     12     // led 1
#define LED2     14     // led 2
#define LEDON    LOW
#define LEDOFF   HIGH

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
Eastron          eastron;
mqttMapConfigS   *eastronCfg = NULL;
int              eastronCfgLength = 0;
piTimer          ptimer;

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////

/*
  Function called to publish the state of 
*/
void mqttPublishInitialState() {
  String str = WiFi.macAddress();
  mqttPublishState("MAC", str.c_str());
  mqttPublishState("HardwareId", HARDWARE_ID);  
  mqttPublishState("Version", PROGRAM_VERSION);  
  mqttPublishState("DeviceType", params[F("device_type")].c_str());  

  mqttPublishRegularState();
}

void mqttPublishRegularState() {
  String s = "";
  eTimeToStr(s, millis() / 1000);
  mqttPublishState("Uptime", s.c_str()); 
  
  s = String(ESP.getVcc());
  mqttPublishState("VCC", s.c_str()); 
  
  s = String(WiFi.RSSI());
  mqttPublishState("RSSI", s.c_str()); 

  if (timeStatus() == timeSet){
    s = NTP.getTimeDateString();
    mqttPublishState("LastSeenDateTime", s.c_str()); 
  
    s = NTP.getTimeDateString(NTP.getFirstSync());
    mqttPublishState("LastBootDateTime", s.c_str()); 

    if (eastron.Connected) {
      s = NTP.getTimeDateString();
      mqttPublishState("LastConnectedDateTime", s.c_str()); 
    }
  }

  mqttPublishState("Connected", eastron.Connected ? MQTT_ON_PAYLOAD:MQTT_OFF_PAYLOAD);
}

void mqttPublishState(const char *topic, const char *payload) {
  mqttPublishState(topic, payload, false);                             // true - for release!!!!
}

void mqttPublishState(const char *topic, const char *payload, bool retained) {
  String vtopic = String(MQTT_STATE_TOPIC) + String(topic);
  if (mqttClient.publish(vtopic.c_str(), payload, retained)) {
    DEBUG_PRINT(F("MQTT publish ok. Topic: "));
    DEBUG_PRINT(vtopic);
    DEBUG_PRINT(F(" Payload: "));
    DEBUG_PRINTLN(payload);
  } else {
    DEBUG_EPRINTLN(F("MQTT message publish failed"));
  }
}

/*
  Function called to connect/reconnect to the MQTT broker
*/
void reconnect() {
  uint8_t i = 0;
  String srv = params[F("mqtt_server")];                                                                      // extend visiblity of the "mqtt_server" parameter
  mqttClient.setServer(srv.c_str(), params[F("mqtt_port")].toInt());
  while (!mqttClient.connected()) {
    if (mqttClient.connect(HARDWARE_ID, params[F("mqtt_user")].c_str(), params[F("mqtt_passwd")].c_str())) {  // because connect doing here
      DEBUG_PRINTLN(F("The client is successfully connected to the MQTT broker"));

      // subscribe to control topic
      String vtopic = String(MQTT_STATE_TOPIC) + SF("control");
      mqttClient.subscribe(vtopic.c_str());
    } else {
      DEBUG_EPRINT(F("The connection to the MQTT broker failed."));
      DEBUG_EPRINT(F(" Username: "));
      DEBUG_EPRINT(params[F("mqtt_user")]);
      DEBUG_EPRINT(F(" Password: "));
      DEBUG_EPRINT(params[F("mqtt_passwd")]);
      DEBUG_EPRINT(F(" Broker: "));
      DEBUG_EPRINT(srv);
      DEBUG_EPRINT(F(":"));
      DEBUG_EPRINTLN(params[F("mqtt_port")]);
      delay(1000);
      if (i == 3) {
        restart(); 
      }
      i++;
    }
  }
}

/*
  Callback for receive message from MQTT broker
*/
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String sPayload;
  BufferToString(sPayload, (char*)payload, length);

  DEBUG_PRINT(F("MQTT message arrived ["));
  DEBUG_PRINT(topic);
  DEBUG_PRINTLN(SF("] ") + sPayload);

  // commands
  CmdCallback(sPayload); 
}

///////////////////////////////////////////////////////////////////////////
//   NTP
///////////////////////////////////////////////////////////////////////////
boolean syncEventTriggered = false;   // True if a time even has been triggered
NTPSyncEvent_t ntpEvent;              // Last triggered event

void processSyncEvent(NTPSyncEvent_t ntpEvent) {
  if (ntpEvent) {
    DEBUG_EPRINT(F("NTP sync error: "));
    switch (noResponse) {
      case noResponse: DEBUG_EPRINTLN(F("NTP server not reachable")); break;
      case invalidAddress: DEBUG_EPRINTLN(F("Invalid NTP server address")); break;
      default: DEBUG_EPRINTLN(F("")); 
    }
  } else {
    DEBUG_PRINT(F("Got NTP time: "));
    DEBUG_PRINTLN(NTP.getTimeDateString(NTP.getLastNTPSync()));
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
  params.LoadFromEEPROM();

  WiFiManager wifiManager(DEBUG_SERIAL);
  wifiManager.mainProgramVersion = PROGRAM_VERSION;

  WiFiManagerParameter custom_mqtt_text("<br/>MQTT config: <br/>");
  wifiManager.addParameter(&custom_mqtt_text);
  WiFiManagerParameter custom_mqtt_user("mqtt-user", "MQTT User", params[F("mqtt_user")], 20);
  wifiManager.addParameter(&custom_mqtt_user);
  WiFiManagerParameter custom_mqtt_password("mqtt-password", "MQTT Password", params[F("mqtt_passwd")], 20, "type = \"password\"");
  wifiManager.addParameter(&custom_mqtt_password);
  WiFiManagerParameter custom_mqtt_server("mqtt-server", "MQTT Broker Address", params[F("mqtt_server")], 20);
  wifiManager.addParameter(&custom_mqtt_server);
  WiFiManagerParameter custom_mqtt_port("mqtt-port", "MQTT Broker Port", params[F("mqtt_port")], 20);
  wifiManager.addParameter(&custom_mqtt_port);
  WiFiManagerParameter custom_mqtt_path("mqtt-path", "MQTT Path", params[F("mqtt_path")], 20);
  wifiManager.addParameter(&custom_mqtt_path);
  WiFiManagerParameter custom_device_type("device-type", "Device type", params[F("device_type")], 20);
  wifiManager.addParameter(&custom_device_type);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);
  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  if (withAutoConnect){
    if (!wifiManager.autoConnect()) { 
      restart();
    }
  } else {
    String ssid = SF("ESP") + String(ESP.getChipId());
    if (!wifiManager.startConfigPortal(ssid.c_str())) { 
      if (shouldSaveConfig) 
        DEBUG_PRINTLN("shouldSaveConfig");      
      restart();
    }
  }
  if (shouldSaveConfig) {
    params.SetParam(F("mqtt_server"), custom_mqtt_server.getValue());
    params.SetParam(F("mqtt_port"), custom_mqtt_port.getValue());
    params.SetParam(F("mqtt_user"), custom_mqtt_user.getValue());
    params.SetParam(F("mqtt_passwd"), custom_mqtt_password.getValue());
    params.SetParam(F("mqtt_path"), custom_mqtt_path.getValue());
    params.SetParam(F("device_type"), custom_device_type.getValue());

    params.SaveToEEPROM();
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
  DEBUG_PRINTLN(F("Restart..."));
  ESP.reset();
  delay(1000);
}

/*
  Function called to reset the configuration of the switch
  Warning! It clears wifi connection parameters!
*/
void reset() {
  DEBUG_PRINTLN(F("Reset..."));
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
    DEBUG_EPRINT(F("OTA error: "));
    DEBUG_EPRINTLN(error);
    switch (error) {
      case OTA_AUTH_ERROR:    DEBUG_EPRINTLN(F("OTA: Auth Failed")); break;
      case OTA_BEGIN_ERROR:   DEBUG_EPRINTLN(F("OTA: Begin Failed")); break;
      case OTA_CONNECT_ERROR: DEBUG_EPRINTLN(F("OTA: Connect Failed")); break;
      case OTA_RECEIVE_ERROR: DEBUG_EPRINTLN(F("OTA: Receive Failed")); break;
      case OTA_END_ERROR:     DEBUG_EPRINTLN(F("OTA: End Failed")); break;
    }

    ESP.restart();
  });
  
  ArduinoOTA.onStart([]() {
    DEBUG_PRINTLN(F("Start OTA"));
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_PRINTLN(F("End OTA"));
  });
  
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);
  // No authentication by default
  // ArduinoOTA.setPassword("admin123");  
  ArduinoOTA.setHostname(HARDWARE_ID);
  ArduinoOTA.begin();
}

///////////////////////////////////////////////////////////////////////////
//  execute commands
///////////////////////////////////////////////////////////////////////////
const char* strCommandsDesc = 
  "Command reboot reboots ESP.\r\n"\
  "Command startwificfg puts ESP to configure mode. Show configuration AP.\r\n"\
  "Command set <param_name> <value> writes parameter to ESP memory. \r\n"\
  "parameters: mqtt_server, mqtt_port, mqtt_user, mqtt_passwd, mqtt_path, device_type";
  
bool CmdCallback(String &cmd) {
  if (cmd == "reboot") {
    DEBUG_PRINTLN(F("COMMAND: reboot. Rebooting..."));
    restart();
    delay(200);
    return true;
  }

  if (cmd == "startwificfg") {
    DEBUG_PRINTLN(F("COMMAND: start wifi config."));
    wifiSetup(false);
    restart();
    delay(200);
    return true;
  }

  // set <param> <value>. sample: set device_type 220
  if (cmd.length() > 7 && cmd.startsWith("set ")) {
    String name = cmd.substring(4);
    int indx = name.indexOf(" ");
    String value = name.substring(indx + 1);
    name = name.substring(0, indx);
    DEBUG_PRINTLN(SF("COMMAND: config <") + name + SF("> <") + value + SF(">"));

    if (params.ParamExists(name)) {
      params.LoadFromEEPROM();
      params.SetParam(name, value);
      params.SaveToEEPROM();
    }
    else {
      DEBUG_EPRINTLN(SF("Command error. Parameter <") + name + SF("> not found"));
    }
    return true;
  }

  return false;
}

///////////////////////////////////////////////////////////////////////////
//   Setup() and loop()
///////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.setDebugOutput(false);
  Serial1.setDebugOutput(true);
  Serial.begin(115200); //74880
  Serial1.begin(115200); 

  // client ID
  sprintf(HARDWARE_ID, "%06X", ESP.getChipId());
  // start logger
  logger.cmdCallback(CmdCallback, strCommandsDesc);
  logger.begin(HARDWARE_ID, &Serial1, true);
  logger.setProgramVersion(PROGRAM_VERSION);
  logger.setTimeFormat(ltNone);

  //timer
  ptimer.Add(TID_POLL, MILLIS_TO_POLL);
  ptimer.Add(TID_HOLD_REG, MILLIS_TO_POLL_HOLD_REG);

  // for debug
  delay(200);
  DEBUG_PRINTLN(F(""));
  DEBUG_PRINTLN(F("Starting..."));

  DEBUG_PRINT(F("ResetReason: "));
  DEBUG_PRINTLN(ESP.getResetReason());

  DEBUG_PRINT(F("Hardware ID/Hostname: "));
  DEBUG_PRINTLN(HARDWARE_ID);

  DEBUG_PRINT(F("MAC address: "));
  DEBUG_PRINTLN(WiFi.macAddress());

  DEBUG_PRINT("Module VCC: ");
  DEBUG_PRINTLN(ESP.getVcc());

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
    DEBUG_WPRINTLN(F("Programming mode active!"));
  }

  // setup xparam lib
  params.SetLogger(&logger);
  params.begin();
  params.LoadFromEEPROM();
  
  // wifi setup with autoconnect
  wifiSetup(true);

  // pause for connecting
  if (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  // NTP config
  NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
    ntpEvent = event;
    syncEventTriggered = true;
  });
  NTP.begin("ua.pool.ntp.org", 0, false);
  NTP.setInterval(30 * 60); // twice a hour

  // configure mqtt topic
  String mqttPath = params[F("mqtt_path")];
  if (mqttPath.length() == 0) {
    sprintf(MQTT_STATE_TOPIC, "%s/PowerMeter/", HARDWARE_ID);
  } else {
    if (mqttPath[mqttPath.length() - 1] == '/') {
      sprintf(MQTT_STATE_TOPIC, "%s", mqttPath.c_str());
    } else {
      sprintf(MQTT_STATE_TOPIC, "%s/", mqttPath.c_str());
    }
  }
  DEBUG_PRINT(F("MQTT command topic: "));
  DEBUG_PRINTLN(MQTT_STATE_TOPIC);

  // configure MQTT
  mqttClient.setCallback(mqttCallback);

  // connect to the MQTT broker
  reconnect();

  // ArduinoOTA
  setupArduinoOTA();

  ticker.detach();
  ticker.attach(0.1, tick);

  mqttPublishInitialState();

  // eastron setup
  eastron.SetLogger(&logger);
  DEBUG_PRINT(F("DeviceType: "));
  DEBUG_PRINTLN(params[F("device_type")]);
  eastron.ModbusSetup(params[F("device_type")].c_str());

  String str;
  eastron.getStrModbusConfig(str);
  DEBUG_PRINTLN(F("Modbus config:"));
  DEBUG_PRINTLN(str);

  // set password in work mode
  logger.setPassword("123454321");

  ticker.detach();
  digitalWrite(LED1, LEDOFF);
}

// the loop function runs over and over again forever
void loop() {
  // Programming pin activates setup
  if (!inProgrammingMode && (!digitalRead(PIN_PGM))) {
    DEBUG_WPRINTLN(F("Wifi setup activated"));

    wifiSetup(false);
    delay(1000);
  }
  
  digitalWrite(LED2, LEDON);
  ArduinoOTA.handle();
  logger.handle();
  digitalWrite(LED2, LEDOFF);

  yield();

  // NTP
  if (syncEventTriggered) {
    processSyncEvent(ntpEvent);
    syncEventTriggered = false;
  }

  // keep the MQTT client connected to the broker
  if (!mqttClient.connected()) {
    digitalWrite(LED2, LEDON);
    reconnect();
    digitalWrite(LED2, LEDOFF);
  }
  mqttClient.loop();

  // check wifi connection
  if (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  yield();

  if (!mqttClient.connected()){
    digitalWrite(LED2, LEDOFF);
    return;
  }

  if (ptimer.isArmed(TID_POLL)) {
    digitalWrite(LED2, LEDON);

    // modbus poll function
    if (ptimer.isArmed(TID_HOLD_REG)) {
      eastron.Poll(POLL_ALL);
      ptimer.Reset(TID_HOLD_REG);
    } else {
      eastron.Poll(POLL_INPUT_REGISTERS);
    };

    yield();

    // publish some system vars
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
        mqttPublishState(eastron.mapConfig[i].mqttTopicName, str.c_str());        
      }    
    };
  
    ptimer.Reset(TID_POLL);
    digitalWrite(LED2, LEDOFF);
  }
  
  digitalWrite(LED2, LEDOFF);
  delay(100);
}


