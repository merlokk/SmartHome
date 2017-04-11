

#include <Arduino.h>
#include "eastron.h"

// devices configuration
#define eastron220Len 5
mqttMapConfigS eastron220[eastron220Len] = {
  {"Voltage",         POLL_INPUT_REGISTERS, 0x00, MDB_FLOAT},
  {"Current",         POLL_INPUT_REGISTERS, 0x06, MDB_FLOAT},
  {"PowerActive",     POLL_INPUT_REGISTERS, 0x0C, MDB_FLOAT},
  {"PowerVA",         POLL_INPUT_REGISTERS, 0x12, MDB_FLOAT},
  {"PowerVAR",        POLL_INPUT_REGISTERS, 0x18, MDB_FLOAT}
};

#define eastron630smallLen 18
mqttMapConfigS eastron630small[eastron630smallLen] = {
  {"Voltage1",        POLL_INPUT_REGISTERS, 0x00, MDB_FLOAT},
  {"Voltage2",        POLL_INPUT_REGISTERS, 0x02, MDB_FLOAT},
  {"Voltage3",        POLL_INPUT_REGISTERS, 0x04, MDB_FLOAT},
  {"Current1",        POLL_INPUT_REGISTERS, 0x06, MDB_FLOAT},
  {"Current2",        POLL_INPUT_REGISTERS, 0x08, MDB_FLOAT},
  {"Current3",        POLL_INPUT_REGISTERS, 0x0A, MDB_FLOAT},
  {"PowerActive1",    POLL_INPUT_REGISTERS, 0x0C, MDB_FLOAT},
  {"PowerActive2",    POLL_INPUT_REGISTERS, 0x0E, MDB_FLOAT},
  {"PowerActive3",    POLL_INPUT_REGISTERS, 0x10, MDB_FLOAT},
  {"PowerVA1",        POLL_INPUT_REGISTERS, 0x12, MDB_FLOAT},
  {"PowerVA2",        POLL_INPUT_REGISTERS, 0x14, MDB_FLOAT},
  {"PowerVA3",        POLL_INPUT_REGISTERS, 0x16, MDB_FLOAT},
  {"PowerVAR1",       POLL_INPUT_REGISTERS, 0x18, MDB_FLOAT},
  {"PowerVAR2",       POLL_INPUT_REGISTERS, 0x1A, MDB_FLOAT},
  {"PowerVAR3",       POLL_INPUT_REGISTERS, 0x1C, MDB_FLOAT},
  {"NeutralCurrent",  POLL_INPUT_REGISTERS, 0xE0, MDB_FLOAT},
//  {"Data",       POLL_INPUT_REGISTERS, 0x00, MDB_16BYTE_HEX}, // debug only

  {"Frequency",       POLL_INPUT_REGISTERS, 0x46, MDB_FLOAT},

  {"SerialNumber",    POLL_HOLDING_REGISTERS, 0x2A, MDB_8BYTE_HEX},
};

