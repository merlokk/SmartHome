#include "xlogger.h"

char pf_buffer[PRINTF_BUFFER_LENGTH];
char lineBuffer[LINE_BUFFER_LENGTH] = {0};
int lineBufferLen = 0;

xLogger::xLogger() {
  
}

void xLogger::begin(char *_hostName, Stream *_serial, bool _serialEnabled, char *_passwd) {
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  setSerial(_serial);
  enableSerial(_serialEnabled);
  setPassword(_passwd);
}

void xLogger::enableSerial(bool _serialEnabled) {
  serialEnabled = _serialEnabled && logSerial;
}

void xLogger::setProgramVersion(char * _programVersion) {
  programVersion = _programVersion;
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

  if (programVersion && strnlen(programVersion, 1))
    msg += SF(" Program version: ") + String(programVersion);
  msg += SF(" Logger version: ") + String(XLOGGER_VERSION) + SF(".") + STR_RN;

  msg += "Enable serial=" + (serialEnabled ? SF("enable") : SF("disable")) + STR_RN;

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

void xLogger::processLineBuffer() {
  // add end of char
  lineBuffer[lineBufferLen] = 0x00;

  // if end of line or buffer full
  if (lineBuffer[lineBufferLen - 1] != '\n' && LINE_BUFFER_LENGTH - 1 > lineBufferLen) {  
    return;
  }

  curHeader.logTime = millis();

  String msg = "";
  formatLogMessage(msg, lineBuffer, lineBufferLen, &curHeader);
  curHeader.logSize = msg.length();
  // write to serial
  if (serialEnabled && logSerial) {
    logSerial->print(msg);
  }

  // write to telnet
  if (telnetConnected) { 
    telnetClient.print(msg);
  }

 
  lineBufferLen = 0;
}

size_t xLogger::write(uint8_t c) {
  lineBuffer[lineBufferLen] = c;
  lineBufferLen++;

  processLineBuffer();
  return 1;
}

size_t xLogger::write(const uint8_t *buffer, size_t size) {
  if (!size)
    return size;
                              
  memcpy(&lineBuffer[lineBufferLen], buffer, min((int)size, LINE_BUFFER_LENGTH - lineBufferLen - 1)); // copy with checking length
  lineBufferLen += size;

  processLineBuffer();
  
  return size;
}


