#ifndef XMQTT_H
#define XMQTT_H

#include <Arduino.h>
#include <etools.h>
#include <xlogger.h>        // logger https://github.com/merlokk/xlogger

class xMQTT {
public:
  xMQTT();

  void begin(const char * topicName, xLogger * _logger = NULL, bool _postAsJson = false, bool retained = false);
  void init(const char * topicName);
  void SetLogger(xLogger * _logger);
  void SetPostAsJson(bool _postAsJson);
  void SetTopicName(const char * topicName);

  bool Connect();
  bool Disconnect();
  bool Reconnect();

  void BeginPublish();                     // works for json push
  void PublishState(const char *topic, const char *payload);
  void PublishState(String &topic, String &payload);
  void Commit(String &topicAdd);           // works for json push

private:
  xLogger * logger = NULL;
  bool postAsJson = false;
  bool retained = false;

  void mqttCallback(char* topic, byte* payload, unsigned int length);

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

#endif // XMQTT_H
