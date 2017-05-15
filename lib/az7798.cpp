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
      if (c == '\r') {
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
//  return millis() - LastGetMeasurements < 20000; // 20s timeout
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
  Serial.flush();

  switch (cmd) {
  case acNone:
    processingCommand = acNone;
    break;

  case acGetVersion:
    serial->println("I\r\n");
    DEBUG_PRINTLN(SF("Sent command: info."));
    processingCommand = acGetVersion;
    state = asSentCommand;
    break;

  case acGetDateTime:
    break;

  case acSetDateTime:
    // number of seconds from 1 jan 2000.
    // ">"
    serial->println("C %d\r");
    DEBUG_PRINTLN(SF("Sent command: set datetime ") + String(0));
    processingCommand = acSetDateTime;
    state = asSentCommand;
    break;

  case acGetMeasurements:
    //": T20.4C:C1753ppm:H47.5%"
    serial->println(":\r");
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
  DEBUG_PRINTLN(SF("Process:") + cmd);

  switch (cmd) {
  case acNone:
    break;

  case acGetVersion:
    Version = responseBuffer;
    break;

  case acGetDateTime:
    break;

  case acSetDateTime:
    // number of seconds from 1 jan 2000.
    // ">"

    break;

  case acGetMeasurements:
    Measurements = responseBuffer;
    if (Measurements.length() >= 15)
      ExtractMeasurements();
    LastGetMeasurements = millis();
    break;

  default:
    break;
  }

  processingCommand = acNone;

  responseBuffer = "";
  while (serial->read() != -1);
}

void az7798::ExtractMeasurements() {
  Temperature = 0;
  Humidity = 0;
  CO2 = 0;

}
