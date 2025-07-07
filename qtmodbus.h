#ifndef QTMODBUS_H
#define QTMODBUS_H
#include "errno.h"
#include "modbus/modbus.h"
#define COILS_START 00001
#define DISCRETE_START 10001
#define INPUT_REG_START 31001
#define HOLDING_REG_START 40001
#define MAX_ADDR 49999

//enum MODBUS_ADU_TYPE { ASCII, RTU, TCP };

class qtmodbus
{
private:
    int adu_type;
    modbus_t *ctx;

public:
    qtmodbus(int adu_type);
    int COM_Init(const char *device, int baud_rate, char polarity, char dataBits, char stopBits);
    int TCP_Init(const char *ip, int port);
    int Connect();
    void Close();
    int SetSlave(int adr);
    const char *GetErrMsg(int err);

    int WriteHoldingRegisters(int addr, int nb, uint16_t *src);
    int ReadHoldingRegisters(int addr, int nb, uint16_t *dest);

    int ReadInputRegisters(int addr, int nb, uint16_t *dest);

    int ReadCoils(int addr, int nb, uint8_t *dest);
    int WriteCoils(int addr, int nb, uint8_t *src);

    int ReadDiscrete(int addr, int nb, uint8_t *dest);
};

#endif // QTMODBUS_H
