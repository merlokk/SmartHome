#include "xlogger.h"

char pf_buffer[PRINTF_BUFFER_LENGTH];
char lineBuffer[LINE_BUFFER_LENGTH] = {0};
int lineBufferLen = 0;

xLogger::xLogger() {
  
}

void xLogger::begin(char *_hostName, Stream *_serial, char *_passwd) {
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  setSerial(_serial);
  setPassword(_passwd);
}

void xLogger::handle() {
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      if (telnetClient) // Close the last connect - only one supported
        telnetClient.stop();

      // Get telnet client
      telnetClient = telnetServer.available();
      telnetClient.flush();  // clear input buffer, else you get strange characters

      // Show the initial message
      showInitMessage();

      // Empty buffer in
      while(telnetClient.available()) 
          telnetClient.read();

    }
  }

  // Is client connected ? (to reduce overhead in active)
  telnetConnected = (telnetClient && telnetClient.connected());
  
}

void xLogger::setSerial(Stream *_serial) {
  logSerial = _serial;
}

void xLogger::setPassword(char *_passwd) {
  
}

void xLogger::showInitMessage() {
  String msg = SF("*** Telnet debug for ESP8266.\r\n");

  msg += SF("Logger version: ") + String(XLOGGER_VERSION) + SF(". Program version: ") + SF("\r\n");

  telnetClient.print(msg);
}

void xLogger::formatLogMessage(String &str, const char *buffer, size_t size, LogHeader *header) {
  str = "";

  if (header) {
    // if show time
    eTimeToStr(str, header->logTime / 1000);
    str += SF(" ");
  
    switch (header->logLevel) {
      case llInfo:     str += SF("INFO: "); break;
      case llWarning:  str += SF("WARNING: "); break;
      case llError:    str += SF("ERROR: "); break;
      default:         str += SF("UNKNOWN: "); break;
    }
  }

  for(int i = 0; i < size; i++) {
    str += String((char)buffer[i]);  
  }
}

size_t xLogger::write(uint8_t c) {
  write(&c, 1);
}

size_t xLogger::write(const uint8_t *buffer, size_t size) {
  if (!size)
    return size;
                              
  memcpy(&lineBuffer[lineBufferLen], buffer, min(size, LINE_BUFFER_LENGTH - lineBufferLen - 1)); // copy with checking length
  lineBufferLen += size;
  
  if (buffer[size - 1] != '\n') {
    return size;
  }
  curHeader.logTime = millis();
  curHeader.logSize = size;

  lineBuffer[lineBufferLen] = 0x00;

  String msg = "";
  formatLogMessage(msg, lineBuffer, lineBufferLen, &curHeader);
  // write to serial
  if (logSerial) {
    logSerial->print(msg);
  }

  // write to telnet
  if (telnetConnected) { 
    telnetClient.print(msg);
  }

 
  lineBufferLen = 0;
  return size;
}


