#include "hdc1080.h"

hdc1080::hdc1080() {
  TextIDs = SF("n/a");
}

uint8_t hdc1080::Reset() {
  uint8_t err = hdc.GetLastError();
  reg = hdc.readRegister();
  err = hdc.GetLastError();
  if (err) return err;

  reg.SoftwareReset = 1;

  hdc.writeRegister(reg);
  err = hdc.GetLastError();
  if (err) return err;

  delay(20);

  reg = hdc.readRegister();
  err = hdc.GetLastError();
  if (err) return err;

  hdc.setResolution(HDC1080_RESOLUTION_11BIT, HDC1080_RESOLUTION_11BIT);
}

void hdc1080::SensorInit(){
  // default address hdc1080 - 0x40
  hdc.begin(0x40);

  hdc.GetLastError();
  uint16_t HDC1080MID = hdc.readManufacturerId();
  uint8_t err = hdc.GetLastError();
  if (!err && HDC1080MID != 0x0000 && HDC1080MID != 0xFFFF) {
    // sensor online
    char HDCSerial[12] = {0};
    HDC1080_SerialNumber sernum = hdc.readSerialNumber();
    sprintf(HDCSerial, "%02X-%04X-%04X", sernum.serialFirst, sernum.serialMid, sernum.serialLast);
    TextIDs = SF("HDC1080: manufacturer ID=0x") + String(HDC1080MID, HEX) +  // 0x5449 ID of Texas Instruments
        SF(" device ID=0x") + String(hdc.readDeviceId(), HEX) +              // 0x1050 ID of the device
        SF(" serial number=") + String(HDCSerial);

    DEBUG_PRINTLN(TextIDs);

 //   hdc1080.heatUp(10); // heating every start -- not cool. needs to have test....
    hdc.setResolution(HDC1080_RESOLUTION_11BIT, HDC1080_RESOLUTION_11BIT);
    aConnected = true;
  } else {
    TextIDs = SF("offline...");
    DEBUG_PRINTLN(SF("HDC1080 sensor offline. error:") + String(err));
    aConnected = false;
  }
}

void hdc1080::begin(xLogger *_logger) {
  atimer.Add(TID_POLL, MILLIS_TO_POLL);
  atimer.Add(TID_SET_TIME, MILLIS_TO_SET_TIME);
  atimer.Add(TID_TIMEOUT, MILLIS_TIMEOUT);

  SetLogger(_logger);

  SensorInit();
}

void hdc1080::handle() {

  if (atimer.isArmed(TID_POLL)) {
    reg = hdc.readRegister();
    DEBUG_PRINTLN(SF("Heater: ") + String(reg.Heater, BIN));
//    mqtt.PublishState("Heater", String(reg.Heater));

   /*   if (reg.Heater) {
        DEBUG_PRINTLN("Try to clear heating state...");
        reg.Heater = 0;
        hdc1080.writeRegister(reg);
      }*/

    double Temp = hdc.readTemperature();
    double Hum = hdc.readHumidity();
    uint8_t err = hdc.GetLastError();
    if (!err) {
      aConnected = true;
      Temperature = Temp;
      Humidity = Hum;
      DEBUG_PRINTLN(SF("T=") + String(Temp) + SF("C, RH=") + String(Hum) + "%");
 //     mqtt.PublishState(SF("Temperature"), String(Temp));
 //     mqtt.PublishState(SF("Humidity"), String(Hum));
 //     mqtt.PublishState(SF("THConnected"), SF("ON"));
    } else {
      aConnected = false;
  //    mqtt.PublishState(SF("THConnected"), SF("OFF"));
      DEBUG_PRINTLN("HDC1080 I2C error: " + String(err));
    }

    atimer.Reset(TID_POLL);
    return;
  }

}

void hdc1080::SetLogger(xLogger *_logger) {
  logger = _logger;
}

bool hdc1080::Connected() {
  return aConnected;
}

String hdc1080::GetTextIDs() const {
  return TextIDs;
}

float hdc1080::GetTemperature() const {
  return Temperature;
}

float hdc1080::GetHumidity() const {
  return Humidity;
}

