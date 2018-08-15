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

  if (!si.begin(false)) {
    DEBUG_PRINTLN(SF("Si1145 sensor init error. last error:") + String(si.getLastError()));
    return;
  };

  si.getLastError();
  uint8_t siID = si.readPartId();
  si.setMeassureChannels(SI1145_PARAM_CHLIST_ENUV | SI1145_PARAM_CHLIST_ENALSIR | SI1145_PARAM_CHLIST_ENALSVIS |
                         SI1145_PARAM_CHLIST_ENPS2 | SI1145_PARAM_CHLIST_ENPS3);
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

bool si1145::ExecMeasurementCycle(uint16_t *gainVis, uint16_t *gainIR) {
  *gainVis = 0x8000;
  *gainIR = 0x8000;
  si.getLastError();


  si.setVisibleGain(*gainVis);
  si.setIRGain(*gainIR);
  uint8_t merr = si.takeForcedMeasurement();
  if (merr || si.getLastError()) {
    DEBUG_PRINTLN(SF("SI1145 error get data for gain calculation. err:") + String(merr, HEX));
    return false;
  } else {
    int gvis = si.readVisible() - si.getADCOffset();
    int gir = si.readIR() - si.getADCOffset();
    *gainVis = si.calcOptimalGainFromSignal(gvis);
    *gainIR = si.calcOptimalGainFromSignal(gir);
    DEBUG_PRINTLN(SF("vis=") + String(gvis) + SF(" ir=") + String(gir) +
                  SF(" gain Vis=0x") + String(*gainVis, HEX) + SF(" Ir=0x") + String(*gainIR, HEX));
  }

  uint8_t err;
  si.getLastError();

  // cycle while not error
  bool repeat = false;
  while (repeat) {
    repeat = false;
    uint8_t meserr = si.takeForcedMeasurement();
    if(meserr)
      DEBUG_PRINTLN(SF("SI1145 take measurement error:0x") + String(meserr, HEX));

    err = si.getLastError();
    if(err) {
      DEBUG_PRINTLN(SF("SI1145 error get data for gain calculation. err:") + String(err, HEX));
      return false;
    }

    switch(meserr) {
    case 0x80:  // Invalid command
    case 0x88:  // PS1 overflow
    case 0x89:  // PS2 overflow
    case 0x8A:  // PS3 overflow
      break;
    case 0x8C:  // VIS overflow
      *gainVis = si.decGain(*gainVis);
      if (*gainVis == 0xFFFF) {
        *gainVis = 0x8000;
      } else {
        repeat = true;
        DEBUG_PRINTLN(SF("SI1145 Vis measurement repeat. new gain:0x") + String(*gainVis));
      }
      break;
    case 0x8D:  // IR overflow
      *gainIR = si.decGain(*gainIR);
      if (*gainIR == 0xFFFF) {
        *gainIR = 0x8000;
      } else {
        repeat = true;
        DEBUG_PRINTLN(SF("SI1145 IR measurement repeat. new gain:0x") + String(*gainIR));
      }
      break;
    case 0x8E:  // AUX overflow
      break;
    default:
      break;
    }
  }

  si.setVisibleGain(*gainVis);
  si.setIRGain(*gainIR);

  return true;
}

void si1145::handle() {

  if (atimer.isArmed(TID_POLL)) {
    uint16_t gainVis = 0x8000;
    uint16_t gainIR = 0x8000;

    // if we can do measurement (dont have i2c error)
    if (ExecMeasurementCycle(&gainVis, &gainIR)) {
      double visible = si.readVisible();
      double ir = si.readIR();
      double uv = si.readUV() / 100; // the index is multiplied by 100
      double ref = si.readPS2();
      double temp = si.readPS3();
      uint8_t err = si.getLastError();

      if (!err) {
        aConnected = true;
        aVisible = visible;
        aIR = ir;
        aUV = uv;
        DEBUG_PRINTLN(SF("SI1145 Visible=") + String(visible) + SF(" IR=") + String(ir) + SF(" UVindx=") + String(uv) +
                      SF(" Temp=") + String(temp) + SF(" ref=") + String(ref));
        ir = ir - si.getADCOffset();
        if (ir < 0) ir = 0;

        visible = visible - si.getADCOffset();
        if (visible < 0) visible = 0;

        float lux = (5.41f * visible) / si.calcGain(gainVis) + (-0.08f * ir) / si.calcGain(gainIR);
        if (lux < 0)
          lux = 0.0;
        float luxir = ir / (si.calcGain(gainIR) * 2.44f);
        if (lux < 0)
          lux = 0.0;

        DEBUG_PRINTLN(SF("Real Visible=") + String(visible / si.calcGain(gainVis)) + SF(" IR=") + String(ir / si.calcGain(gainIR)) +
                      SF(" lux=") + String(lux) + SF(" luxir=") + String(luxir));

        if (amqtt){
          amqtt->PublishState(atopicVisible, String(lux));
          amqtt->PublishState(atopicIR, String(luxir));
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
    } else {
      aConnected = false;
      if (amqtt){
        amqtt->PublishState(atopicOnline, SF("OFF"));
      }
      DEBUG_PRINTLN("Si1145 I2C error.");
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

