
#include <Arduino.h>
#include <emodbus.h>
#include <etools.h>

static uint16_t crc16_update(uint16_t crc, uint8_t a) {
  int i;

  crc ^= a;
  for (i = 0; i < 8; ++i) {
    if (crc & 1)
      crc = (crc >> 1) ^ 0xA001;
    else
      crc = (crc >> 1);
  }

  return crc;
}

void strModbusError(String &str, int error) {
  str = SF("[0x") + String(error, HEX) + SF("] ");
  switch (error) {
    case MBSuccess:               str += SF("Success."); break;
    case MBIllegalFunction:       str += SF("Illegal Function."); break;
    case MBIllegalDataAddress:    str += SF("Illegal Data Address."); break;
    case MBIllegalDataValue:      str += SF("Illegal Data Value."); break;
    case MBSlaveDeviceFailure:    str += SF("Slave Device Failure."); break;
    case MBInvalidSlaveID:        str += SF("Invalid Slave ID."); break;
    case MBInvalidFunction:       str += SF("Invalid Func."); break;
    case MBResponseTimedOut:      str += SF("Response Timeout."); break;
    case MBInvalidCRC:            str += SF("Invalid CRC."); break;
    case MBInvalidPacketLength:   str += SF("Invalid Packet Length."); break;
    default:                      str += SF("Unknown error."); 
  }  
}

ModbusMaster::ModbusMaster(void) {
  _idle = NULL;
  _preTransmission = NULL;
  _postTransmission = NULL;

  _serial = NULL;
}

void ModbusMaster::begin(Stream *serial, int SerialSpeed)
{
  _serial = serial;

#ifdef USE_SOFTWARE_SERIAL
  static_cast<SoftwareSerial*>(serial)->begin(SerialSpeed);
#else
  static_cast<HardwareSerial*>(serial)->begin(SerialSpeed);
#endif
}

// callbacks
void ModbusMaster::idle(void (*idle)())
{
  _idle = idle;
}

void ModbusMaster::preTransmission(void (*preTransmission)())
{
  _preTransmission = preTransmission;
}

void ModbusMaster::postTransmission(void (*postTransmission)())
{
  _postTransmission = postTransmission;
}

