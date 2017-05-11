#ifndef XMQTT_H
#define XMQTT_H

#include <Arduino.h>
#include <PubSubClient.h>        // https://github.com/knolleary/pubsubclient/releases/tag/v2.6
#include <ArduinoJson.h>         // https://github.com/bblanchon/ArduinoJson
#include <etools.h>
#include <xparam.h>
#include <xlogger.h>             // logger https://github.com/merlokk/xlogger

#define MQTT_DEBUG
#define JSON_OBJ_BUFFER_LEN 500  // json buffer length
#define BUF_RESERVE_LEN 256      // reserve for json string
#define MAX_JSON_LENGTH 1500     // maximum json length in bytes

typedef bool (*cmdCallback)(String &cmd);

class xMQTT {
public:
  xMQTT();

  void begin(const char *_hwID, const char *_topicName, xParam *_params, xLogger *_logger = NULL, bool _postAsJson = false, bool retained = false);
  void SetParamStorage(xParam *_params);
  void SetLogger(xLogger *_logger);
  void SetPostAsJson(bool _postAsJson);
  void SetTopicName(const char * topicName);
  void SetDefaultRetained(bool _retained);
  void SetProgramVersion(char * _programVersion);
  void SetHardwareID(const String &value);

  void SetCmdCallback(cmdCallback callback);

  bool Connect();
  bool Disconnect();
  bool Reconnect();
  bool Connected();

  void BeginPublish();                           // works for json push
  void PublishState(const char *topic, const char *payload);
  void PublishState(const char *topic, const String &payload);
  void PublishState(const String &topic, const String &payload);
  void Commit();                                // works for json push
  void Commit(const String &topicAdd);
  void Commit(const char *topicAdd);

  void PublishInitialState();

  void handle();

private:
  xParam *params = NULL;
  xLogger *logger = NULL;
  bool postAsJson = false;
  bool retained = false;
  int resetErrorCnt = 0;  // connect error counter
  String jsonStrBuffer = "";

  String mqttTopic;
  String hardwareID;
  const char * programVersion = NULL;

  WiFiClient wifiClient;
  PubSubClient mqttClient;

  void mqttInternalPublish(const String &topic, const String &payload);
  bool jsonInternalPublish(const String &topic, const String &payload);

  cmdCallback _cmdCallback;
  bool execCmdCallback(String &cmd);
  void mqttCallback(char* topic, uint8_t *payload, unsigned int length);

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

template<typename... Args>
void xMQTT::DEBUG_PRINTLN(Args... args) {
#ifdef MQTT_DEBUG
  if (logger) {
    logger->println(args...);
  }
#endif
}

#endif // XMQTT_H
