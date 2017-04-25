#ifndef XPARAM_H
#define XPARAM_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define JSON_MEM_BUFFER 512

class xParam
{
public:
  xParam();

  void begin();

  template<typename T>
  bool SetParam(String &paramName, T &paramValue);
  template<typename T>
  bool GetParam(String &paramName, T &param);
  bool GetParamStr(String &paramName, String &paramValue);

  bool LoadParamsFromEEPROM();
  bool SaveParamsToEEPROM();
  bool LoadParamsFromWeb(String &url);

  void GetParamsJsonStr(String &str);

private:
  StaticJsonBuffer<JSON_MEM_BUFFER> jsonBuffer;
};

#endif // XPARAM_H
