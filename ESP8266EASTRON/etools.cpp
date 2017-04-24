#include <Arduino.h>
#include "etools.h"

void BufferToString(String & str, const char* buf, int len) {
  str.reserve(len + 1); // set internal String buffer length
  // bug in lib! strcpy instead of strncpy!!!! WString.cpp#L176
  for (int i = 0; i < len; i++) {
    str.concat(buf[i]);
  }  
}


