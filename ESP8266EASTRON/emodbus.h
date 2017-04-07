/*
 * Modbus RTU library.
 * It works via HardwareSerial. ESP-8266 tested.
 * 
 * (c) Oleg Moiseenko 2017
 */

#ifndef __EMODBUS_H__
#define __EMODBUS_H__

#include <Arduino.h>

// Modbus errors
#define MBSuccess                    0x00
#define MBIllegalFunction            0x01
#define MBIllegalDataAddress         0x02
#define MBIllegalDataValue           0x03
#define MBSlaveDeviceFailure         0x04
#define MBInvalidSlaveID             0xE0
#define MBInvalidFunction            0xE1
#define MBResponseTimedOut           0xE2
#define MBInvalidCRC                 0xE3
#define MBInvalidPacketLength        0xE4

// Modbus function codes for bit access
#define MBReadCoils                   0x01 // Modbus function 0x01 Read Coils
#define MBReadDiscreteInputs          0x02 // Modbus function 0x02 Read Discrete Inputs
#define MBWriteSingleCoil             0x05 // Modbus function 0x05 Write Single Coil
#define MBWriteMultipleCoils          0x0F // Modbus function 0x0F Write Multiple Coils

// Modbus function codes for 16 bit access
#define MBReadHoldingRegisters        0x03 // Modbus function 0x03 Read Holding Registers
#define MBReadInputRegisters          0x04 // Modbus function 0x04 Read Input Registers
#define MBWriteSingleRegister         0x06 // Modbus function 0x06 Write Single Register
#define MBWriteMultipleRegisters      0x10 // Modbus function 0x10 Write Multiple Registers
#define MBMaskWriteRegister           0x16 // Modbus function 0x16 Mask Write Register
#define MBReadWriteMultipleRegisters  0x17 // Modbus function 0x17 Read Write Multiple Registers


class ModbusMaster {
  public:
    ModbusMaster();

    //init
    void begin(HardwareSerial *serial, int SerialSpeed);
    //callbacks
    void idle(void (*)());
    void preTransmission(void (*)());
    void postTransmission(void (*)());

    HardwareSerial *_serial = NULL;                              ///< reference to serial port object
    static const uint16_t ku16MBResponseTimeout          = 2000; ///< Modbus timeout [milliseconds]

    
    static const uint8_t ku8MBIllegalFunction            = 0x01;
    static const uint8_t ku8MBIllegalDataAddress         = 0x02;
    static const uint8_t ku8MBIllegalDataValue           = 0x03;
     static const uint8_t ku8MBSlaveDeviceFailure         = 0x04;
    static const uint8_t ku8MBSuccess                    = 0x00;
     static const uint8_t ku8MBInvalidSlaveID             = 0xE0;
    static const uint8_t ku8MBInvalidFunction            = 0xE1;
    static const uint8_t ku8MBResponseTimedOut           = 0xE2;
    static const uint8_t ku8MBInvalidCRC                 = 0xE3;
    // Modbus function codes for bit access
    static const uint8_t ku8MBReadCoils                  = 0x01; ///< Modbus function 0x01 Read Coils
    static const uint8_t ku8MBReadDiscreteInputs         = 0x02; ///< Modbus function 0x02 Read Discrete Inputs
    static const uint8_t ku8MBWriteSingleCoil            = 0x05; ///< Modbus function 0x05 Write Single Coil
    static const uint8_t ku8MBWriteMultipleCoils         = 0x0F; ///< Modbus function 0x0F Write Multiple Coils

    // Modbus function codes for 16 bit access
    static const uint8_t ku8MBReadHoldingRegisters       = 0x03; ///< Modbus function 0x03 Read Holding Registers
    static const uint8_t ku8MBReadInputRegisters         = 0x04; ///< Modbus function 0x04 Read Input Registers
    static const uint8_t ku8MBWriteSingleRegister        = 0x06; ///< Modbus function 0x06 Write Single Register
    static const uint8_t ku8MBWriteMultipleRegisters     = 0x10; ///< Modbus function 0x10 Write Multiple Registers
    static const uint8_t ku8MBMaskWriteRegister          = 0x16; ///< Modbus function 0x16 Mask Write Register
    static const uint8_t ku8MBReadWriteMultipleRegisters = 0x17; ///< Modbus function 0x17 Read Write Multiple Registers
    
    // master function that conducts Modbus transactions
    uint8_t ModbusMasterTransaction(
      uint8_t _u8MBSlave, 
      uint8_t u8MBFunction, uint16_t _u16ReadAddress, uint16_t _u16ReadQty,
      uint8_t *_u8ReceiveBuffer, 
      uint16_t *_u16TransmitBuffer, uint16_t _u16WriteQty);

  private:
    // idle callback function; gets called during idle time between TX and RX
    void (*_idle)();
    // preTransmission callback function; gets called before writing a Modbus message
    void (*_preTransmission)();
    // postTransmission callback function; gets called after a Modbus message has been sent
    void (*_postTransmission)();
};


#endif // ifndef __EMODBUS_H__
