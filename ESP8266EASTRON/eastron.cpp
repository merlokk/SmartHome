

#include <Arduino.h>
#include "eastron.h"

mqttMapConfigS eastron630small[eastron630smallLen] = {
  {"Voltage1",        POLL_INPUT_REGISTERS, 0x01, MDB_INT},
  {"Voltage2",        POLL_INPUT_REGISTERS, 0x02, MDB_INT},
  {"Voltage3",        POLL_INPUT_REGISTERS, 0x04, MDB_INT},
  {"Current1",        POLL_INPUT_REGISTERS, 0x06, MDB_INT},
  {"Current2",        POLL_INPUT_REGISTERS, 0x08, MDB_INT},
  {"Current3",        POLL_INPUT_REGISTERS, 0x0A, MDB_INT},
  {"PowerActive1",    POLL_INPUT_REGISTERS, 0x0C, MDB_INT},
  {"PowerActive2",    POLL_INPUT_REGISTERS, 0x0E, MDB_INT},
  {"PowerActive3",    POLL_INPUT_REGISTERS, 0x10, MDB_INT},
  {"PowerVA1",        POLL_INPUT_REGISTERS, 0x12, MDB_INT},
  {"PowerVA2",        POLL_INPUT_REGISTERS, 0x14, MDB_INT},
  {"PowerVA3",        POLL_INPUT_REGISTERS, 0x16, MDB_INT},
  {"PowerVAR1",       POLL_INPUT_REGISTERS, 0x18, MDB_INT},
  {"PowerVAR2",       POLL_INPUT_REGISTERS, 0x1A, MDB_INT},
  {"PowerVAR3",       POLL_INPUT_REGISTERS, 0x1C, MDB_INT}
};

Eastron::Eastron() {
  if (SWAPHWSERIAL)
    ser.swap();
  ser.begin(SDM_BAUD);
  
}

void Eastron::Poll(byte Command) {
  Connected = false;


/*      while (sdmSer.available() > 0)  {                                         //read serial if any old data is available
        sdmSer.read();
      }

      sdmSer.write(sdmarr, FRAMESIZE - 1);                                      //send 8 bytes

      sdmSer.flush();         

      resptime = millis() + MAX_MILLIS_TO_WAIT;

      while (sdmSer.available() < FRAMESIZE)  {
        if (resptime >= millis()) {
          delay(1);
        } else {
          timeouterr = true;
          break;
        }

  */
}

int Eastron::AddModbusDiap(byte Command, word StartDiap, word LengthDiap) {
  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command == 0){
      modbusArray[i].Command = Command;
      modbusArray[i].StartDiap = StartDiap;
      modbusArray[i].LengthDiap = LengthDiap;
      modbusArray[i].Address = (int*) new word[modbusArray[i].LengthDiap];
      
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

int* Eastron::getValueAddress(byte Command, word ModbusAddress) {
  for (int i = 0; i < MAX_MODBUS_DIAP; i++) {
    if (modbusArray[i].Command == Command &&
        modbusArray[i].StartDiap >= ModbusAddress &&
        modbusArray[i].StartDiap + modbusArray[i].LengthDiap <= ModbusAddress){
      return modbusArray[i].Address - modbusArray[i].StartDiap + ModbusAddress;
    }
  }

  return NULL;
}

word Eastron::getWordValue(byte Command, word ModbusAddress){
  word *w = (word*)getValueAddress(Command, ModbusAddress);
  if (*w == NULL) {
    return 0;
  }
  return *w;  
}

int Eastron::getIntValue(byte Command, word ModbusAddress) {
  int *i = getValueAddress(Command, ModbusAddress);
  if (*i == NULL) {
    return 0;
  }
  return *i;  
}

void Eastron::getValue(String &str, byte Command, word ModbusAddress, byte valueType) {
  switch (valueType) {
    case MDB_WORD:  
      str = String(getWordValue(Command, ModbusAddress));
      break;
    case MDB_INT:   
      str = String(getIntValue(Command, ModbusAddress));
      break;
    default: str = "---";
  }
}





