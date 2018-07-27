/*
 * Plantower pms5003, pms7003, pmsA003 dust sensors library.
 * Get PM1.0, PM2.5, PM10 concentration, unit: μg/m³.
 * It works via SoftwareSerial or HardwareSerial object. It needs to init it first. ESP-8266 tested.
 *
 * (c) Oleg Moiseenko 2018
 */

#ifndef PMSX003_H
#define PMSX003_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <pitimer.h>
#include <etools.h>
#include <xlogger.h>            // logger https://github.com/merlokk/xlogger

#include <xmqtt.h>

#include <pms7003.h>           // library here: https://github.com/bertrik/pms7003/tree/master/pms7003_esp

#define PMS_DEBUG

// poll
#define MILLIS_TO_POLL          10*1000       // max time to wait for poll

// timers
#define TID_POLL                0x0001        // timer UID for poll

class pmsx003 {
public:
  pmsx003();

  void begin(xLogger *_logger, Stream *_serial);
  void handle();
  void SetLogger(xLogger * _logger);
  void SetMQTT(xMQTT *_mqtt, String _topicOnline, String _topicPM1_0, String _topicPM2_5, String _topicPM10);
  uint8_t Reset();

  bool Connected();

  uint16_t GetPM1_0() const;
  uint16_t GetPM2_5() const;
  uint16_t GetPM10() const;
  pms_meas_t *GetMeasurements();

private:
  Stream *aserial = NULL;
  xLogger *logger = NULL;
  piTimer atimer;
  xMQTT *amqtt = NULL;

  bool aConnected = false;

  // string ID
  String TextIDs;

  //mqtt topics
  String atopicOnline;
  String atopicPM1_0;
  String atopicPM2_5;
  String atopicPM10;

  // decoded measurements
  pms_meas_t pms_meas;

  void SetStandbyMode(bool wakeup);
  void SetActiveMode(bool activeMode);
  void ManualMeasurement();
  void SensorInit();

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

template <typename... Args>
void pmsx003::DEBUG_PRINTLN(Args... args) {
#ifdef PMS_DEBUG
  if (logger) {
    logger->println(args...);
  }
#endif
}

#endif // PMSX003_H
