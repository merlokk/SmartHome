/*
 * 
 * General ESP8266 library.
 * 
 * MQTT, NTP, ArduinoOTA, WifiNabager, ExecuteCommands, xLogger
 * 
 * (c) Oleg Moiseenko 2017
 */

// vcc measurements
ADC_MODE(ADC_VCC);                            // set ADC to meassure esp8266 VCC

///////////////////////////////////////////////////////////////////////////
//   MQTT
///////////////////////////////////////////////////////////////////////////

/*
  Function called to init MQTT
*/
void initMQTT(const char * topicName) {
  mqtt.begin(HARDWARE_ID, topicName, &params, &logger, true, false);  // hardwareID, topicName, xParam, xLogger, postAsJson, retained
  mqtt.SetProgramVersion(PROGRAM_VERSION);
  mqtt.SetCmdCallback(CmdCallback);
  
  mqtt.Connect();
  mqtt.PublishInitialState();
}

void mqttPublishRegularState() {
  String s = "";
  eTimeToStr(s, millis() / 1000);
  mqtt.PublishState(SF("Uptime"), s); 
  mqtt.PublishState(SF("VCC"), String(ESP.getVcc())); 
  mqtt.PublishState(SF("RSSI"), String(WiFi.RSSI())); 

  if (timeStatus() == timeSet){
    mqtt.PublishState(SF("LastSeenDateTime"), NTP.getTimeDateString()); 
    mqtt.PublishState(SF("LastBootDateTime"), NTP.getTimeDateString(NTP.getFirstSync())); 

#ifdef MODBUS_OBJ_NAME
    if (MODBUS_OBJ_NAME.Connected) {
      mqtt.PublishState(SF("LastConnectedDateTime"), NTP.getTimeDateString()); 
    }
#endif
  }

#ifdef MODBUS_OBJ_NAME
  mqtt.PublishState(SF("Connected"), MODBUS_OBJ_NAME.Connected ? MQTT_ON_PAYLOAD:MQTT_OFF_PAYLOAD);
#endif
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
  WiFiManagerParameter custom_mqtt_text1("<br/>Development config: <br/>");
  wifiManager.addParameter(&custom_mqtt_text1);
  WiFiManagerParameter custom_passwd("device-paswd", "Device dev password", params[F("device_passwd")], 20);
  wifiManager.addParameter(&custom_passwd);

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(180);
  // set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  // resets after 
  wifiManager.setBreakAfterConfig(true);

  bool needsRestart = false;
  if (withAutoConnect){
    if (!wifiManager.autoConnect()) { 
      needsRestart = true;
    }
  } else {
    String ssid = SF("ESP") + String(ESP.getChipId());
    if (!wifiManager.startConfigPortal(ssid.c_str())) { 
      needsRestart = true;
    }
  }
  if (shouldSaveConfig) {
    params.SetParam(F("mqtt_server"), custom_mqtt_server.getValue());
    params.SetParam(F("mqtt_port"), custom_mqtt_port.getValue());
    params.SetParam(F("mqtt_user"), custom_mqtt_user.getValue());
    params.SetParam(F("mqtt_passwd"), custom_mqtt_password.getValue());
    params.SetParam(F("mqtt_path"), custom_mqtt_path.getValue());
    params.SetParam(F("device_type"), custom_device_type.getValue());
    params.SetParam(F("device_passwd"), custom_passwd.getValue());

    params.SaveToEEPROM();
  }

  if (needsRestart) {
    restart();
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

void initPrintStartDebugInfo() {
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
}

///////////////////////////////////////////////////////////////////////////
//   Update from WEB
///////////////////////////////////////////////////////////////////////////

void UpdateFromWeb(const String &server) {
  t_httpUpdate_return ret = ESPhttpUpdate.update(server, 80, "/update/arduino.php", PROGRAM_VERSION);
  switch(ret) {
  case HTTP_UPDATE_FAILED:
    DEBUG_EPRINTLN(F("[update] Update failed."));
    break;
  case HTTP_UPDATE_NO_UPDATES:
    DEBUG_PRINTLN(F("[update] Update no Update."));
    break;
  case HTTP_UPDATE_OK:
    DEBUG_PRINTLN(F("[update] Update ok.")); // may not called we reboot the ESP
    break;
  }
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
  if (params[F("device_passwd")].length() > 0)
    ArduinoOTA.setPassword(params[F("device_passwd")].c_str());    
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
  "parameters: mqtt_server, mqtt_port, mqtt_user, mqtt_passwd, mqtt_path, device_type.\r\n"\
  "System commands: resetcfg, webupdate, webupdatec. ";
  
bool CmdCallback(String &cmd) {
  if (cmd == "reboot") {
    DEBUG_PRINTLN(F("COMMAND: reboot. Rebooting..."));
    restart();
    delay(200);
    return true;
  }

  if (cmd == "resetcfg") {
    DEBUG_PRINTLN(F("COMMAND: Reset wifi config..."));
    reset();
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

  if (cmd.length() >= 15 && cmd.startsWith("webupdate ")) {
    String srv = cmd.substring(10);
    DEBUG_PRINTLN(SF("COMMAND: web update. server=") + srv);
    UpdateFromWeb(srv);
    return true;
  }

  if (cmd.length() >= 16 && cmd.startsWith("webupdatec ")) {
    String srv = cmd.substring(10);
    DEBUG_PRINTLN(SF("COMMAND: web update. server=") + srv);
    params.LoadFromWeb(srv + "/update/arduinocfg.php");
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
//  logger and xParam logic
///////////////////////////////////////////////////////////////////////////

void initLogger() {
  logger.cmdCallback(CmdCallback, strCommandsDesc);
  logger.begin(HARDWARE_ID, &Serial1, true);
  logger.setProgramVersion(PROGRAM_VERSION);
  logger.setTimeFormat(ltNone);
}

void initXParam() {
  params.SetLogger(&logger);
  params.begin();
  params.LoadFromEEPROM();
}


///////////////////////////////////////////////////////////////////////////
//   generalSetup() and generalloop()
///////////////////////////////////////////////////////////////////////////
void generalSetup() {
  // start
  ticker.attach(0.1, tick);

  // client ID
  sprintf(HARDWARE_ID, "%06X", ESP.getChipId());
  // start logger
  initLogger();
  
  // for debug
  delay(200);
  initPrintStartDebugInfo();

  WiFi.hostname(HARDWARE_ID);

  // 0 = programming mode
  pinMode(PIN_PGM, INPUT);   
  inProgrammingMode = !digitalRead(PIN_PGM);
  if (inProgrammingMode) {
    DEBUG_WPRINTLN(F("Programming mode active!"));
  }

  // setup xparam lib
  initXParam();
  
  // wifi setup with autoconnect
  wifiSetup(true);

  // pause for connecting
  // By default, ESP will attempt to reconnect to Wi-Fi network whenever it is disconnected. There is no need to handle this by separate code. ?????  TODO: check
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_WPRINTLN(F("Wifi is not connected. Trying to reconnect..."));
    WiFi.begin();
    delay(5000);
  }

  // NTP config
  NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
    ntpEvent = event;
    syncEventTriggered = true;
  });
  NTP.begin("ua.pool.ntp.org", 0, false);
  NTP.setInterval(30 * 60); // twice a hour
  
  ticker.detach();
  ticker.attach(0.1, tick);

  // configure MQTT
  initMQTT("PowerMeter");
  
  // ArduinoOTA
  setupArduinoOTA();
  
}

bool generalLoop() {

  // Programming pin activates setup
  if (!inProgrammingMode && (!digitalRead(PIN_PGM))) {
    DEBUG_WPRINTLN(F("Wifi setup activated"));

    wifiSetup(false);
    delay(1000);
    return false;
  }

  // check wifi connection
  if (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  ArduinoOTA.handle();
  logger.handle();

  yield();

  // NTP
  if (syncEventTriggered) {
    processSyncEvent(ntpEvent);
    syncEventTriggered = false;
  }

  yield();

  // MQTT
  mqtt.handle();
  if (!mqtt.Connected()){
    return false;
  }

  return true;
}


