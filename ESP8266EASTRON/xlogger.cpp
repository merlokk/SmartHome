#include "xlogger.h"

char pf_buffer[PRINTF_BUFFER_LENGTH];
char lineBuffer[LINE_BUFFER_LENGTH] = {0};
int lineBufferLen = 0;

const char *strLogLevel[llLast] = {
  "Info",
  "Warning",
  "Error",
};

const char *strLogTimeFormat[ltLast] = {
  "none",
  "time from start",
  "milliseconds from start",
  "gmt time (needs NTP available)"
};

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

  if (telnetConnected) {
    // get buffer from client
    while(telnetClient.available()) {  
      char c = telnetClient.read();

      if (c == '\n' || c == '\r') {
        
        // Process the command
        if (telnetCommand.length() > 0) 
          processCommand(telnetCommand);
          
        telnetCommand = ""; 
      } else 
        if (isPrintable(c)) 
          telnetCommand.concat(c);
    }
  }


  
}

void xLogger::processCommand(String &cmd) {
  println(llInfo, "Telnet received command: " + cmd);

  if (cmd == "serial enable") {
    serialEnabled = true;
    return;
  }
  if (cmd == "serial disable") {
    serialEnabled = false;
    return;
  }
  if (cmd == "serial ?") {
    println(SF("Serial enabled: ") + (serialEnabled ? SF("enable") : SF("disable")));
    return;
  }

  if (cmd == "showdebuglvl enable") {
    showDebugLevel = true;
    return;
  }
  if (cmd == "showdebuglvl disable") {
    showDebugLevel = false;
    return;
  }
  if (cmd == "showdebuglvl ?") {
    println(SF("Show debug level: ") + (showDebugLevel ? SF("enable") : SF("disable")));
    return;
  }

  if (cmd == "loglvl info") {
    filterLogLevel = llInfo;
    return;
  }
  if (cmd == "loglvl warning") {
    filterLogLevel = llWarning;
    return;
  }
  if (cmd == "loglvl error") {
    filterLogLevel = llError;
    return;
  }
  if (cmd == "loglvl ?") {
    print("Log level: " + String(strLogLevel[filterLogLevel]));
    return;
  }

  if (cmd == "time none") {
    logTimeFormat = ltNone;
    return;
  }
  if (cmd == "time str") {
    logTimeFormat = ltStrTime;
    return;
  }
  if (cmd == "time ms") {
    logTimeFormat = ltMsTime;
    return;
  }
  if (cmd == "time gmt") {
    logTimeFormat = ltGMTTime;
    return;
  }
  if (cmd == "time ?") {
    print("Time format: " + String(strLogTimeFormat[logTimeFormat]));
    return;
  }

  
}


void xLogger::setSerial(Stream *_serial) {
  logSerial = _serial;
}

void xLogger::setPassword(char *_passwd) {
  strncpy(passwd, _passwd, 10);
}

void xLogger::setTimeFormat(LogTimeFormat _timeFormat) {
  logTimeFormat = _timeFormat;
}

void xLogger::setShowDebugLevel(bool _showDebugLevel) {
  showDebugLevel = _showDebugLevel;
} 

void xLogger::setFilterDebugLevel(LogLevel _logLevel) {
    filterLogLevel = _logLevel;
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
    // show time
    switch (logTimeFormat) {
      case ltStrTime:
        eTimeToStr(str, header->logTime / 1000);
        str += SF(" ");
        break;
      case ltMsTime:
        str = String(millis());
        str += SF(" ");
        break;
      case ltGMTTime:
        if (timeStatus() != timeNotSet) {
          str += NTP.getTimeStr(NTP.getUptime() + millis() / 1000);
          str += SF(" ");
        }
        break;
      case ltNone:
      deafult:
        break;
    };
  
    // show log level
    if (showDebugLevel && true) {
      switch (header->logLevel) {
        case llInfo:     str += SF("INFO: "); break;
        case llWarning:  str += SF("WARNING: "); break;
        case llError:    str += SF("ERROR: "); break;
        default:         str += SF("UNKNOWN: "); break;
      }
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


