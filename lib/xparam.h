#ifndef __XPARAM_H
#define __XPARAM_H

#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <EEPROM.h>
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <xlogger.h>
#include <etools.h>

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
  template<typename TString>
  String GetParamStr(const TString &paramName);
  template<typename TString>
  bool ParamExists(const TString &paramName);

  template<typename TString>
  String operator[](const TString &paramName) {
    return GetParamStr(paramName);
  }

  void ClearAll();
  bool LoadFromEEPROM();
  bool SaveToEEPROM();
  bool LoadFromRTC();
  bool SaveToRTC();
  bool LoadFromWeb(const String &url);

  void GetParamsJsonStr(String &str);

  void SetLogger(xLogger * _logger);
private:
  xLogger* logger = NULL;
  // memory buffer 4-byte aligned
  char jsonMem[JSON_MEM_BUFFER_LEN] STORE_ATTR = {0};

  template <typename... Args>
  void DEBUG_PRINTLN(Args... args);
};

// not in class

char crc8(const char *data, char len);

// templates

template<typename T, typename TString>
bool xParam::SetParam(const TString &paramName, const T& paramValue) {
  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject *root = &jsonBuffer.parseObject((const char *)jsonMem);
  if (!root->success()){
    DEBUG_PRINTLN(llError, "SetParam: json load error.");
    root = &jsonBuffer.createObject();
  };
  if (!root->success()) {
    DEBUG_PRINTLN(llError, "SetParam: json final check load error.");
    return false;
  }

  root->set(paramName, paramValue);

  if (root->measureLength() > JSON_MEM_BUFFER_LEN - 1) {
    DEBUG_PRINTLN(llError, "SetParam: JSON too big. Can't save it to memory.");
    return false;
  }

  int n = root->printTo(&jsonMem[0], JSON_MEM_BUFFER_LEN - 1);
  jsonMem[n] = 0x00;
  return root->success();
}

template<typename T, typename TString>
bool xParam::GetParam(const TString &paramName, T &param) {
  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject((const char *)jsonMem);
  if (!root.success() || !root.containsKey(paramName) ) {//|| !(root[paramName].is<T>()))
    DEBUG_PRINTLN(llError, "GetParam: error loading json.");
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

template<typename TString>
String xParam::GetParamStr(const TString &paramName) {
  String s = "";
  if (GetParam(paramName, s))
    return s;
  else
    return "";
}

template<typename TString>
bool xParam::ParamExists(const TString &paramName) {
  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject((const char *)jsonMem);

  return (root.success() && root.containsKey(paramName));
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
