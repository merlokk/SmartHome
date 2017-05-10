#include "xmqtt.h"

xMQTT::xMQTT() {
  mqttClient.setClient(wifiClient);
}

void xMQTT::begin(const char *_hwID, const char *_topicName, xParam *_params, xLogger *_logger, bool _postAsJson, bool retained) {
  // here right order!!!
  SetHardwareID(String(_hwID));
  SetLogger(_logger);
  SetParamStorage(_params);
  SetTopicName(_topicName);
  SetPostAsJson(_postAsJson);
}

void xMQTT::SetParamStorage(xParam *_params) {
  params = _params;
}

void xMQTT::SetLogger(xLogger * _logger) {
  logger = _logger;
}

void xMQTT::SetPostAsJson(bool _postAsJson) {
  postAsJson = _postAsJson;
}

void xMQTT::SetTopicName(const char *defaultTopicName) {
  if (!params)
    return;

  String mqttPath = (*params)[F("mqtt_path")];
  if (mqttPath.length() == 0) {
    mqttTopic = hardwareID + "/" + defaultTopicName + "/";
  } else {
    if (mqttPath[mqttPath.length() - 1] == '/') {
      mqttTopic = mqttPath;
    } else {
      mqttTopic = mqttPath + "/";
    }
  }
  DEBUG_PRINTLN(SF("MQTT command topic: ") + mqttTopic);
}

void xMQTT::SetDefaultRetained(bool _retained) {
  retained = _retained;
}

void xMQTT::SetProgramVersion(char *_programVersion){
  programVersion = _programVersion;
}

void xMQTT::SetHardwareID(const String &value){
  hardwareID = value;
}

void xMQTT::SetCmdCallback(cmdCallback callback) {
  _cmdCallback = callback;
}

bool xMQTT::execCmdCallback(String &cmd) {
  if (_cmdCallback) {
    return _cmdCallback(cmd);
  }
  return false;
}

void xMQTT::mqttCallback(char* topic, byte* payload, unsigned int length) {
  String sPayload;
  BufferToString(sPayload, (char*)payload, length);

  DEBUG_PRINTLN(SF("MQTT message arrived [") + topic + SF("] ") + sPayload);

  // commands
  execCmdCallback(sPayload);
}


bool xMQTT::Connect() {
  if (mqttClient.connected()) {
    return true;
  }
  if (!params) {
    return false;
  }

  auto &lparams = *params;
  uint8_t i = 0;
  String srv = lparams[F("mqtt_server")];                                                                             // extend visiblity of the "mqtt_server" parameter
  mqttClient.setServer(srv.c_str(), lparams[F("mqtt_port")].toInt());
  while (!mqttClient.connected()) {
    if (mqttClient.connect(hardwareID.c_str(), lparams[F("mqtt_user")].c_str(), lparams[F("mqtt_passwd")].c_str())) {  // because connect doing here
      DEBUG_PRINTLN(F("The client is successfully connected to the MQTT broker"));

      // subscribe to control topic
      String vtopic = mqttTopic + SF("control");
      mqttClient.subscribe(vtopic.c_str());
      resetErrorCnt = 0;
    } else {
      DEBUG_PRINTLN(llError, SF("The connection to the MQTT broker failed. User: ") + lparams[F("mqtt_user")] +
        SF(" Passwd: ") + lparams[F("mqtt_passwd")] +  SF(" Broker: ") + srv + SF(":") + lparams[F("mqtt_port")]);

      delay(1000);

      if (i >= 2) {
        break;
      }
      i++;

      if (resetErrorCnt >= 50) {
        DEBUG_PRINTLN(F("ESP reset..."));
        ESP.reset();
        delay(1000);
      }
      resetErrorCnt++;
    }
  }
}

bool xMQTT::Disconnect() {
  if (!mqttClient.connected()) {
    return true;
  }
  mqttClient.disconnect();
}

bool xMQTT::Reconnect() {
  if (Disconnect())
    return Connect();
  return false;
}

bool xMQTT::Connected(){
  return mqttClient.connected();
}

void xMQTT::mqttInternalPublish(const String &topic, const String &payload) {
  if (!mqttClient.connected()) {
    return;
  }

  String vtopic = mqttTopic + topic;
  if (mqttClient.publish(vtopic.c_str(), payload.c_str(), retained)) {
    DEBUG_PRINTLN(SF("MQTT publish ok. Topic: ") + vtopic + SF(" Payload: ") + payload);
  } else {
    DEBUG_PRINTLN(llError, F("MQTT message publish failed"));
  }
}

void xMQTT::BeginPublish() {
  if (!postAsJson)
    return;

  jsonBuffer.reserve(BUF_RESERVE_LEN);
  jsonBuffer = "{";
}

void xMQTT::PublishState(const char *topic, const char *payload) {
  PublishState(String(topic), String(payload));
}

void xMQTT::PublishState(const char *topic, const String &payload) {
  PublishState(String(topic), payload);
}

void xMQTT::PublishState(const String &topic, const String &payload) {
  if (postAsJson) {
    jsonBuffer += "\"" + topic + "\":\"" + payload + "\",";
  } else {
    mqttInternalPublish(topic, payload);
  }
}

void xMQTT::Commit(const String &topicAdd) {
  if (!postAsJson)
    return;

  if (jsonBuffer[jsonBuffer.length() - 1] = ',')
    jsonBuffer.remove(jsonBuffer.length() - 1);
  jsonBuffer += "}";

  mqttInternalPublish(topicAdd, jsonBuffer);
}

void xMQTT::Commit(const char *topicAdd) {
  Commit(String(topicAdd));
}

void xMQTT::Commit() {
  Commit("");
}

void xMQTT::PublishInitialState() {
  BeginPublish();
  PublishState(SF("MAC"), WiFi.macAddress());
  PublishState(SF("HardwareId"), hardwareID);
  PublishState(SF("Version"), programVersion);
  PublishState(SF("DeviceType"), (*params)[F("device_type")]);
  Commit(SF("System"));
}

void xMQTT::handle() {
  if (!mqttClient.connected()) {
    Connect();
  }

  mqttClient.loop();
}

