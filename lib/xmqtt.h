#ifndef XMQTT_H
#define XMQTT_H

#include <Arduino.h>
#include <etools.h>
#include <xparam.h>
#include <xlogger.h>        // logger https://github.com/merlokk/xlogger

#define MQTT_DEBUG
#define BUF_RESERVE_LEN 256 // reserve for json string

typedef void (*cmdCallback)(String &cmd);

class xMQTT {
public:
  xMQTT();

  void begin(const char *_topicName, xParam *_params, xLogger *_logger = NULL, bool _postAsJson = false, bool retained = false);
  void SetParamStorage(xParam *_params);
  void SetLogger(xLogger *_logger);
  void SetPostAsJson(bool _postAsJson);
  void SetTopicName(const char * topicName);
  void SetDefaultRetained(bool _retained);

  void SetCmdCallback(cmdCallback);

  bool Connect();
  bool Disconnect();
  bool Reconnect();

  void BeginPublish();                     // works for json push
  void PublishState(const char *topic, const char *payload);
  void PublishState(String &topic, String &payload);
  void Commit(String &topicAdd);           // works for json push

private:
  xParam * params = NULL;
  xLogger * logger = NULL;
  bool postAsJson = false;
  bool retained = false;
  int resetErrorCnt = 0;  // connect error counter
  String jsonBuffer = "";

  String mqttTopic;
  String hardwareID;

  cmdCallback _cmdCallback;
  void execCmdCallback(String &cmd);

  void mqttCallback(char* topic, byte* payload, unsigned int length);

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
