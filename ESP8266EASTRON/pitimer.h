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
  int timerID = 0;
  int lastTimeReset = 0;
};

class piTimer {
  public:
    piTimer();

    bool Add(int timerID, int interval, bool fromNow = false); 
    bool Delete(int timerID);
    bool Reset(int timerID);
    bool isArmed(int timerID);
  private:
    timer timers[PI_MAX_TIMERS];
};

#endif // ifndef __PITIMER_H__

