#include "xmqtt.h"

xMQTT::xMQTT() {

}

void xMQTT::begin(const char *_topicName, xParam *_params, xLogger *_logger, bool _postAsJson, bool retained) {
  SetTopicName(_topicName);
  SetParamStorage(_params);
  SetLogger(_logger);
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
  mqttClient.setCallback(mqttCallback);

  // configure mqtt topic
  String mqttPath = params[F("mqtt_path")];
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

void xMQTT::SetCmdCallback(cmdCallback) {
  _cmdCallback = cmdCallback;
}

void xMQTT::execCmdCallback(String &cmd) {
  if (_cmdCallback) {
    _cmdCallback(cmd);
  }
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

  uint8_t i = 0;
  String srv = params[F("mqtt_server")];                                                                      // extend visiblity of the "mqtt_server" parameter
  mqttClient.setServer(srv.c_str(), params[F("mqtt_port")].toInt());
  while (!mqttClient.connected()) {
    if (mqttClient.connect(hardwareID.c_str(), params[F("mqtt_user")].c_str(), params[F("mqtt_passwd")].c_str())) {  // because connect doing here
      DEBUG_PRINTLN(F("The client is successfully connected to the MQTT broker"));

      // subscribe to control topic
      String vtopic = mqttTopic + SF("control");
      mqttClient.subscribe(vtopic.c_str());
      resetErrorCnt = 0;
    } else {
      DEBUG_PRINTLN(llError, SF("The connection to the MQTT broker failed. User: ") + params[F("mqtt_user")] +
        SF(" Passwd: ") + params[F("mqtt_passwd")] +  SF(" Broker: ") + srv + SF(":") + params[F("mqtt_port")]);

      delay(1000);

      if (i >= 2) {
        break;
      }
      i++;

      if (resetErrorCnt >= 50) {
        restart();
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

void xMQTT::BeginPublish() {
  if (!postAsJson)
    return;

  jsonBuffer.reserve(BUF_RESERVE_LEN);
  jsonBuffer = "{";
}

void xMQTT::PublishState(const char *topic, const char *payload) {
  PublishState(String(topic), String(payload))
}

void xMQTT::PublishState(String &topic, String &payload) {
  if (postAsJson) {
    jsonBuffer += "\"" + topic + "\":\"" + payload + "\",";
  } else {
    String vtopic = mqttTopic + topic;
    if (mqttClient.publish(vtopic.c_str(), payload.c_str(), retained)) {
      DEBUG_PRINTLN(SF("MQTT publish ok. Topic: ") + vtopic + SF(" Payload: ") + payload);
    } else {
      DEBUG_PRINTLN(llError, F("MQTT message publish failed"));
    }
  }

}

void xMQTT::Commit(String &topicAdd) {
  if (!postAsJson)
    return;
  if (jsonBuffer[jsonBuffer.length() - 1] = ',')
    jsonBuffer.remove(jsonBuffer.length() - 1);
  jsonBuffer += "}";

  String vtopic = mqttTopic + topicAdd;
  if (mqttClient.publish(vtopic.c_str(), jsonBuffer.c_str(), retained)) {
    DEBUG_PRINTLN(SF("MQTT publish ok. Topic: ") + vtopic + SF(" Payload: ") + jsonBuffer);
  } else {
    DEBUG_PRINTLN(llError, F("MQTT message publish failed"));
  }
}
