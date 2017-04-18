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

enum LogLevel: uint8_t{
  llInfo, 
  llWarning,
  llError
};

enum LogTimeFormat: uint8_t {
  ltStrTime,
  ltMsTime,
  ltGMTTime
};

struct LogHeader {
  int logTime = 0;
  uint16_t logSize = 0;
  LogLevel logLevel = llInfo;
};

class xLogger: public Print{
  public:
    xLogger();

    void begin(char *_hostName, Stream *_serial, char *_passwd);
    void handle();
    void setSerial(Stream *_serial);
    void setPassword(char *_passwd);

    virtual size_t write(uint8_t c);
    virtual size_t write(const uint8_t *buffer, size_t size);
    
    template<typename... Args>
    void printf(LogLevel loglev, const char* fmtstr, Args... args)
    {
      curHeader.logLevel = loglev;
      printf(fmtstr, args...);
    }
    template<typename... Args>
    void printf(const char* fmtstr, Args... args) {
      int len = snprintf(pf_buffer, sizeof(pf_buffer), fmtstr, args...);
      print(pf_buffer);
    }
    
    template<typename... Args>
    void print(LogLevel loglev, Args... args)
    {
      curHeader.logLevel = loglev;
      print(args...);
    }
    template<typename... Args>
    void print(Args... args)
    {
      Print::print(args...);
    }

    template<typename... Args>
    void println(LogLevel loglev, Args... args)
    {
      curHeader.logLevel = loglev;
      println(args...);
    }
    template<typename... Args>
    void println(Args... args)
    {
      Print::println(args...);
    }
  private:
    Stream *logSerial = NULL;
    uint8_t logMem[LOG_SIZE] = {0};
    char passwd[10] = {0};
    bool telnetConnected = false;

    WiFiServer telnetServer = WiFiServer(TELNET_PORT);
    WiFiClient telnetClient;

    LogHeader curHeader;

    void showInitMessage();
    void formatLogMessage(String &str, const char *buffer, size_t size, LogHeader *header);
};

#endif // ifndef __XLOGGER_H__

