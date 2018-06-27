#include "hdc1080.h"

hdc1080::hdc1080() {

}

void hdc1080::begin(Stream *_serial, xLogger *_logger) {
  atimer.Add(TID_POLL, MILLIS_TO_POLL);
  atimer.Add(TID_SET_TIME, MILLIS_TO_SET_TIME);
  atimer.Add(TID_TIMEOUT, MILLIS_TIMEOUT);

  SetLogger(_logger);
}

void hdc1080::handle() {

  if (atimer.isArmed(TID_SET_TIME)) {

    atimer.Reset(TID_SET_TIME);
    return;
  }

}

void hdc1080::SetLogger(xLogger *_logger) {
  logger = _logger;
}

bool hdc1080::Connected() {
  return false;
}

String hdc1080::GetVersion() const {
  return Version;
}

float hdc1080::GetTemperature() const {
  return Temperature;
}

float hdc1080::GetHumidity() const {
  return Humidity;
}

