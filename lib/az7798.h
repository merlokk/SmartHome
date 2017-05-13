#ifndef AZ7798_H
#define AZ7798_H

#include <Arduino.h>
#include <pitimer.h>

#define AZ_DEBUG

//timer
piTimer atimer;
// poll
#define MILLIS_TO_POLL          15*1000       // max time to wait for poll
#define MILLIS_TIMEOUT          700           // AZ response timeout
// timers
#define TID_POLL                0x0001        // timer UID for poll
#define TID_TIMEOUT             0x0002        // timer UID for response timeout

enum AZState {
  asInit         = 0x00,
  asWait         = 0x01,
  asSentCommand  = 0x02,
  asGotResponse  = 0x03,
  asTimeout      = 0x04
};

enum AZProcessCommands {
  acNone             = 0x00,
  acGetVersion       = 0x01,
  acGetDateTime      = 0x02,
  acSetDateTime      = 0x03,
  acGetMeasurements  = 0x04
};

class az7798 {
public:
  az7798();

  void begin();
  void handle();



  // strings from AZ
  String Version;
  String DateTime;
  bool SetDateTime;
  String Measurements;
  // decoded measurements
  float Temperature;
  float Humidity;
  int CO2;

  //last connect to AZ
  int LastGetMeasurements;


private:
  AZState state = asInit;
  AZProcessCommands processingCommand = acNone;
  String responseBuffer;

  void SendCommand(AZProcessCommands cmd);
  void ProcessCommand(AZProcessCommands cmd);
  void ExtractMeasurements();
};

#endif // AZ7798_H
