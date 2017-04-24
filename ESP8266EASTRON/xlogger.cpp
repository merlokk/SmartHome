#include "xlogger.h"

char pf_buffer[PRINTF_BUFFER_LENGTH];
char lineBuffer[LINE_BUFFER_LENGTH] = {0};
int lineBufferLen = 0;

const char *strLogLevel[llLast] = {
  "n/a",
  "Info",
  "Warning",
  "Error",
};

const char *strLogTimeFormat[ltLast] = {
  "none",
  "time from start",
  "milliseconds from start",
  "milliseconds from previous log entry",
  "gmt time (needs NTP available)"
};

xLogger::xLogger() {
  
}

void xLogger::begin(char *_hostName, Stream *_serial, bool _serialEnabled, char *_passwd) {
  hostName = String(_hostName);
  telnetServer.begin();
  telnetServer.setNoDelay(true);
  setSerial(_serial);
  enableSerial(_serialEnabled);
  setPassword(_passwd);
}

void xLogger::cmdCallback(logCallback cb, const char* _commandDescription) {
  _cmdCallback = cb;
  commandDescription = _commandDescription;
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
      if (telnetAuthenticated)
        showLog();

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

bool xLogger::processCommand(String &cmd) {
  // process login
  if (!telnetAuthenticated) {

    if (cmd == String(passwd)) {
      telnetClient.println("Password accepted.");
      println(llInfo, "Password accepted.");

      showLog();

      telnetAuthenticated = true;
      return true;
    }

    telnetClient.println("Password rejected.");
    println(llError, "Password (" + cmd + ") rejected.");

    return false;
  }

  // process command
  println(llInfo, "Telnet received command: " + cmd);

  if (cmd == "?") {
    showInitMessage();
    return true;
  }

  if (cmd == "showlog") {
    showLog();
    return true;
  }

  if (cmd == "serial enable") {
    serialEnabled = true;
    return true;
  }
  if (cmd == "serial disable") {
    serialEnabled = false;
    return true;
  }
  if (cmd == "serial ?") {
    println(SF("Serial: ") + (serialEnabled ? SF("enable") : SF("disable")));
    return true;
  }

  if (cmd == "showdebuglvl enable") {
    showDebugLevel = true;
    return true;
  }
  if (cmd == "showdebuglvl disable") {
    showDebugLevel = false;
    return true;
  }
  if (cmd == "showdebuglvl ?") {
    println(SF("Show debug level: ") + (showDebugLevel ? SF("enable") : SF("disable")));
    return true;
  }

  if (cmd == "loglvl info") {
    filterLogLevel = llInfo;
    return true;
  }
  if (cmd == "loglvl warning") {
    filterLogLevel = llWarning;
    return true;
  }
  if (cmd == "loglvl error") {
    filterLogLevel = llError;
    return true;
  }
  if (cmd == "loglvl ?") {
    println(filterLogLevel, "Log level: " + String(strLogLevel[filterLogLevel]));
    return true;
  }

  if (cmd == "time none") {
    logTimeFormat = ltNone;
    return true;
  }
  if (cmd == "time str") {
    logTimeFormat = ltStrTime;
    return true;
  }
  if (cmd == "time ms") {
    logTimeFormat = ltMsTime;
    return true;
  }
  if (cmd == "time btw") {
    logTimeFormat = ltMsBetween;
    return true;
  }
  if (cmd == "time gmt") {
    logTimeFormat = ltGMTTime;
    return true;
  }
  if (cmd == "time ?") {
    println("Time format: " + String(strLogTimeFormat[logTimeFormat]));
    return true;
  }

  bool res = false;
  if (_cmdCallback) 
    res = _cmdCallback(cmd);

  if (!res) 
    println(llError, "Unknown command.");
  
  return res;
}


void xLogger::setSerial(Stream *_serial) {
  logSerial = _serial;
}

void xLogger::setPassword(char *_passwd) {
  strncpy(passwd, _passwd, 10);
  telnetAuthenticated = !strnlen(passwd, 1);
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
    msg += SF("Program version: ") + String(programVersion);
  msg += SF(" Logger version: ") + String(XLOGGER_VERSION) + SF(".") + STR_RN;
  msg += SF("Host name: ") + hostName + SF(" IP:") + WiFi.localIP().toString() + SF(" Mac address:") + WiFi.macAddress() + STR_RN;
  msg += SF("Free Heap RAM: ") + String(ESP.getFreeHeap()) + STR_RN + STR_RN;

  msg += SF("Command serial [enable|disable|?] write log to serial debug port.") + STR_RN;
  msg += SF("Serial: ") + (serialEnabled ? SF("enable") : SF("disable")) + STR_RN;
  msg += SF("Command showdebuglvl [enable|disable|?] shows debug level in log lines.")+ STR_RN;
  msg += SF("Show debug level: ") + (showDebugLevel ? SF("enable") : SF("disable")) + STR_RN;
  msg += SF("Command loglvl [info|warning|error|?] filters messages by log level.") + STR_RN;
  msg += SF("Log level: ") + String(strLogLevel[filterLogLevel]) + STR_RN;
  msg += SF("Command time [none|str|ms|btw|gmt|?] shows time in log lines.") + STR_RN;
  msg += SF("Time format: ") + String(strLogTimeFormat[logTimeFormat]) + STR_RN;
  if (commandDescription && _cmdCallback)
    msg += String(commandDescription) + STR_RN;
  msg += STR_RN;
  
  if (!telnetAuthenticated && strnlen(passwd, 1))
    msg += SF("Please, enter password before entering commands. Password length may be up to 10 symbols.") + STR_RN;

  telnetClient.print(msg);
}

int xLogger::getNextLogPtr(int fromPtr) {
  int ptr = 0;
  LogHeader header;

  while (ptr <= LOG_SIZE - 1) {
    memcpy(&header, &logMem[ptr], sizeof(LogHeader));
    if (header.logTime == 0 && header.logLevel == llNone)
      return ptr;

    // check data length
    if (header.logSize < 0 || header.logSize > LOG_SIZE - ptr + 1)
      break;
      
    ptr += sizeof(LogHeader) + header.logSize + 1;
    if (ptr > fromPtr)
      return ptr;
  }
  
  return -1;
}

int xLogger::getEmptytLogPtr() {
  int ptr = 0;
  LogHeader header;

  while (ptr <= LOG_SIZE - 1) {
    memcpy(&header, &logMem[ptr], sizeof(LogHeader));
    if (header.logTime == 0 && header.logLevel == llNone) {
      return ptr;
    }

    // check data length
    if (header.logSize < 0 || header.logSize > LOG_SIZE - ptr + 1)
      break;

    ptr += sizeof(LogHeader) + header.logSize + 1;
  }

  return -1;
}

void xLogger::addLogToBuffer(LogHeader &header, const char *buffer) {
  if (header.logSize <= 0 || !buffer)
    return;

  int ptr = getEmptytLogPtr();

  // check buffer length
  if ((ptr < 0) || (ptr + sizeof(LogHeader) + header.logSize + 1 > LOG_SIZE)) {
    // get size that we need in buffer
    int qptr = getNextLogPtr(max(LOG_SEGMENT, (int)sizeof(LogHeader) + header.logSize + 1));
    if (qptr > 0) {
      // move buffer with 0x00 tail
      Serial1.println(header.logSize);
      Serial1.println(qptr);
      memmove(&logMem[0], &logMem[qptr], LOG_SIZE + sizeof(LogHeader) - qptr);
      ptr = getEmptytLogPtr();
      Serial1.println(ptr);
    } else {
      Serial1.println("cant get");
      // if we cant get next pointer
      return;   
    }
  }

  // double check
  if ((ptr < 0) || (ptr + sizeof(LogHeader) + header.logSize + 1 > LOG_SIZE)){ 
    Serial1.println("dbl check fail");
    Serial1.println(ptr);
    return;
  }

  // copy header
  memcpy(&logMem[ptr], &header, sizeof(LogHeader));
  ptr += sizeof(LogHeader);

  // copy buffer
  memcpy(&logMem[ptr], buffer, header.logSize);
  ptr += header.logSize;
  
  // add 0x00 at the end
  logMem[ptr] = 0x00;  
  ptr++;
  
  // fill next log entry with 0x00
  memset(&logMem[ptr], 0x00, sizeof(LogHeader));
}

void xLogger::showLog() {
  telnetClient.println(SF("\r\n\r\n******** Cached log."));

  int ptr = 0;
  String str;
  LogHeader header;

  while (ptr <= LOG_SIZE - 1) {
    memcpy(&header, &logMem[ptr], sizeof(LogHeader));
    if (header.logTime == 0 && header.logLevel == llNone) {
      break;
    }

    formatLogMessage(str, (char*)(&logMem[ptr] + sizeof(LogHeader)), header.logSize, &header);
    telnetClient.print(str);
    
    ptr += sizeof(LogHeader) + header.logSize + 1;
  }
  telnetClient.println(SF("********"));
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
        str = String(header->logTime);
        str += SF(" ");
        break;
      case ltMsBetween:
        str = String(header->logTime - oldMillis);
        str += SF(" ");
        oldMillis = header->logTime;
        break;
      case ltGMTTime:
        if (timeStatus() != timeNotSet) {
          str += NTP.getTimeStr(NTP.getLastBootTime() + header->logTime / 1000);
          str += SF(" ");
        }
        break;
      case ltNone:
      deafult:
        break;
    };
  
    // show log level
    if (showDebugLevel) {
      switch (header->logLevel) {
        case llInfo:     str += SF("INFO: "); break;
        case llWarning:  str += SF("WARNING: "); break;
        case llError:    str += SF("ERROR: "); break;
        default:         str += SF("UNKNOWN: "); break;
      }
    }
  }

  str += String(buffer);  
}

void xLogger::processLineBuffer() {
  // add end of char
  lineBuffer[lineBufferLen] = 0x00;

  // if end of line or buffer full
  if (lineBuffer[lineBufferLen - 1] != '\n' && LINE_BUFFER_LENGTH - 1 > lineBufferLen) {  
    return;
  }

  // processing...
  if (filterLogLevel <= curHeader.logLevel) { // filter here
    curHeader.logTime = millis();

    // write to buffer
    curHeader.logSize = lineBufferLen;
    addLogToBuffer(curHeader, &lineBuffer[0]);

    String msg = "";
    formatLogMessage(msg, lineBuffer, lineBufferLen, &curHeader);

    // write to serial
    if (serialEnabled && logSerial) {
      logSerial->print(msg);
    }

    // write to telnet
    if (telnetConnected && (telnetAuthenticated || !strnlen(passwd, 1)) ) { 
      telnetClient.print(msg);
    }
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


