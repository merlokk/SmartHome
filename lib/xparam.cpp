#include <xparam.h>

xParam::xParam(){

}

void xParam::begin(){
  ClearAll();
}

void xParam::ClearAll() {
  memset(&jsonMem[0], 0x00, JSON_MEM_BUFFER_LEN);
  jsonMem[0] = '{';
  jsonMem[1] = '}';
}

bool xParam::LoadFromEEPROM() {
  ClearAll();

  EEPROM.begin(JSON_MEM_BUFFER_LEN);
  EEPROM.get(0, jsonMem);
  EEPROM.end();

  if (crc8(&jsonMem[0], JSON_MEM_BUFFER_LEN - 2) != jsonMem[JSON_MEM_BUFFER_LEN - 1]) {
    ClearAll();
    DEBUG_PRINTLN(llError, F("xParam: Invalid settings in EEPROM, settings was cleared."));

    return false;
  }

  return true;
}

bool xParam::SaveToEEPROM() {
  jsonMem[JSON_MEM_BUFFER_LEN - 1] = crc8(&jsonMem[0], JSON_MEM_BUFFER_LEN - 2);

  EEPROM.begin(JSON_MEM_BUFFER_LEN);
  EEPROM.put(0, jsonMem);
  EEPROM.end();

  return true;
}

bool xParam::LoadFromRTC() {
  ClearAll();
  ESP.rtcUserMemoryRead(0, (uint32_t*)&jsonMem[0], JSON_MEM_BUFFER_LEN);

  if (crc8(&jsonMem[0], JSON_MEM_BUFFER_LEN - 2) != jsonMem[JSON_MEM_BUFFER_LEN - 1]) {
    ClearAll();
    DEBUG_PRINTLN(llError, F("xParam: Invalid settings in RTC, settings was cleared."));

    return false;
  }

  return true;
}

bool xParam::SaveToRTC() {
  jsonMem[JSON_MEM_BUFFER_LEN - 1] = crc8(&jsonMem[0], JSON_MEM_BUFFER_LEN - 2);

  ESP.rtcUserMemoryWrite(0, (uint32_t*)&jsonMem[0], JSON_MEM_BUFFER_LEN);
}

bool xParam::LoadFromWeb(const String &url) {
  DEBUG_PRINTLN(SF("xParam: load from web: ") + url);
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  if(httpCode > 0) {
    DEBUG_PRINTLN(SF("xParam: GET ok: ") + String(httpCode));

    // file found at server
    if (httpCode == HTTP_CODE_OK) {
      String s = http.getString();
      int n = min((int)s.length(), JSON_MEM_BUFFER_LEN - 1);
      s.toCharArray(&jsonMem[0], n);
      jsonMem[n] = 0x00;
      DEBUG_PRINTLN(SF("xParam: loaded ok. length: ") + String(n));
    } else {
      return false;
    }
  } else {
    DEBUG_PRINTLN(llError, SF("xParam: GET error: ") + http.errorToString(httpCode));
    return false;
  }

  http.end();

  return true;
}

void xParam::GetParamsJsonStr(String &str) {
  str = String((const char*)jsonMem);
}

void xParam::SetLogger(xLogger *_logger) {
  logger = _logger;
}

// non class functions
char crc8(const char *data, char len) {
  char crc = 0x00;
  while (len--) {
    char extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      char sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

