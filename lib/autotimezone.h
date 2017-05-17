/*
 * AZ7798 library.
 * Get measurements and set date time.
 * It works via HardwareSerial/SoftwareSerial. ESP-8266 tested.
 *
 * (c) Oleg Moiseenko 2017
 */

#ifndef AUTOTIMEZONE_H
#define AUTOTIMEZONE_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>    // https://github.com/bblanchon/ArduinoJson
#include <TimeLib.h>        // https://github.com/PaulStoffregen/Time
#include <NtpClientLib.h>   // https://github.com/gmag11/NtpClient
#include <xlogger.h>        // logger https://github.com/merlokk/xlogger
#include <pitimer.h>
#include <etools.h>

// stage 1
// http://ip-api.com/json
// http://freegeoip.net/json/
// stage 2
// https://api.teleport.org/api/timezones/iana:Europe%2FKiev/offsets/?date=2017-05-17T09:25:41Z

#define TZ_DEBUG
#define JSON_OBJ_BUFFER_LEN 1024

// timers
#define ERROR_TIMEOUT           20000
#define TID_ERROR_TIMEOUT       0x0001        // timer UID for error timeout

enum TimeZoneState {
  tzInit          = 0x00,
  tzWait          = 0x01,
  tzStage1Send    = 0x02,
  tzWaitStage2    = 0x03,
  tzStage2Send    = 0x04,
  tzGotResponse   = 0x05,
  tzSleep         = 0x06
};

class AutoTimeZone {
public:
  AutoTimeZone();

  bool GotData = false;

  void begin(xLogger *_logger);
  void handle();
  void SetLogger(xLogger *value);

  String getIanaTimezone() const;
  String getTimezoneShortName() const;
  int getBaseOffset() const;
  int getDSTOffset() const;
  int getCurrentOffset() const;

  void GetStr(String &s);

private:
  xLogger *logger = NULL;
  piTimer ttimer;

  TimeZoneState state = tzInit;

  String lat;
  String lon;
  String IP;
  String Country;
  String CountryCode;
  String City;
  String IanaTimezone;
  String TimezoneShortName;
  int BaseOffset;
  int DSTOffset;
  int CurrentOffset;
  bool ProcessStage1(const String &str);
  bool ProcessStage2(const String &str);

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

template <typename... Args>
void AutoTimeZone::DEBUG_PRINTLN(Args... args) {
#ifdef TZ_DEBUG
  if (logger) {
    logger->println(args...);
  }
#endif
}

#endif // AUTOTIMEZONE_H
