/*
 * Tools
 * 
 * (c) Oleg Moiseenko 2017
 */

#ifndef __ETOOLS_H__
#define __ETOOLS_H__

#include <Arduino.h>

// put strings to flash
#define SF(x) String(F(x))
#define STR_RN SF("\r\n")

//align
#define STORE_ATTR __attribute__((aligned(4)))


// IEEE 754 Float. web check here https://www.h-schmidt.net/FloatConverter/IEEE754.html
// wiki https://en.wikipedia.org/wiki/IEEE_floating_point
union dataFloat {
  float    f;
  uint8_t  arr[4];
  uint32_t i;
};

union dataDouble
{
    double d;          
    uint8_t b[8];
    uint64_t i64;
};

// printing date time utils
#define SECS_PER_MIN  (60UL)
#define SECS_PER_HOUR (3600UL)
#define SECS_PER_DAY  (SECS_PER_HOUR * 24L)

#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  

// String
void BufferToString(String & str, const char* buf, int len);
String IntToStr(int i, int digits = 2);

// tools
int RSSItoQuality(const int RSSI);
void RSSItoStr(String &str, const int RSSI);

#endif // ifndef __ETOOLS_H__

