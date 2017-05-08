#ifndef XMQTT_H
#define XMQTT_H

#include <Arduino.h>
#include <etools.h>
#include <xlogger.h>        // logger https://github.com/merlokk/xlogger

class xMQTT {
public:
  xMQTT();

  void begin();
  void initMQTT(const char * topicName);
  void SetLogger(xLogger * _logger);
  void SetPostJson();

  bool Connect();
  bool Reconnect;

  void mqttPublishState(const char *topic, const char *payload, bool retained);
  void Commit();

private:
  xLogger * logger = NULL;

  void mqttCallback(char* topic, byte* payload, unsigned int length);

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

#endif // XMQTT_H
