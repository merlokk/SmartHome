#include "pmsx003.h"

pmsx003::pmsx003() {
  TextIDs = SF("n/a");
}

void pmsx003::SetLogger(xLogger *_logger) {
  logger = _logger;
}

void pmsx003::SetMQTT(xMQTT *_mqtt, String _topicOnline, String _topicPM1_0, String _topicPM2_5, String _topicPM10) {
  amqtt = _mqtt;

  atopicOnline = _topicOnline;
  atopicPM1_0 = _topicPM1_0;
  atopicPM2_5 = _topicPM2_5;
  atopicPM10 = _topicPM10;
}

void pmsx003::begin(xLogger *_logger, Stream *_serial) {
  atimer.Add(TID_POLL, MILLIS_TO_POLL);

  SetLogger(_logger);
  aserial = _serial;

  SensorInit();

  return;
}

void pmsx003::SensorInit() {

  // initialize the sensor
  static_cast<SoftwareSerial*>(aserial)->begin(9600);

  SetStandbyMode(true);
  SetActiveMode(true);

  PmsInit();

  //pinMode(PIN_RST, INPUT_PULLUP);
  //pinMode(PIN_SET, INPUT_PULLUP);

  DEBUG_PRINTLN(TextIDs);

  return;
}

bool pmsx003::Connected() {
  return aConnected;
}

void pmsx003::SetStandbyMode(bool wakeup) {
  uint8_t txbuf[8];
  int txlen = PmsCreateCmd(txbuf, sizeof(txbuf), PMS_CMD_ON_STANDBY, wakeup);
  aserial->write(txbuf, txlen);

  return;
}

void pmsx003::SetActiveMode(bool activeMode) {
  uint8_t txbuf[8];
  int txlen = PmsCreateCmd(txbuf, sizeof(txbuf), PMS_CMD_AUTO_MANUAL, activeMode);
  aserial->write(txbuf, txlen);

  return;
}

void pmsx003::ManualMeasurement() {
  uint8_t txbuf[8];
  int txlen = PmsCreateCmd(txbuf, sizeof(txbuf), PMS_CMD_TRIG_MANUAL, 0);
  aserial->write(txbuf, txlen);

  return;
}

void pmsx003::handle() {

  // read data from serial
  while (aserial->available()) {
      uint8_t c = aserial->read();
      if (PmsProcess(c)) {
          // parse it
          pms_meas_t pms_meas;
          PmsParse(&pms_meas);
          // sum it
  //        pms_meas_sum.pm10 += pms_meas.concPM10_0_amb;
  //        pms_meas_sum.pm2_5 += pms_meas.concPM2_5_amb;
  //        pms_meas_sum.pm1_0 += pms_meas.concPM1_0_amb;
  //        pms_meas_count++;
      }
  }

}

