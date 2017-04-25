#include <xparam.h>

xParam::xParam(){

}

void xParam::begin(){

}

bool xParam::GetParamStr(String &paramName, String &paramValue)
{

}

bool xParam::LoadParamsFromEEPROM()
{

}

bool xParam::SaveParamsToEEPROM()
{

}

bool xParam::LoadParamsFromWeb(String &url)
{

}

void xParam::GetParamsJsonStr(String &str)
{
  str = "";
  JsonObject& root = jsonBuffer.createObject();
  root.printTo(str);
}

template<typename T>
bool xParam::SetParam(String &paramName, T &paramValue)
{

}

template<typename T>
bool xParam::GetParam(String &paramName, T &param)
{

}
