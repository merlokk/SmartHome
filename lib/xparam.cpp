#include <xparam.h>

xParam::xParam(){

}

void xParam::begin(){

}

bool xParam::GetParamStr(String &paramName, String &paramValue) {

  return true;
}

bool xParam::LoadParamsFromEEPROM() {
  memset(&jsonMem[0], 0x00, JSON_MEM_BUFFER_LEN);

  EEPROM.begin(JSON_MEM_BUFFER_LEN);
//  EEPROM.get(0, jsonMem);
  EEPROM.end();

/*  if (jsonMem[JSON_MEM_BUFFER_LEN] != crc8(&jsonMem[0], JSON_MEM_BUFFER_LEN - 1)) {
    memset(&jsonMem[0], 0x00, JSON_MEM_BUFFER_LEN);
    DEBUG_PRINTLN(F("Invalid settings in EEPROM, settings was cleared."));
  }
*/
  return true;
}

bool xParam::SaveParamsToEEPROM() {
//  jsonMem[JSON_MEM_BUFFER_LEN] = crc8(&jsonMem[0], JSON_MEM_BUFFER_LEN - 1);

  EEPROM.begin(JSON_MEM_BUFFER_LEN);

  EEPROM.end();

  return true;
}

bool xParam::LoadParamsFromWeb(String &url) {

  return true;
}

void xParam::GetParamsJsonStr(String &str) {
  str = String((const char*)jsonMem);
}

void xParam::SetLogger(xLogger *_logger) {
  logger = _logger;
}

// non class functions
uint8_t crc8(const uint8_t *data, uint8_t len) {
  uint8_t crc = 0x00;
  while (len--) {
    uint8_t extract = *data++;
    for (byte tempI = 8; tempI; tempI--) {
      uint8_t sum = (crc ^ extract) & 0x01;
      crc >>= 1;
      if (sum) {
        crc ^= 0x8C;
      }
      extract >>= 1;
    }
  }
  return crc;
}

