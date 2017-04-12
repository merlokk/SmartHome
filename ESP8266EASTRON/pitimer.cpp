#include "pitimer.h"

piTimer::piTimer(){
  
}

bool piTimer::Add(int timerID, int interval, bool fromNow){
  for (int i = 0; i < PI_MAX_TIMERS; i++) {
    if(!timers[i].timerID){
      timers[i].timerID = timerID;
      timers[i].interval = interval;
      if (fromNow)
        timers[i].lastTimeReset = millis();
      
      return true;
    }
  }
  
  return false;  
}

bool piTimer::Delete(int timerID) {
  
}

bool piTimer::Reset(int timerID) {
  
}

bool piTimer::isArmed(int timerID) {
  
}


