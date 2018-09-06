/*
 * BME280 library.
 * Get temperature and humidity.
 * It works via I2C Wire object. It needs to init it first. ESP-8266 tested.
 *
 * (c) Oleg Moiseenko 2018
 */

#ifndef LIBSI1145_H
#define LIBSI1145_H

/*  Ideas:
  https://github.com/HGrabas/SI1145/blob/master/SI1145.cpp
  https://github.com/ArticCynda/OpenObservatory/blob/master/firmware-v3.2/si1145/SI114X.cpp
  https://github.com/torvalds/linux/blob/master/drivers/iio/light/si1145.c
*/

#include <Arduino.h>
#include <pitimer.h>
#include <etools.h>
#include <xlogger.h>            // logger https://github.com/merlokk/xlogger

#include <xmqtt.h>

#include <Wire.h>
#include <Adafruit_SI1145.h>    // original: https://github.com/adafruit/Adafruit_SI1145_Library
                                // with error handling fix: https://github.com/merlokk/Adafruit_SI1145_Library

#define SI_DEBUG

// poll
#define MILLIS_TO_POLL          10*1000       // max time to wait for poll

// timers
#define TID_POLL                0x0001        // timer UID for poll

class si1145 {
public:
  si1145();

  void begin(xLogger *_logger);
  void handle();
  void SetLogger(xLogger * _logger);
  void SetMQTT(xMQTT *_mqtt, String _topicOnline, String _topicVisible, String _topicIR, String _topicUV);
  uint8_t Reset();

  bool Connected();

  String GetTextIDs() const;
  float GetVisible() const;
  float GetIR() const;
  float GetUV() const;

  uint16_t calcOptimalGainFromSignal(int signal);
private:
  xLogger *logger = NULL;
  piTimer atimer;
  xMQTT *amqtt = NULL;

  Adafruit_SI1145 si; // I2C

  bool aConnected = false;
  // string ID
  String TextIDs;

  //mqtt topics
  String atopicOnline;
  String atopicVisible;
  String atopicIR;
  String atopicUV;

  // decoded measurements
  float aVisible;
  float aIR;
  float aUV;

  void SensorInit();
  bool ExecMeasurementCycle(uint16_t *gainVis, uint16_t *gainIR, double *uv);

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

template <typename... Args>
void si1145::DEBUG_PRINTLN(Args... args) {
#ifdef SI_DEBUG
  if (logger) {
    logger->println(args...);
  }
#endif
}

#endif // LIBSI1145_H
