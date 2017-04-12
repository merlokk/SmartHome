/*
 * Modbus RTU library.
 * It works via HardwareSerial. ESP-8266 tested.
 * 
 * (c) Oleg Moiseenko 2017
 */

#ifndef __EMODBUS_H__
#define __EMODBUS_H__

#include <Arduino.h>

enum ModbusError {
  MBSuccess                    = 0x00,
  MBIllegalFunction            = 0x01,
  MBIllegalDataAddress         = 0x02,
  MBIllegalDataValue           = 0x03,
  MBSlaveDeviceFailure         = 0x04,
  MBInvalidSlaveID             = 0xE0,
  MBInvalidFunction            = 0xE1,
  MBResponseTimedOut           = 0xE2,
  MBInvalidCRC                 = 0xE3,
  MBInvalidPacketLength        = 0xE4
};
void strModbusError(String &str, int error);

enum ModbusFunctions {
  // Modbus function codes for bit access
  MBReadCoils                  = 0x01, // Modbus function 0x01 Read Coils
  MBReadDiscreteInputs         = 0x02, // Modbus function 0x02 Read Discrete Inputs
  MBWriteSingleCoil            = 0x05, // Modbus function 0x05 Write Single Coil
  MBWriteMultipleCoils         = 0x0F, // Modbus function 0x0F Write Multiple Coils

  // Modbus function codes for 16 bit access
  MBReadHoldingRegisters       = 0x03, // Modbus function 0x03 Read Holding Registers
  MBReadInputRegisters         = 0x04, // Modbus function 0x04 Read Input Registers
  MBWriteSingleRegister        = 0x06, // Modbus function 0x06 Write Single Register
  MBWriteMultipleRegisters     = 0x10, // Modbus function 0x10 Write Multiple Registers
  MBMaskWriteRegister          = 0x16, // Modbus function 0x16 Mask Write Register
  MBReadWriteMultipleRegisters = 0x17  // Modbus function 0x17 Read Write Multiple Registers
};

typedef void (*callback)();

class ModbusMaster {
  public:
    ModbusMaster();

    //init
    void begin(HardwareSerial *serial, int SerialSpeed);
    //callbacks
    void idle(callback);
    void preTransmission(callback);
    void postTransmission(callback);

    HardwareSerial *_serial;                                     // reference to serial port object
    static const uint16_t ku16MBResponseTimeout          = 2000; // Modbus timeout [milliseconds]

    // master function that conducts Modbus transactions
    uint8_t ModbusMasterTransaction(
      uint8_t u8MBSlave,
      uint8_t u8MBFunction, uint16_t u16ReadAddress, uint16_t u16ReadQty,
      uint8_t *u8ReceiveBuffer,
      uint16_t *u16TransmitBuffer, uint16_t u16WriteAddress, uint16_t u16WriteQty);

    uint8_t ModbusMasterTransaction(
      uint8_t u8MBSlave,
      uint8_t u8MBFunction, uint16_t u16ReadAddress, uint16_t u16ReadQty,
      uint8_t *_u8ReceiveBuffer);
private:
    // idle callback function; gets called during idle time between TX and RX
    callback _idle;
    // preTransmission callback function; gets called before writing a Modbus message
    callback _preTransmission;
    // postTransmission callback function; gets called after a Modbus message has been sent
    callback _postTransmission;
};


#endif // ifndef __EMODBUS_H__
