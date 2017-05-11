#include "xmqtt.h"

xMQTT::xMQTT() {
  mqttClient.setClient(wifiClient);

  using namespace std::placeholders;
  mqttClient.setCallback(std::bind(&xMQTT::mqttCallback, this, _1, _2, _3));
}

void xMQTT::begin(const char *_hwID, const char *_topicName, xParam *_params, xLogger *_logger, bool _postAsJson, bool retained) {
  // here is the right order!!!
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
  DEBUG_PRINTLN(SF("MQTT topic: ") + mqttTopic);
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

void xMQTT::mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  String sPayload;
  BufferToString(sPayload, (char*)payload, length);

  DEBUG_PRINTLN(SF("MQTT message arrived [") + topic + SF("] ") + sPayload);

  // commands
  // execute logger commands
  if (logger && logger->ExecCommand(sPayload)) {
    return;
  }
  // execute callback commands
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

bool xMQTT::jsonInternalPublish(const String &topic, const String &payload) {
  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject *root = &jsonBuffer.parseObject(jsonStrBuffer);
  if (!root->success()){
    DEBUG_PRINTLN(llError, "xMQTT: json load error.");
    root = &jsonBuffer.createObject();
  };
  if (!root->success()) {
    DEBUG_PRINTLN(llError, F("xMQTT: json final check load error."));
    return false;
  }

  if (topic.indexOf("/") < 0) {
    root->set(topic, payload);
  } else {
    JsonObject *jobj = root;
    String str = topic;
    String sNode = "";

    if (str.indexOf("/") == 0)
      str = str.substring(1);

    while (str.indexOf("/") >= 0) {
      int indx = str.indexOf("/");
      sNode = str.substring(0, indx);
      str = str.substring(indx + 1);

      JsonObject *jNode = &(*jobj)[sNode].asObject();
      if (!jNode->success())
        jNode = &jobj->createNestedObject(sNode);

      jobj = jNode;
    }

    jobj->set(str, payload);

  }

  if (root->measureLength() > MAX_JSON_LENGTH) {
    DEBUG_PRINTLN(llError, "xMQTT: JSON too big. Can't save to memory.");
    return false;
  }


  jsonStrBuffer = "";
  root->printTo(jsonStrBuffer);
  return root->success();
}

void xMQTT::BeginPublish() {
  if (!postAsJson)
    return;

  jsonStrBuffer.reserve(BUF_RESERVE_LEN);
  jsonStrBuffer = "{}";
}

void xMQTT::PublishState(const char *topic, const char *payload) {
  PublishState(String(topic), String(payload));
}

void xMQTT::PublishState(const char *topic, const String &payload) {
  PublishState(String(topic), payload);
}

void xMQTT::PublishState(const String &topic, const String &payload) {
  if (postAsJson) {
    jsonInternalPublish(topic, payload);
  } else {
    mqttInternalPublish(topic, payload);
  }
}

void xMQTT::Commit(const String &topicAdd) {
  if (!postAsJson)
    return;

  mqttInternalPublish(topicAdd, jsonStrBuffer);
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

