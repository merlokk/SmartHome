/*
 * Eastron bridge. 
 * Universal eastron reader.
 * 
 * (c) Oleg Moiseenko 2017
 */
 
#ifndef __MODBUSPOLL_H__
#define __MODBUSPOLL_H__

#include <Arduino.h>
#include <etools.h>
#include <emodbus.h>
#include <xlogger.h>        // logger https://github.com/merlokk/xlogger

#define MODBUSPOLL_DEBUG

#define SERIAL_BAUD                 9600       // baudrate
#define MODBUS_POLL_TIMEOUT         700        // max time to wait for response from SDM

// Poll commands
#define POLL_ALL                    0
#define POLL_HOLDING_REGISTERS      MBReadHoldingRegisters
#define POLL_INPUT_REGISTERS        MBReadInputRegisters

// registers configuration
#define MDB_WORD         1
#define MDB_INT          2
#define MDB_INT64        3
#define MDB_FLOAT        4
#define MDB_FLOAT8       5
#define MDB_2BYTE_HEX    6
#define MDB_4BYTE_HEX    7
#define MDB_INT64_HEX    8
#define MDB_8BYTE_HEX    9
#define MDB_16BYTE_HEX  10
struct mqttMapConfigS {
  const char * mqttTopicName;
  byte command;
  word modbusAddress;
  byte valueType;
};

#define MAX_MODBUS_DIAP 20
typedef struct {
  byte Command = 0; 
  word StartDiap = 0; 
  word LengthDiap = 0;
  uint8_t* Address = NULL;
} ModbusDiap; 

class ModbusPoll {
  private:
    ModbusDiap modbusArray[MAX_MODBUS_DIAP];
    ModbusMaster modbusNode;
    xLogger * logger = NULL;
    Stream * mSerial = &Serial;
    uint8_t deviceAddress = 1;
    uint32_t sleepBetweenPolls = 0;
  public:
    uint8_t* getValueAddress(byte Command, word ModbusAddress);
  
    bool Connected = false;
    const mqttMapConfigS *mapConfig;
    int mapConfigLen;
  
    ModbusPoll(uint8_t _deviceAddress = 1);
    void SetLogger(xLogger * _logger);
    void SetSerial(Stream * _serial);
    void SetDeviceAddress(uint8_t _deviceAddress);
    void SetSleepBetweenPolls(uint32_t _sleep);
    int AddModbusDiap(byte Command, word StartDiap, word LengthDiap);
    int getModbusDiapLength();
    void getStrModbusConfig(String &str);
    void ModbusSetup(const char *deviceType);
    void Connect();
    void Poll(byte Command);
    void PollAddress(word ModbusAddress);

    uint16_t getWordValue(byte Command, word ModbusAddress);
    void setWordValue(uint16_t value, byte Command, word ModbusAddress);
    uint32_t getIntValue(byte Command, word ModbusAddress);
    uint64_t getInt64Value(byte Command, word ModbusAddress);
    float getFloatValue(byte Command, word ModbusAddress);
    void getMemoryHex(String &str, byte Command, word ModbusAddress, int len);
    void getValue(String &str, byte Command, word ModbusAddress, byte valueType);

    template <typename... Args>
    void DEBUG_PRINTLN(Args... args);
};

#endif // ifndef __MODBUSPOLL_H__


