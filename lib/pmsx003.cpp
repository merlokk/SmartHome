#include "pmsx003.h"

struct tPMSVer {
  uint8_t version;
  char *name;
};

tPMSVer PMSVer[] = {
  {0x80, "PMS5003"},
  {0x91, "PMS7003"},
  {0x97, "PMSA003"}
};

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

char *pmsx003::GetVersionName(uint8_t ver) {
  for(int i = 0; i < 2; i++) {
    if (PMSVer[i].version == ver)
      return PMSVer[i].name;
  }
  return "n/a";
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

  PmsInit();
  while (aserial->available())  aserial->read();

  // wakeup and reset
  SetSleepWakeupMode(true);
  delay(3000);

  // sensor must send us 1st measurement
  if (ReadPMSPacket()) {
    pms_meas_t pms_meas;
    PmsParse(&pms_meas);
    version = pms_meas.version;
    errorCode = pms_meas.errorCode;
  }

  bool res = SetAutoSendMode(false);
  delay(100);

  while (aserial->available())  aserial->read();

  //pinMode(PIN_RST, INPUT_PULLUP);
  //pinMode(PIN_SET, INPUT_PULLUP);

  if (res) {
    TextIDs = SF("Plantower PMS sensor online. Version: 0x") + String(version, HEX) +
        SF(" name: ") + String(GetVersionName(version));
  } else {
    TextIDs = SF("Plantower PMS sensor offline.");
  }
  DEBUG_PRINTLN(TextIDs);

  return;
}

bool pmsx003::ReadPMSPacket() {
  // read data from serial
  while (aserial->available()) {
      uint8_t c = aserial->read();
      if (PmsProcess(c)) {
        return true;
      }
  }
  return false;
}

bool pmsx003::Connected() {
  return aConnected;
}

uint8_t pmsx003::getErrorCode() const {
  return errorCode;
}

// when wakeup pms resets and first command can come in 3s. not earlier!
void pmsx003::SetSleepWakeupMode(bool wakeup) {
  uint8_t txbuf[8];
  int txlen = PmsCreateCmd(txbuf, sizeof(txbuf), PMS_CMD_ON_STANDBY, wakeup);
  aserial->write(txbuf, txlen);

  return;
}

bool pmsx003::SetAutoSendMode(bool activeMode) {
  while (aserial->available())  aserial->read();

  uint8_t txbuf[8];
  int txlen = PmsCreateCmd(txbuf, sizeof(txbuf), PMS_CMD_AUTO_MANUAL, activeMode);
  aserial->write(txbuf, txlen);

  delay(15);

  if (ReadPMSPacket()) {
    uint16_t res = PmsParse16();
    if (res != PMS_CMD_AUTO_MANUAL >> 8 + activeMode) {
      DEBUG_PRINTLN(SF("PMS set mode OK. res: 0x") + String(res, HEX));
    } else {
      DEBUG_PRINTLN(SF("PMS set mode error: wrong response from pms (crc ok): 0x") + String(res, HEX));
    }
  } else {
    DEBUG_PRINTLN(SF("PMS set mode error: no or wrong response from pms."));
    return false;
  }

  return true;
}

void pmsx003::ManualMeasurement() {
  uint8_t txbuf[8];
  int txlen = PmsCreateCmd(txbuf, sizeof(txbuf), PMS_CMD_TRIG_MANUAL, 0);
  aserial->write(txbuf, txlen);

  return;
}

void pmsx003::handle() {

  if (atimer.isArmed(TID_POLL)) {

    DEBUG_PRINTLN("Manual mes...");
    ManualMeasurement();




    atimer.Reset(TID_POLL);
  }


  // read data
  while (ReadPMSPacket()) {
    // parse it
    pms_meas_t pms_meas;
    PmsParse(&pms_meas);

    errorCode = pms_meas.errorCode;

    // sum it
  //        pms_meas_sum.pm10 += pms_meas.concPM10_0_amb;
  //        pms_meas_sum.pm2_5 += pms_meas.concPM2_5_amb;
  //        pms_meas_sum.pm1_0 += pms_meas.concPM1_0_amb;
  //        pms_meas_count++;
    DEBUG_PRINTLN(SF("PMS ver: 0x") + String(pms_meas.version, 16) +
                  SF(" err: ") + String(pms_meas.errorCode) +
                  SF(" PM1.0: ") + String(pms_meas.concPM1_0_amb) +
                  SF(" PM2.5: ") +  String(pms_meas.concPM2_5_amb) +
                  SF(" PM10: ") + String(pms_meas.concPM10_0_amb));
    DEBUG_PRINTLN(SF(" PM1.0: ") + String(pms_meas.concPM1_0_CF1) +
                  SF(" PM2.5: ") +  String(pms_meas.concPM2_5_CF1) +
                  SF(" PM10: ") + String(pms_meas.concPM10_0_CF1));
    DEBUG_PRINTLN(SF(" raw0.3: ") + String(pms_meas.rawGt0_3um) +
                  SF(" raw0.5: ") +  String(pms_meas.rawGt0_5um) +
                  SF(" raw1.0: ") + String(pms_meas.rawGt1_0um) +
                  SF(" raw2.5: ") + String(pms_meas.rawGt2_5um) +
                  SF(" raw5.0: ") + String(pms_meas.rawGt5_0um) +
                  SF(" raw10.0: ") + String(pms_meas.rawGt10_0um));
  }

}

