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

bool xParam::LoadFromWeb(const String &url) {

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

