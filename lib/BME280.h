/*
 * BME280 library.
 * Get temperature and humidity.
 * It works via I2C Wire object. It needs to init it first. ESP-8266 tested.
 *
 * (c) Oleg Moiseenko 2018
 */

#ifndef BME280_H
#define BME280_H

#include <Arduino.h>
#include <pitimer.h>
#include <etools.h>
#include <xlogger.h>            // logger https://github.com/merlokk/xlogger

#include <xmqtt.h>

#include <Wire.h>
#include <Adafruit_BME280.h>    // original: https://github.com/adafruit/Adafruit_BME280_Library
                                // with error handling fix: https://github.com/merlokk/Adafruit_BME280_Library

#define BME_DEBUG

// poll
#define MILLIS_TO_POLL          10*1000       // max time to wait for poll

// timers
#define TID_POLL                0x0001        // timer UID for poll

class bme280 {
public:
  bme280();

  void begin(xLogger *_logger);
  void handle();
  void SetLogger(xLogger * _logger);
  void SetMQTT(xMQTT *_mqtt, String _topicOnline, String _topicT, String _topicH, String _topicP);
  uint8_t Reset();

  bool Connected();

  String GetTextIDs() const;
  float GetTemperature() const;
  float GetHumidity() const;
  float GetPressure() const;

private:
  xLogger *logger = NULL;
  piTimer atimer;
  xMQTT *amqtt = NULL;

  Adafruit_BME280 bme; // I2C

  bool aConnected = false;
  // string ID
  String TextIDs;

  //mqtt topics
  String atopicOnline;
  String atopicT;
  String atopicH;
  String atopicP;

  // decoded measurements
  float Temperature;
  float Humidity;
  float Pressure;

  void SensorInit();

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

template <typename... Args>
void bme280::DEBUG_PRINTLN(Args... args) {
#ifdef BME_DEBUG
  if (logger) {
    logger->println(args...);
  }
#endif
}

#endif // BME280_H