#define eastron630Len 69
mqttMapConfigS eastron630[eastron630Len] = {
  {"Voltage1",        POLL_INPUT_REGISTERS, 0x00, MDB_FLOAT},
  {"Voltage2",        POLL_INPUT_REGISTERS, 0x02, MDB_FLOAT},
  {"Voltage3",        POLL_INPUT_REGISTERS, 0x04, MDB_FLOAT},
  {"Current1",        POLL_INPUT_REGISTERS, 0x06, MDB_FLOAT},
  {"Current2",        POLL_INPUT_REGISTERS, 0x08, MDB_FLOAT},
  {"Current3",        POLL_INPUT_REGISTERS, 0x0A, MDB_FLOAT},
  {"PowerActive1",    POLL_INPUT_REGISTERS, 0x0C, MDB_FLOAT},
  {"PowerActive2",    POLL_INPUT_REGISTERS, 0x0E, MDB_FLOAT},
  {"PowerActive3",    POLL_INPUT_REGISTERS, 0x10, MDB_FLOAT},
  {"PowerVA1",        POLL_INPUT_REGISTERS, 0x12, MDB_FLOAT},
  {"PowerVA2",        POLL_INPUT_REGISTERS, 0x14, MDB_FLOAT},
  {"PowerVA3",        POLL_INPUT_REGISTERS, 0x16, MDB_FLOAT},
  {"PowerVAR1",       POLL_INPUT_REGISTERS, 0x18, MDB_FLOAT},
  {"PowerVAR2",       POLL_INPUT_REGISTERS, 0x1A, MDB_FLOAT},
  {"PowerVAR3",       POLL_INPUT_REGISTERS, 0x1C, MDB_FLOAT},
  {"PowerFactor1",    POLL_INPUT_REGISTERS, 0x1E, MDB_FLOAT},
  {"PowerFactor2",    POLL_INPUT_REGISTERS, 0x20, MDB_FLOAT},
  {"PowerFactor3",    POLL_INPUT_REGISTERS, 0x22, MDB_FLOAT},
  {"PhaseAngle1",     POLL_INPUT_REGISTERS, 0x24, MDB_FLOAT},
  {"PhaseAngle2",     POLL_INPUT_REGISTERS, 0x26, MDB_FLOAT},
  {"PhaseAngle3",     POLL_INPUT_REGISTERS, 0x28, MDB_FLOAT},
  {"VoltageAvg",      POLL_INPUT_REGISTERS, 0x2A, MDB_FLOAT},
  {"CurrentAvg",      POLL_INPUT_REGISTERS, 0x2E, MDB_FLOAT},
  {"CurrentSum",      POLL_INPUT_REGISTERS, 0x30, MDB_FLOAT},
  {"PowerActiveSum",  POLL_INPUT_REGISTERS, 0x34, MDB_FLOAT},
  {"PowerVASum",      POLL_INPUT_REGISTERS, 0x38, MDB_FLOAT},
  {"PowerVARSum",     POLL_INPUT_REGISTERS, 0x3C, MDB_FLOAT},
  {"PowerFactorSum",  POLL_INPUT_REGISTERS, 0x3E, MDB_FLOAT},
  {"PhaseAngleTotal", POLL_INPUT_REGISTERS, 0x42, MDB_FLOAT},

  {"Frequency",             POLL_INPUT_REGISTERS, 0x46, MDB_FLOAT},
  {"ImportWhLastReset",     POLL_INPUT_REGISTERS, 0x48, MDB_FLOAT},
  {"ExportWhLastReset",     POLL_INPUT_REGISTERS, 0x4A, MDB_FLOAT},
  {"ImportVAhLastReset",    POLL_INPUT_REGISTERS, 0x5C, MDB_FLOAT},
  {"ExportVAhLastReset",    POLL_INPUT_REGISTERS, 0x4E, MDB_FLOAT},
  {"VAhLastReset",          POLL_INPUT_REGISTERS, 0x50, MDB_FLOAT},
  {"AhLastReset",           POLL_INPUT_REGISTERS, 0x54, MDB_FLOAT},

  {"PowerTotalDem",         POLL_INPUT_REGISTERS, 0x54, MDB_FLOAT},
  {"MaxPowerTotalDem",      POLL_INPUT_REGISTERS, 0x56, MDB_FLOAT},
  // hole
  {"PowerVATotalDem",       POLL_INPUT_REGISTERS, 0x64, MDB_FLOAT},
  {"MaxPowerVATotalDem",    POLL_INPUT_REGISTERS, 0x66, MDB_FLOAT},
  {"NeutralCurrentDem",     POLL_INPUT_REGISTERS, 0x68, MDB_FLOAT},
  {"MaxNeutralCurrentDem",  POLL_INPUT_REGISTERS, 0x6A, MDB_FLOAT},

  {"Line1to2Volts",         POLL_INPUT_REGISTERS, 0xC8, MDB_FLOAT},
  {"Line2to3Volts",         POLL_INPUT_REGISTERS, 0xCA, MDB_FLOAT},
  {"Line3to1Volts",         POLL_INPUT_REGISTERS, 0xCC, MDB_FLOAT},
  {"AvgLineToLineVolts",    POLL_INPUT_REGISTERS, 0xCE, MDB_FLOAT},
  {"NeutralCurrent",        POLL_INPUT_REGISTERS, 0xE0, MDB_FLOAT},

  {"Phase1LNVoltsTHD",      POLL_INPUT_REGISTERS, 0xEA, MDB_FLOAT},
  {"Phase2LNVoltsTHD",      POLL_INPUT_REGISTERS, 0xEC, MDB_FLOAT},
  {"Phase3LNVoltsTHD",      POLL_INPUT_REGISTERS, 0xEE, MDB_FLOAT},
  {"Phase1CurrentTHD",      POLL_INPUT_REGISTERS, 0xF0, MDB_FLOAT},
  {"Phase2CurrentTHD",      POLL_INPUT_REGISTERS, 0xF2, MDB_FLOAT},
  {"Phase3CurrentTHD",      POLL_INPUT_REGISTERS, 0xF4, MDB_FLOAT},
  {"AvgVoltageTHD",         POLL_INPUT_REGISTERS, 0xF8, MDB_FLOAT},
  {"AvgCurrentTHD",         POLL_INPUT_REGISTERS, 0xFA, MDB_FLOAT},
  
  {"PowerFactorTotal",      POLL_INPUT_REGISTERS, 0xFE, MDB_FLOAT},
  {"Phase1CurrentDem",      POLL_INPUT_REGISTERS, 0x0102, MDB_FLOAT},
  {"Phase2CurrentDem",      POLL_INPUT_REGISTERS, 0x0104, MDB_FLOAT},
  {"Phase3CurrentDem",      POLL_INPUT_REGISTERS, 0x0106, MDB_FLOAT},
  {"MaxPhase1CurrentDem",   POLL_INPUT_REGISTERS, 0x0108, MDB_FLOAT},
  {"MaxPhase2CurrentDem",   POLL_INPUT_REGISTERS, 0x010A, MDB_FLOAT},
  {"MaxPhase3CurrentDem",   POLL_INPUT_REGISTERS, 0x010C, MDB_FLOAT},
  // hole
  {"Line1to2VoltsTHD",      POLL_INPUT_REGISTERS, 0x014E, MDB_FLOAT},
  {"Line2to3VoltsTHD",      POLL_INPUT_REGISTERS, 0x0150, MDB_FLOAT},
  {"Line3to1VoltsTHD",      POLL_INPUT_REGISTERS, 0x0152, MDB_FLOAT},
  {"AvgLineToLineVoltsTHD", POLL_INPUT_REGISTERS, 0x0154, MDB_FLOAT},

  // Holding registers 
  {"SystemVoltage",   POLL_HOLDING_REGISTERS, 0x06, MDB_FLOAT},
  {"SystemType",      POLL_HOLDING_REGISTERS, 0x08, MDB_FLOAT},
  {"SerialNumber",    POLL_HOLDING_REGISTERS, 0x2A, MDB_8BYTE_HEX}
};

