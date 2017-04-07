

#include <Arduino.h>
#include "eastron.h"

mqttMapConfigS eastron220[eastron220Len] = {
  {"Voltage",         POLL_INPUT_REGISTERS, 0x00, MDB_INT},
  {"Current",         POLL_INPUT_REGISTERS, 0x06, MDB_INT},
  {"PowerActive",     POLL_INPUT_REGISTERS, 0x0C, MDB_INT},
  {"PowerVA",         POLL_INPUT_REGISTERS, 0x12, MDB_INT},
  {"PowerVAR",        POLL_INPUT_REGISTERS, 0x18, MDB_INT}
};
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
  {"Data",       POLL_INPUT_REGISTERS, 0x00, MDB_16BYTE_HEX},

  {"Frequency",       POLL_INPUT_REGISTERS, 0x46, MDB_FLOAT},

  {"SerialNumber",    POLL_HOLDING_REGISTERS, 0x2A, MDB_8BYTE_HEX},
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

void Eastron::ModbusSetup() {
  // Initialize modbus communication settings etc...
  //modbus_configure(SERIAL_BAUD, MODBUS_POLL_TIMEOUT, MODBUS_POLL_INTERVAL, SERIAL_RETRY_COUNT, 0, pck, getModbusDiapLength());  
  modbusNode.begin(&Serial, SERIAL_BAUD);
  
}

void Eastron::Poll(byte Command) {
  Connected = false;

  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command){
      uint8_t res = modbusNode.ModbusMasterTransaction(1, modbusArray[i].Command, modbusArray[i].StartDiap, modbusArray[i].LengthDiap, modbusArray[i].Address);
      if (res != MBSuccess) {
        // debug output error here
      }
      Connected = (res == 0);
    } else {
      break;
    }
  }
  
/*  unsigned int connection_status = modbus_update(pck);
  if (connection_status != MAX_MODBUS_DIAP)
  {
    // re-enable the connection by:
    pck[connection_status].connection = true;
  }*/
}

