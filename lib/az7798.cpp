#include "az7798.h"

az7798::az7798() {
  responseBuffer.reserve(22);
}

void az7798::begin() {

  atimer.Add(TID_POLL, MILLIS_TO_POLL);

  state = asWait;
}

void az7798::handle() {
  if (state == asInit)
    return;

  switch (state) {
  case asWait:
    //here check timer to send command

    if (processingCommand == acNone){
      if (Version.length() == 0) {
        SendCommand(acGetVersion);
        return;
      }
      //here set datetime. if timer and needs...

      SendCommand(acGetMeasurements);
      return;
    }


    break;

  case asSentCommand:
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\r') {
        state = asGotResponse;
        break;
      }
      responseBuffer += c;
    }
    //here if timeout state = asTimeout;
    break;

  case asGotResponse:
    ProcessCommand(processingCommand);
    state = asWait;
    break;

  case asTimeout:
    // add log
    state = asWait;
    break;

  default:
    break;
  }
}

void az7798::SendCommand(AZProcessCommands cmd) {
  Serial.flush();

  switch (cmd) {
  case acNone:
    processingCommand = acNone;
    break;

  case acGetVersion:
    Serial.println("I\r");
    processingCommand = acGetVersion;
    state = asSentCommand;
    break;

  case acGetDateTime:
    break;

  case acSetDateTime:
    // number of seconds from 1 jan 2000.
    // ">"
    Serial.println("C %d\r");
    processingCommand = acSetDateTime;
    state = asSentCommand;
    break;

  case acGetMeasurements:
    //": T20.4C:C1753ppm:H47.5%"
    Serial.println(":\r");
    processingCommand = acGetMeasurements;
    state = asSentCommand;
    break;

  default:
    processingCommand = acNone;
    break;
  }

  responseBuffer = "";
  while (Serial.read() != -1);
}

void az7798::ProcessCommand(AZProcessCommands cmd) {
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
  while (Serial.read() != -1);
}

void az7798::ExtractMeasurements() {
  Temperature = 0;
  Humidity = 0;
  CO2 = 0;

}