Eastron::Eastron() {
}

int Eastron::AddModbusDiap(byte Command, word StartDiap, word LengthDiap) {
  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command == 0){
      modbusArray[i].Command = Command;
      modbusArray[i].StartDiap = StartDiap;
      modbusArray[i].LengthDiap = LengthDiap;
      modbusArray[i].Address = new uint8_t[modbusArray[i].LengthDiap * sizeof(uint16_t) + 4]; // +4 - because of data alignment
      memset(modbusArray[i].Address, 0x00, modbusArray[i].LengthDiap * sizeof(uint16_t));
     
      return 0;
    }
  }
  return -1;
}

int Eastron::getModbusDiapLength() {
  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command == 0){
      return i;
    }
  }
  return MAX_MODBUS_DIAP;
}

void Eastron::getStrModbusConfig(String &str) {
  str = "Max length: " + String(MAX_MODBUS_DIAP) + " Length: " + String(getModbusDiapLength());
  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command){
      str += "\n" + String(i);
      str += ": Cmd:" + String(modbusArray[i].Command);
      str += " Start:" + String(modbusArray[i].StartDiap);
      str += " Len:" + String(modbusArray[i].LengthDiap);
      str += " Addr:" + String((int)modbusArray[i].Address);
    }
  }
}

uint8_t* Eastron::getValueAddress(byte Command, word ModbusAddress) {
  if (!Command) {
    return NULL;
  }
  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command == Command &&
        modbusArray[i].StartDiap <= ModbusAddress &&
        modbusArray[i].StartDiap + modbusArray[i].LengthDiap - 1 >= ModbusAddress){
      return modbusArray[i].Address + (ModbusAddress - modbusArray[i].StartDiap) * 2; 
    }
  }

  return NULL;
}

uint16_t Eastron::getWordValue(byte Command, word ModbusAddress){
  uint8_t *ptr = getValueAddress(Command, ModbusAddress);
  if (ptr == NULL) {
    return 0;
  }
  uint16_t w;
  memcpy(&w, ptr, 2);
  return w;  
}

void Eastron::setWordValue(uint16_t value, byte Command, word ModbusAddress) {
  uint8_t *ptr = getValueAddress(Command, ModbusAddress);
  if (ptr == NULL) {
    return;
  }
  memcpy(ptr, &value, 2); 
}

uint32_t Eastron::getIntValue(byte Command, word ModbusAddress) {
  uint8_t *ptr = getValueAddress(Command, ModbusAddress);
  if (ptr == NULL) {
    return 0;
  }

  uint32_t i;
  memcpy(&i, ptr, 4);
  return i;  
}

