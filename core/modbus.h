#ifndef MODBUSLIB_H
#define MODBUSLIB_H
#include "manager.h"
#include <cstdint>

#define SLAVE_ID 10

class ModbusClient
{
public:
    int ReadInputRegisters(uint16_t *buf, uint16_t startAdress, uint16_t NbReg, int timeout);
    int ReadHoldingRegisters(uint16_t *buf, uint16_t startAdress, uint16_t NbReg, int timeout);
    int ReadDiscrete(uint8_t *buf, uint16_t startAdress, uint16_t NbDis, int timeout);
    int ReadCoils(uint8_t *buf, uint16_t startAdress, uint16_t NbCoils, int timeout);

    int WriteHoldingRegisters(uint16_t *buf, uint16_t startAdress, uint16_t NbRegs, int timeout);
    int WriteCoils(uint8_t *buf, uint16_t startAdress, uint16_t NbCoils, int timeout);

    ModbusClient();
    ~ModbusClient();
    Manager *mngr;
};
#endif
