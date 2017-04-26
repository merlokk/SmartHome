#ifndef __XPARAM_H
#define __XPARAM_H

#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <xlogger.h>

#define DEBUG

#define JSON_OBJ_BUFFER_LEN 500
#define JSON_MEM_BUFFER_LEN 512

class xParam
{
public:
  xParam();

  void begin();

  template<typename T, typename TString>
  bool SetParam(const TString &paramName, const T &paramValue);
  template<typename T, typename TString>
  bool GetParam(const TString &paramName, T &param);
  template<typename T, typename TString>
  bool GetParamDef(const TString &paramName, T &param, String &defaultParamValue);
  bool GetParamStr(String &paramName, String &paramValue);

  bool LoadParamsFromEEPROM();
  bool SaveParamsToEEPROM();
  bool LoadParamsFromWeb(String &url);

  void GetParamsJsonStr(String &str);

  void SetLogger(xLogger * _logger);
  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);

private:
  xLogger* logger = NULL;
  char jsonMem[JSON_MEM_BUFFER_LEN] = {0};
};

// not in class

uint8_t crc8(const uint8_t *data, uint8_t len);


// templates

template<typename T, typename TString>
bool xParam::SetParam(const TString &paramName, const T& paramValue) {

  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject *root = &jsonBuffer.parseObject(String(jsonMem));
  if (!root->success()){
    DEBUG_PRINTLN("SetParam: json load error.");
    root = &jsonBuffer.createObject();
  };
  if (!root->success())
    DEBUG_PRINTLN("SetParam: json final check load error.");

  root->set(paramName, paramValue);

  int n = root->printTo(&jsonMem[0], JSON_MEM_BUFFER_LEN);
  jsonMem[n] = 0x00;
  return root->success();
}

template<typename T, typename TString>
bool xParam::GetParam(const TString &paramName, T &param) {

  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(String(jsonMem));
  if (!root.success() || !root.containsKey(paramName) ) {//|| !(root[paramName].is<T>()))
    DEBUG_PRINTLN("GetParam: error loading json.");
    return false;
  }

  param = root.get<T>(paramName);

  return true;
}

template<typename T, typename TString>
bool xParam::GetParamDef(const TString &paramName, T &param, String &defaultParamValue) {
  if (!GetParam(paramName, param)) {
    param = defaultParamValue;
  }

  return true;
}

template <typename... Args>
void xParam::DEBUG_PRINTLN(Args... args) {
#ifdef DEBUG
  if (logger) {
    logger->println(args...);
  }
#endif
}



#endif // __XPARAM_H
