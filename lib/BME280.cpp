#include "bme280.h"

bme280::bme280() {
  TextIDs = SF("n/a");
}

uint8_t bme280::Reset() {
  uint8_t err = bme.reset();
  if (err) return err;

//  bme.setResolution(HDC1080_RESOLUTION_11BIT, HDC1080_RESOLUTION_11BIT);
  return 0;
}

void bme280::SensorInit(){
  // default address bme280 - 0x77, SDO=GND=>0x76
  if (!bme.begin(0x77)) {
    return;
  };

  bme.getLastError();
  uint8_t bme280ID = bme.readManufacturerId();
  uint8_t err = bme.getLastError();
  if (!err && bme280ID != 0x0000 && bme280ID != 0xFFFF) {
    // sensor online
    TextIDs = SF("BME280: manufacturerID=0x") + String(bme280ID, HEX);  // 0x60 BME280 ID

    DEBUG_PRINTLN(TextIDs);

//    hdc.setResolution(HDC1080_RESOLUTION_11BIT, HDC1080_RESOLUTION_11BIT);
    aConnected = true;
  } else {
    TextIDs = SF("offline...");
    DEBUG_PRINTLN(SF("BME280 sensor offline. error:") + String(err));
    aConnected = false;
  }
}

void bme280::begin(xLogger *_logger) {
  atimer.Add(TID_POLL, MILLIS_TO_POLL);

  SetLogger(_logger);

  SensorInit();
}

void bme280::handle() {

  if (atimer.isArmed(TID_POLL)) {

    uint8_t err;
    bme.getLastError();
    double Temp = bme.readTemperature();
    double Hum = bme.readHumidity();
    double Pre = bme.readPressure() / 100.0F;
    err = bme.getLastError();

    if (!err) {
      aConnected = true;
      Temperature = Temp;
      Humidity = Hum;
      Pressure = Pre;
      DEBUG_PRINTLN(SF("T=") + String(Temp) + SF("C, RH=") + String(Hum) + "% P=" + String(Pre));
      if (amqtt){
        amqtt->PublishState(atopicT, String(Temp));
        amqtt->PublishState(atopicH, String(Hum));
        amqtt->PublishState(atopicP, String(Pre));
        amqtt->PublishState(atopicOnline, SF("ON"));
      }
    } else {
      aConnected = false;
      if (amqtt){
        amqtt->PublishState(atopicOnline, SF("OFF"));
      }
      DEBUG_PRINTLN("BME280 I2C error: " + String(err));
    }

    atimer.Reset(TID_POLL);
    return;
  }

}

void bme280::SetLogger(xLogger *_logger) {
  logger = _logger;
}

void bme280::SetMQTT(xMQTT *_mqtt, String _topicOnline, String _topicT, String _topicH, String _topicP) {
  amqtt = _mqtt;

  atopicOnline = _topicOnline;
  atopicT = _topicT;
  atopicH = _topicH;
  atopicP = _topicP;
}

bool bme280::Connected() {
  return aConnected;
}

String bme280::GetTextIDs() const {
  return TextIDs;
}

float bme280::GetTemperature() const {
  return Temperature;
}

float bme280::GetHumidity() const {
  return Humidity;
}

float bme280::GetPressure() const {
  return Pressure;
}