uint64_t Eastron::getInt64Value(byte Command, word ModbusAddress) {
  uint8_t *ptr = getValueAddress(Command, ModbusAddress);
  if (ptr == NULL) {
    return 0;
  }

  uint64_t li;
  memcpy(&li, ptr, 8);
  return li;  
}

float Eastron::getFloatValue(byte Command, word ModbusAddress) {
  uint8_t *ptr = getValueAddress(Command, ModbusAddress);
  if (ptr == NULL) {
    return 0;
  }

  dataFloat df;
  memcpy(&df.arr[0], ptr + 3, 1);
  memcpy(&df.arr[1], ptr + 2, 1);
  memcpy(&df.arr[2], ptr + 1, 1);
  memcpy(&df.arr[3], ptr + 0, 1);
  return df.f;  
}

void Eastron::getMemoryHex(String &str, byte Command, word ModbusAddress, int len) {
  str = "";
  uint8_t *ptr = getValueAddress(Command, ModbusAddress);
  if (ptr == NULL) {
    return;
  }

  uint8_t b;
  for(int i=0; i < len; i++) {
    memcpy(&b, ptr + i, 1);
    if (b < 0x10) {
      str += "0";
    }      
    str += String(b, HEX) + " ";
  }
}

void Eastron::getValue(String &str, byte Command, word ModbusAddress, byte valueType) {
  switch (valueType) {
    case MDB_WORD:  
      str = String(getWordValue(Command, ModbusAddress));
      break;
    case MDB_INT:   
      str = String(getIntValue(Command, ModbusAddress));
      break;
    case MDB_INT64:   
      char c[10];
      sprintf(c, "%I64d", getInt64Value(Command, ModbusAddress)); // "%llu", (unsigned long long) tr
      str = String(c);
      break;
    case MDB_FLOAT:   
      str = String(getFloatValue(Command, ModbusAddress));
      break;
    case MDB_8BYTE_HEX:   
      getMemoryHex(str, Command, ModbusAddress, 8);
      break;
    case MDB_16BYTE_HEX:   
      getMemoryHex(str, Command, ModbusAddress, 16);
      break;
    default: str = "---";
  }
}

void Eastron::Connect() {
  // Initialize modbus communication settings etc...
  modbusNode.begin(&Serial, SERIAL_BAUD);
}

void Eastron::ModbusSetup(char *deviceType) {
  mapConfig = NULL;
  mapConfigLen = 0;

  // eastron 220, 230
  if (strncmp(deviceType, "220", 5) == 0 || 
      strncmp(deviceType, "230", 5) == 0) {
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x001, 0x1E);   // 1-37
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x046, 0x12);   // 71-79
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x156, 0x04);   // 343-345
    AddModbusDiap(POLL_HOLDING_REGISTERS, 0x043, 0x04); // serial number*/

    mapConfig = eastron220;
    mapConfigLen = eastron220Len;
  }

  // eastron 630 small
  if (strncmp(deviceType, "630s", 5) == 0) {
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x000, 0x1D); 
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x046, 0x02); 
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x0E0, 0x02); 
    AddModbusDiap(POLL_HOLDING_REGISTERS, 0x02A, 0x04); // serial number

    mapConfig = eastron630small;
    mapConfigLen = eastron630smallLen;
  }
  
  // eastron 630 full
  if (strncmp(deviceType, "630", 5) == 0) {
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x000, 0x60); // 1-87      = 88 registers
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x064, 0x08); // 101-107   = 8 registers
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x0C8, 0x4E); // 201-269   = 70 registers
    AddModbusDiap(POLL_INPUT_REGISTERS, 0x14E, 0x4E); // 335-341   = 8 registers
    AddModbusDiap(POLL_HOLDING_REGISTERS, 0x006, 0x04); // voltage and system type
    AddModbusDiap(POLL_HOLDING_REGISTERS, 0x02A, 0x04); // serial number*/

    mapConfig = eastron630;
    mapConfigLen = eastron630Len;
  }
  

  Connect();
}

void Eastron::Poll(byte Command) {
  Connected = false;

  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command){
      uint8_t res = modbusNode.ModbusMasterTransaction(1, modbusArray[i].Command, modbusArray[i].StartDiap, modbusArray[i].LengthDiap, modbusArray[i].Address);
      if (res != MBSuccess) {
        // debug output error here
        Serial1.print("ERROR: modbus poll error: ");
        Serial1.println(res);
      }
      Connected = (res == 0);
    } else {
      break;
    }
  }
}

