/*
 * Logger 
 * 
 * (c) Oleg Moiseenko 2017
 */

#ifndef __XLOGGER_H__
#define __XLOGGER_H__

#include <Arduino.h>
#include <ESP8266WiFi.h>     // https://github.com/esp8266/Arduino
#include <TimeLib.h>         // https://github.com/PaulStoffregen/Time 
#include <NtpClientLib.h>    // https://github.com/gmag11/NtpClient
#include "etools.h"

#define XLOGGER_VERSION      "0.9"

#define TELNET_PORT          23                  // telent port for remote connection
#define LOG_SIZE             2048                // size of log memory in bytes
#define PRINTF_BUFFER_LENGTH 128                 // buffer length for printf
#define LINE_BUFFER_LENGTH   256                 // buffer length for lines (concat print and printf)
extern char pf_buffer[PRINTF_BUFFER_LENGTH];

typedef void (*logCallback)(String &cmd);

enum LogLevel: uint8_t{
  llInfo, 
  llWarning,
  llError,

  llLast
};
extern const char *strLogLevel[llLast];

enum LogTimeFormat: uint8_t {
  ltNone,
  ltStrTime,
  ltMsTime,
  ltGMTTime,

  ltLast
};
extern const char *strLogTimeFormat[ltLast];

struct LogHeader {
  int logTime = 0;
  uint16_t logSize = 0;
  LogLevel logLevel = llInfo;
};

class xLogger: public Print{
  public:
    xLogger();

    void begin(char *_hostName, Stream *_serial = NULL, bool _serialEnabled = false, char *_passwd = "");
    void handle();
    void cmdCallback(logCallback);

    void setSerial(Stream *_serial);
    void enableSerial(bool _serialEnabled);
    void setPassword(char *_passwd);
    void setProgramVersion(char * _programVersion);
    void setTimeFormat(LogTimeFormat _timeFormat);
    void setShowDebugLevel(bool _showDebugLevel);
    void setFilterDebugLevel(LogLevel _logLevel);

    virtual size_t write(uint8_t c);
    virtual size_t write(const uint8_t *buffer, size_t size);

    virtual void print(){}
    virtual void println(){}
    
    template<typename... Args>
    void printf(LogLevel loglev, const char* fmtstr, Args... args)
    {
      int len = snprintf(pf_buffer, sizeof(pf_buffer), fmtstr, args...);
      print(curHeader.logLevel, pf_buffer);
    }
    template<typename... Args>
    void printf(const char* fmtstr, Args... args) {
      printf(llInfo, fmtstr, args...);
    }
    
    template<typename... Args>
    void print(LogLevel loglev, Args... args)
    {
      curHeader.logLevel = loglev;
      Print::print(args...);
    }
    template<typename... Args>
    void print(Args... args)
    {
      print(llInfo, args...);
    }

    template<typename... Args>
    void println(LogLevel loglev, Args... args)
    {
      curHeader.logLevel = loglev;
      Print::println(args...);
    }
    template<typename... Args>
    void println(Args... args)
    {
      println(llInfo, args...);
    }
  private:
    bool serialEnabled = false;
    Stream *logSerial = NULL;
    uint8_t logMem[LOG_SIZE] = {0};
    char passwd[11] = {0};
    bool telnetConnected = false;
    char * programVersion = NULL;
    bool showDebugLevel = true;
    LogLevel filterLogLevel = llInfo;
    LogTimeFormat logTimeFormat = ltStrTime;
    String telnetCommand = "";
    bool telnetAuthenticated = true; // TODO!!!

    // command callback
    logCallback _cmdCallback;

    WiFiServer telnetServer = WiFiServer(TELNET_PORT);
    WiFiClient telnetClient;

    LogHeader curHeader;

    void showInitMessage();
    void formatLogMessage(String &str, const char *buffer, size_t size, LogHeader *header);
    void processLineBuffer();
    void processCommand(String &cmd);
};

#endif // ifndef __XLOGGER_H__

