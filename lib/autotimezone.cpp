#include "autotimezone.h"

AutoTimeZone::AutoTimeZone() {
}

void AutoTimeZone::begin(xLogger *_logger) {
  SetLogger(_logger);

  ttimer.Add(TID_ERROR_TIMEOUT, ERROR_TIMEOUT);
  state = tzWait;
}

// 2017-05-17T14:32:25Z
String GetJSDate(time_t dt) {
  String s = IntToStr(year(dt), 4) + '-' + IntToStr(month(dt)) + '-' + IntToStr(day(dt)) + 'T' +
      IntToStr(hour(dt)) + ':' + IntToStr(minute(dt)) + ':' + IntToStr(second(dt)) + 'Z';
  return s;
}

void AutoTimeZone::handle() {
  if (state == tzInit)
    return;

  switch (state) {
  case tzWait:
    if (ttimer.isArmed(TID_ERROR_TIMEOUT)) {
      state = tzStage1Send;
    }
    break;

  case tzStage1Send: {
    DEBUG_PRINTLN(SF("TZ:HTTP GET stage 1"));
    HTTPClient http;
    http.begin(SF("http://ip-api.com/json"));
    int httpCode = http.GET();
    if(httpCode > 0) {
      DEBUG_PRINTLN(SF("TZ:HTTP GET ok: ") + String(httpCode));

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
         if (ProcessStage1(http.getString())) {
           state = tzWaitStage2;
         } else {
           ttimer.Reset(TID_ERROR_TIMEOUT);
           state = tzWait;
         }
      }
    } else {
      DEBUG_PRINTLN(llError, SF("TZ:HTTP GET error: ") + http.errorToString(httpCode));
    }

    http.end();
    break;
  }

  case tzWaitStage2:
    if (timeStatus() == timeSet && ttimer.isArmed(TID_ERROR_TIMEOUT)) {
      state = tzStage2Send;
    }

    break;

  case tzStage2Send:{
    HTTPClient http;
    String ia = IanaTimezone;
    DEBUG_PRINTLN(SF("TZ:HTTP GET stage 2. iana:") + ia + SF(" now:") + GetJSDate(now()));
    ia.replace("/", "%2F");
    http.begin(SF("http://api.teleport.org/api/timezones/iana:") + ia + SF("/offsets/?date=") + GetJSDate(now()));
    int httpCode = http.GET();
    if(httpCode > 0) {
      DEBUG_PRINTLN(SF("TZ:HTTP GET ok: ") + String(httpCode));

      // file found at server
      if(httpCode == HTTP_CODE_OK) {
        String str = http.getString();
        String strOut = "";

        // short json
        int bCnt = 0;
        int bFrom = -1;
        for (int i = 0; i < str.length(); i++) {
          if (str[i] == '{')
            bCnt ++;
          if (str[i] == '}')
            bCnt --;

          if (bCnt == 2 && bFrom < 0) {
            bFrom = i;
          }

          if (bCnt == 1 && bFrom >= 0) {
            strOut = str.substring(0, bFrom) + "[]" + str.substring(i + 1);
            break;
          }

          if (i == str.length() - 1)
            strOut = str;
        }

        // process json
        if (ProcessStage2(strOut)) {
          state = tzGotResponse;
        } else {
          ttimer.Reset(TID_ERROR_TIMEOUT);
          state = tzWaitStage2;
        }
      }
    } else {
      DEBUG_PRINTLN(llError, SF("TZ:HTTP GET error: ") + http.errorToString(httpCode));
    }

    http.end();
    break;
  }

  case tzGotResponse: {
    String s;
    GetStr(s);
    DEBUG_PRINTLN(SF("TZ: ok. ") + s);

    // all is OK
    GotData = true;

    state = tzSleep;
    break;
  }

  case tzSleep:
    break;

  default:
    break;
  }

}

String AutoTimeZone::getIanaTimezone() const {
  return IanaTimezone;
}

String AutoTimeZone::getTimezoneShortName() const {
  return TimezoneShortName;
}

int AutoTimeZone::getBaseOffset() const {
  return BaseOffset;
}

int AutoTimeZone::getDSTOffset() const {
  return DSTOffset;
}

int AutoTimeZone::getCurrentOffset() const {
  return CurrentOffset;
}

void AutoTimeZone::GetStr(String &s) {
  s = IanaTimezone + " [" + TimezoneShortName + "] offset:" + String(CurrentOffset) +
      " b/d: " + String(BaseOffset) + "/" + String(DSTOffset) +
      " IP:" + IP + " (" + CountryCode + ")" + Country + "-" + City +
      " c:" + lat + ":" + lon;
}

bool AutoTimeZone::ProcessStage1(const String &str) {
  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(str);
  if (!root.success()) {
    DEBUG_PRINTLN(llError, SF("AZ: error loading json."));
    return false;
  }

  IP = root.get<String>("query");

  if (root["status"] != "success") {
    DEBUG_PRINTLN(llError, SF("AZ: server returned error json: ") + root["message"].as<String>());
    return false;
  }

  lat = root["lat"].as<String>();
  lon = root["lon"].as<String>();
  Country = root["country"].as<String>();
  CountryCode = root["countryCode"].as<String>();
  City = root["city"].as<String>();
  IanaTimezone = root["timezone"].as<String>();

  return true;
}

bool AutoTimeZone::ProcessStage2(const String &str) {
  StaticJsonBuffer<JSON_OBJ_BUFFER_LEN> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(str);
  if (!root.success()) {
    DEBUG_PRINTLN(llError, SF("AZ: error loading json."));
    return false;
  }

  if (root["total_offset_min"] == "") {
    DEBUG_PRINTLN(llError, SF("AZ: server returned no data."));
    return false;
  }

  TimezoneShortName = root["short_name"].as<String>();
  BaseOffset = root["base_offset_min"].as<int>();
  DSTOffset = root["dst_offset_min"].as<int>();
  CurrentOffset = root["total_offset_min"].as<int>();

  return true;
}

void AutoTimeZone::SetLogger(xLogger *value)
{
  logger = value;
}
