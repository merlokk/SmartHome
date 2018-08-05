#include "si1145.h"

si1145::si1145() {
  TextIDs = SF("n/a");
}

uint8_t si1145::Reset() {
  si.getLastError();
  uint8_t err = si.reset();

  return err;
}

void si1145::SensorInit(){
  aConnected = false;

  if (!si.begin()) {
    DEBUG_PRINTLN(SF("Si1145 sensor init error. last error:") + String(si.getLastError()));
    return;
  };

  si.getLastError();
  uint8_t siID = si.readPartId();
  uint8_t err = si.getLastError();
  if (!err && siID != 0x00 && siID != 0xFF) {
    // sensor online
    TextIDs = SF("Si1145: partID=0x") + String(siID, HEX) + // ID: 0x45 Si1145, 0x46 Si1146, 0x47 Si1147
        SF(" revID=0x") + String(si.readRevId(), HEX) +     // 0x00
        SF(" seqID=0x") + String(si.readSeqId(), HEX);      // 0x08

    DEBUG_PRINTLN(TextIDs);

    aConnected = true;
  } else {
    TextIDs = SF("offline...");
    DEBUG_PRINTLN(SF("Si1145 sensor offline. error:") + String(err) + SF(" id:0x") + String(siID, HEX));
  }
}

void si1145::begin(xLogger *_logger) {
  atimer.Add(TID_POLL, MILLIS_TO_POLL);

  SetLogger(_logger);

  SensorInit();
}

void si1145::handle() {

  if (atimer.isArmed(TID_POLL)) {

    uint8_t err;
    si.getLastError();

    double visible = si.readVisible();
    double ir = si.readIR();
    double uv = si.readUV();
    err = si.getLastError();

    if (!err) {
      aConnected = true;
      aVisible = visible;
      aIR = ir;
      aUV = uv;
      DEBUG_PRINTLN(SF("Visible=") + String(visible) + SF(", IR=") + String(ir) + " UVindx=" + String(uv));
      if (amqtt){
        amqtt->PublishState(atopicVisible, String(visible));
        amqtt->PublishState(atopicIR, String(ir));
        amqtt->PublishState(atopicUV, String(uv));
        amqtt->PublishState(atopicOnline, SF("ON"));
      }
    } else {
      aConnected = false;
      if (amqtt){
        amqtt->PublishState(atopicOnline, SF("OFF"));
      }
      DEBUG_PRINTLN("Si1145 I2C error: " + String(err));
    }

    atimer.Reset(TID_POLL);
    return;
  }

}

void si1145::SetLogger(xLogger *_logger) {
  logger = _logger;
}

void si1145::SetMQTT(xMQTT *_mqtt, String _topicOnline, String _topicVisible, String _topicIR, String _topicUV) {
  amqtt = _mqtt;

  atopicOnline = _topicOnline;
  atopicVisible = _topicVisible;
  atopicIR = _topicIR;
  atopicUV = _topicUV;
}

bool si1145::Connected() {
  return aConnected;
}

String si1145::GetTextIDs() const {
  return TextIDs;
}

float si1145::GetVisible() const {
  return aVisible;
}

float si1145::GetIR() const {
  return aIR;
}

float si1145::GetUV() const {
  return aUV;
}

