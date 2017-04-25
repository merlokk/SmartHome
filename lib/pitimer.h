/*
 * Timer 
 * 
 * (c) Oleg Moiseenko 2017
 */

#ifndef __PITIMER_H__
#define __PITIMER_H__

#include <Arduino.h>

#define PI_MAX_TIMERS 20

struct timer {
  int timerUID = 0;
  int interval = 0;
  int lastTimeReset = 0;
};

class piTimer {
  public:
    piTimer();

    bool Add(int timerUID, int interval, bool fromNow = false); 
    bool Delete(int timerUID);
    bool Reset(int timerUID);
    bool isArmed(int timerUID);
  private:
    timer timers[PI_MAX_TIMERS];
    int getTimerNum(int timerUID);
};

#endif // ifndef __PITIMER_H__

