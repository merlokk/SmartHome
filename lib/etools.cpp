#include <Arduino.h>
#include <etools.h>

void BufferToString(String & str, const char* buf, int len) {
  str.reserve(len + 1); // set internal String buffer length
  // bug in lib! strcpy instead of strncpy!!!! WString.cpp#L176
  for (int i = 0; i < len; i++) {
    str.concat(buf[i]);
  }  
}

String IntToStr(int i, int digits) {
  String s = String(i);
  if (s.length() < digits) {
    for(int k = 0; k < digits - s.length(); k++)
      s = '0' + s;
  }
  return s;
}

// https://www.speedguide.net/faq/how-does-rssi-dbm-relate-to-signal-quality-percent-439
// rssi -100 = 0%; -50=100%
int RSSItoQuality(const int RSSI) {
  int quality = 2 * (RSSI + 100);

  if (quality < 0) {
    quality = 0;
  }
  if (quality > 100) {
    quality = 100;
  }
  return quality;
}

void RSSItoStr(String &str, const int RSSI) {
  str = String(RSSI) + " (" + RSSItoQuality(RSSI) + "%)";
}