uint8_t ModbusMaster::ModbusMasterTransaction(
    uint8_t u8MBSlave,
    uint8_t u8MBFunction, uint16_t u16ReadAddress, uint16_t u16ReadQty,
    uint8_t *_u8ReceiveBuffer, 
    uint16_t *u16TransmitBuffer, uint16_t u16WriteAddress, uint16_t u16WriteQty)
{
//  Serial1.print("Modbus poll. F="); 
//  Serial1.print(u8MBFunction);
//  Serial1.print(" Addr="); 
//  Serial1.println(u16ReadAddress);

  uint8_t u8ModbusADU[256];
  uint8_t u8ModbusADUSize = 0;
  uint8_t i, u8Qty;
  uint16_t u16CRC;
  uint32_t u32StartTime;
  uint8_t u8BytesLeft = 8;
  uint8_t u8MBStatus = MBSuccess;

  // assemble Modbus Request Application Data Unit
  u8ModbusADU[u8ModbusADUSize++] = u8MBSlave;
  u8ModbusADU[u8ModbusADUSize++] = u8MBFunction;

  switch (u8MBFunction) {
    case MBReadCoils:
    case MBReadDiscreteInputs:
    case MBReadInputRegisters:
    case MBReadHoldingRegisters:
    case MBReadWriteMultipleRegisters:
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16ReadAddress);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16ReadAddress);
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16ReadQty);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16ReadQty);
      break;
  }

  switch (u8MBFunction) {
    case MBWriteSingleCoil:
    case MBMaskWriteRegister:
    case MBWriteMultipleCoils:
    case MBWriteSingleRegister:
    case MBWriteMultipleRegisters:
    case MBReadWriteMultipleRegisters:
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16WriteAddress);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16WriteAddress);
      break;
  }

  switch (u8MBFunction) {
    case MBWriteSingleCoil:
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16WriteQty);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16WriteQty);
      break;

    case MBWriteSingleRegister:
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16TransmitBuffer[0]);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16TransmitBuffer[0]);
      break;

    case MBWriteMultipleCoils:
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16WriteQty);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16WriteQty);
      u8Qty = (u16WriteQty % 8) ? ((u16WriteQty >> 3) + 1) : (u16WriteQty >> 3);
      u8ModbusADU[u8ModbusADUSize++] = u8Qty;
      for (i = 0; i < u8Qty; i++) {
        switch (i % 2) {
          case 0: // i is even
            u8ModbusADU[u8ModbusADUSize++] = lowByte(u16TransmitBuffer[i >> 1]);
            break;

          case 1: // i is odd
            u8ModbusADU[u8ModbusADUSize++] = highByte(u16TransmitBuffer[i >> 1]);
            break;
        }
      }
      break;

    case MBWriteMultipleRegisters:
    case MBReadWriteMultipleRegisters:
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16WriteQty);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16WriteQty);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16WriteQty << 1);

      for (i = 0; i < lowByte(u16WriteQty); i++) {
        u8ModbusADU[u8ModbusADUSize++] = highByte(u16TransmitBuffer[i]);
        u8ModbusADU[u8ModbusADUSize++] = lowByte(u16TransmitBuffer[i]);
      }
      break;

    case MBMaskWriteRegister:
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16TransmitBuffer[0]);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16TransmitBuffer[0]);
      u8ModbusADU[u8ModbusADUSize++] = highByte(u16TransmitBuffer[1]);
      u8ModbusADU[u8ModbusADUSize++] = lowByte(u16TransmitBuffer[1]);
      break;
  }

  // append CRC
  u16CRC = 0xFFFF;
  for (i = 0; i < u8ModbusADUSize; i++) {
    u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
  }
  u8ModbusADU[u8ModbusADUSize++] = lowByte(u16CRC);
  u8ModbusADU[u8ModbusADUSize++] = highByte(u16CRC);
  u8ModbusADU[u8ModbusADUSize] = 0;

  // flush receive buffer before transmitting request
  while (_serial->read() != -1);

  // transmit request
  if (_preTransmission) {
    _preTransmission();
  }
  for (i = 0; i < u8ModbusADUSize; i++) {
    _serial->write(u8ModbusADU[i]);
  }

  u8ModbusADUSize = 0;
  _serial->flush();    // flush transmit buffer
  if (_postTransmission) {
    _postTransmission();
  }

  // loop until we run out of time or bytes, or an error occurs
  u32StartTime = millis();
  while (u8BytesLeft && !u8MBStatus) {
    if (_serial->available()) {
      u8ModbusADU[u8ModbusADUSize++] = _serial->read();
      u8BytesLeft--;
    } else {
      if (_idle) {
        _idle();
      }
    }

    // evaluate slave ID, function code once enough bytes have been read
    if (u8ModbusADUSize == 5) {
      // verify response is for correct Modbus slave
      if (u8ModbusADU[0] != u8MBSlave) {
        u8MBStatus = MBInvalidSlaveID;
        break;
      }

      // verify response is for correct Modbus function code (mask exception bit 7)
      if ((u8ModbusADU[1] & 0x7F) != u8MBFunction) {
        u8MBStatus = MBInvalidFunction;
        break;
      }

      // check whether Modbus exception occurred; return Modbus Exception Code
      if (bitRead(u8ModbusADU[1], 7)) {
        u8MBStatus = u8ModbusADU[2];
        break;
      }

      // evaluate returned Modbus function code
      switch (u8ModbusADU[1]) {
        case MBReadCoils:
        case MBReadDiscreteInputs:
        case MBReadInputRegisters:
        case MBReadHoldingRegisters:
        case MBReadWriteMultipleRegisters:
          u8BytesLeft = u8ModbusADU[2];
          break;

        case MBWriteSingleCoil:
        case MBWriteMultipleCoils:
        case MBWriteSingleRegister:
        case MBWriteMultipleRegisters:
          u8BytesLeft = 3;
          break;

        case MBMaskWriteRegister:
          u8BytesLeft = 5;
          break;
      }
    }
    if ((millis() - u32StartTime) > ku16MBResponseTimeout) {
      u8MBStatus = MBResponseTimedOut;
    }
  }

  // verify response is large enough to inspect further
  if (!u8MBStatus && u8ModbusADUSize >= 5) {
    // check packet length
    if (u8ModbusADU[2] != u8ModbusADUSize -3 -2) {
      u8MBStatus = MBInvalidPacketLength;     
    }
    
    // calculate CRC
    u16CRC = 0xFFFF;
    for (i = 0; i < (u8ModbusADUSize - 2); i++) {
      u16CRC = crc16_update(u16CRC, u8ModbusADU[i]);
    }

    // verify CRC
    if (!u8MBStatus && (lowByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 2] ||
                        highByte(u16CRC) != u8ModbusADU[u8ModbusADUSize - 1])) {
      u8MBStatus = MBInvalidCRC;
    }
  }

  if (u8MBStatus == MBSuccess) {
    memcpy(_u8ReceiveBuffer, &u8ModbusADU[3], u8ModbusADU[2]);
  }

  /*
  // disassemble ADU into words
  if (!u8MBStatus)
    {
    // evaluate returned Modbus function code
    switch(u8ModbusADU[1])
    {
      case ku8MBReadCoils:
      case ku8MBReadDiscreteInputs:
        // load bytes into word; response bytes are ordered L, H, L, H, ...
        for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
        {
          if (i < ku8MaxBufferSize)
          {
            _u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 4], u8ModbusADU[2 * i + 3]);
          }

          _u8ResponseBufferLength = i;
        }

        // in the event of an odd number of bytes, load last byte into zero-padded word
        if (u8ModbusADU[2] % 2)
        {
          if (i < ku8MaxBufferSize)
          {
            _u16ResponseBuffer[i] = word(0, u8ModbusADU[2 * i + 3]);
          }

          _u8ResponseBufferLength = i + 1;
        }
        break;

      case ku8MBReadInputRegisters:
      case ku8MBReadHoldingRegisters:
      case ku8MBReadWriteMultipleRegisters:
        // load bytes into word; response bytes are ordered H, L, H, L, ...
        for (i = 0; i < (u8ModbusADU[2] >> 1); i++)
        {
          if (i < ku8MaxBufferSize)
          {
            _u16ResponseBuffer[i] = word(u8ModbusADU[2 * i + 3], u8ModbusADU[2 * i + 4]);
          }

          _u8ResponseBufferLength = i;
        }
        break;
    }
    }*/

  return u8MBStatus;
}
