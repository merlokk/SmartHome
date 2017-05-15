#include "az7798.h"

az7798::az7798() {
  responseBuffer.reserve(22);
}

void az7798::begin(Stream *_serial, xLogger *_logger) {
  atimer.Add(TID_POLL, MILLIS_TO_POLL);
  atimer.Add(TID_TIMEOUT, MILLIS_TIMEOUT);

  SetSerial(_serial);
  SetLogger(_logger);

  state = asWait;
}

void az7798::handle() {
  if (state == asInit || !serial)
    return;

  switch (state) {
  case asWait:
    if (!atimer.isArmed(TID_POLL)) {
      break;
    }
    DEBUG_PRINTLN("awa:" + String(processingCommand));

    if (processingCommand == acNone){
      if (Version.length() == 0) {
        SendCommand(acGetVersion);
        return;
      }
      //here set datetime. if timer and needs...

      SendCommand(acGetMeasurements);
      return;
    } else {
      processingCommand = acNone;
    }


    break;

  case asSentCommand:
    while (serial->available()) {
      char c = serial->read();
      if (c == '\r' || c == '\0') {
        state = asGotResponse;
        break;
      }
      responseBuffer += c;
    }

    if (atimer.isArmed(TID_TIMEOUT)) {
      state = asTimeout;
    }
    break;

  case asGotResponse:
    ProcessCommand(processingCommand);

    atimer.Reset(TID_POLL);
    processingCommand = acNone;
    state = asWait;
    break;

  case asTimeout:
    DEBUG_PRINTLN(llError, SF("Receive command timeout. Buffer:") + responseBuffer);

    atimer.Reset(TID_POLL);
    processingCommand = acNone;
    state = asWait;
    break;

  default:
    break;
  }
}

void az7798::SetLogger(xLogger *_logger) {
  logger = _logger;
}

void az7798::SetSerial(Stream *_serial) {
  serial = _serial;
}

bool az7798::Connected() {
//  return millis() - LastGetMeasurements < MILLIS_TO_POLL * 3;
  return true;
}

String az7798::GetVersion() const {
  return Version;
}

String az7798::GetMeasurements() const {
  return Measurements;
}

float az7798::GetTemperature() const {
  return Temperature;
}

float az7798::GetHumidity() const {
  return Humidity;
}

int az7798::GetCO2() const {
  return CO2;
}

void az7798::SendCommand(AZProcessCommands cmd) {
  serial->flush();

  switch (cmd) {
  case acNone:
    processingCommand = acNone;
    break;

  case acGetVersion:
    serial->print("I\r");
    DEBUG_PRINTLN(SF("Sent command: info."));
    processingCommand = acGetVersion;
    state = asSentCommand;
    break;

  case acGetDateTime:
    break;

  case acSetDateTime:
    // number of seconds from 1 jan 2000.
    // ">"
    serial->print("C %d\r");
    DEBUG_PRINTLN(SF("Sent command: set datetime ") + String(0));
    processingCommand = acSetDateTime;
    state = asSentCommand;
    break;

  case acGetMeasurements:
    //": T20.4C:C1753ppm:H47.5%"
    serial->print(":\r");
    DEBUG_PRINTLN(SF("Sent command: GetMeasurements."));
    processingCommand = acGetMeasurements;
    state = asSentCommand;
    break;

  default:
    processingCommand = acNone;
    break;
  }

  responseBuffer = "";
  while (serial->read() != -1);
  atimer.Reset(TID_POLL);
  atimer.Reset(TID_TIMEOUT);
}

void az7798::ProcessCommand(AZProcessCommands cmd) {
  switch (cmd) {
  case acNone:
    break;

  case acGetVersion:
    Version = responseBuffer;
    if (Version.startsWith("i ")) {
      Version = Version.substring(2);
    } else {
      DEBUG_PRINTLN(llWarning, SF("AZ strange version information..."));
    }
    DEBUG_PRINTLN(SF("AZ version: ") + Version);
    break;

  case acGetDateTime:
    break;

  case acSetDateTime:
    // number of seconds from 1 jan 2000.
    // ">"

    break;

  case acGetMeasurements:
    if (responseBuffer.startsWith(": ")) {
      Measurements = responseBuffer.substring(2);
      DEBUG_PRINTLN(SF("AZ mes: ") + Measurements);
      if (Measurements.length() >= 15)
        ExtractMeasurements();
      LastGetMeasurements = millis();
    } else {
      DEBUG_PRINTLN(llError, SF("AZ invalid mes data:") + String(responseBuffer));
    }
    break;

  default:
    break;
  }

  processingCommand = acNone;

  responseBuffer = "";
  while (serial->read() != -1);
}

void ExtractStr(const String &in, String &out, const String &sFrom, const String &sTo) {
  out = "";

  int i1 = in.indexOf(sFrom);
  int i2 = in.indexOf(sTo);
  if (i1 < 0 || i2 < 0 || i1 >= i2) {
    return;
  }

  out = in.substring(i1 + sFrom.length(), i2);
}

void az7798::ExtractMeasurements() {
  Temperature = 0;
  Humidity = 0;
  CO2 = 0;

  String s;
  ExtractStr(Measurements, s, "T", "C:");
  Temperature = s.toFloat();
  ExtractStr(Measurements, s, ":C", "ppm");
  CO2 = s.toInt();
  ExtractStr(Measurements, s, "H", "%");
  Humidity = s.toFloat();
}
