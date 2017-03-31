

#include <Arduino.h>
#include "eastron.h"

mqttMapConfigS eastron630small[eastron630smallLen] = {
  {"Voltage1",       0x01, MDB_INT},
  {"Voltage2",       0x02, MDB_INT},
  {"Voltage3",       0x04, MDB_INT},
  {"Current1",       0x06, MDB_INT},
  {"Current2",       0x08, MDB_INT},
  {"Current3",       0x0A, MDB_INT},
  {"PowerActive1",   0x0C, MDB_INT},
  {"PowerActive2",   0x0E, MDB_INT},
  {"PowerActive3",   0x10, MDB_INT},
  {"PowerVA",        0x12, MDB_INT},
  {"PowerVA",        0x14, MDB_INT},
  {"PowerVA",        0x16, MDB_INT},
  {"PowerVAR",       0x18, MDB_INT},
  {"PowerVAR",       0x1A, MDB_INT},
  {"PowerVAR",       0x1C, MDB_INT}
};

Eastron::Eastron() {
  if (SWAPHWSERIAL)
    ser.swap();
  ser.begin(SDM_BAUD);
  
}

void Eastron::Poll(byte Command) {
  Connected = false;


/*      while (sdmSer.available() > 0)  {                                         //read serial if any old data is available
        sdmSer.read();
      }

      sdmSer.write(sdmarr, FRAMESIZE - 1);                                      //send 8 bytes

      sdmSer.flush();         

      resptime = millis() + MAX_MILLIS_TO_WAIT;

      while (sdmSer.available() < FRAMESIZE)  {
        if (resptime >= millis()) {
          delay(1);
        } else {
          timeouterr = true;
          break;
        }

  */
}

int Eastron::AddModbusDiap(byte Command, word StartDiap, word LengthDiap) {
  
}






