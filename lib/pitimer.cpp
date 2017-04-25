#include <pitimer.h>

piTimer::piTimer(){
  
}

bool piTimer::Add(int timerUID, int interval, bool fromNow){
  for (int i = 0; i < PI_MAX_TIMERS; i++) {
    if(!timers[i].timerUID){
      timers[i].timerUID = timerUID;
      timers[i].interval = interval;
      if (fromNow)
        timers[i].lastTimeReset = millis();
      else
        timers[i].lastTimeReset = - interval; // after reset -> millis() == 0!!!
      return true;
    }
  }
  
  return false;  
}

bool piTimer::Delete(int timerUID) {
  int id = getTimerNum(timerUID);
  if (id < 0) return false;

  timers[id].timerUID = 0; 
  return true;
}

bool piTimer::Reset(int timerUID) {
  int id = getTimerNum(timerUID);
  if (id < 0) return false;

  timers[id].lastTimeReset = millis(); 
  return true;
}

bool piTimer::isArmed(int timerUID) {
  int id = getTimerNum(timerUID);
  if (id < 0) return false;

  return (timers[id].lastTimeReset + timers[id].interval < millis());
}

int piTimer::getTimerNum(int timerUID) {
  for (int i = 0; i < PI_MAX_TIMERS; i++) {
    if(timers[i].timerUID == timerUID){
      return i;
    }
  }
  
  return -1;  
}



