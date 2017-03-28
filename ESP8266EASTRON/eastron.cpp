

#include <Arduino.h>
#include "eastron.h"


Eastron::Eastron() {
  if (SWAPHWSERIAL)
    ser.swap();
  ser.begin(SDM_BAUD);
  
}

void Eastron::Poll() {
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





