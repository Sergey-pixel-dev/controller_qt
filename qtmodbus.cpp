#include "qtmodbus.h"

qtmodbus::qtmodbus(int adu_type)
{
    this->adu_type = adu_type;
    this->ctx = NULL;
}

int qtmodbus::COM_Init(
    const char *device, int baud_rate, char polarity, char data_bits, char stop_bits)
{
    errno = 0;
    if (this->adu_type != 0 && this->adu_type != 1) {
        return 1;
    }
    this->ctx = modbus_new_rtu(device, baud_rate, polarity, data_bits, stop_bits);
    if (ctx == NULL) {
        return errno;
    }
    return 0;
}

int qtmodbus::TCP_Init(const char *ip, int port)
{
    errno = 0;
    if (this->adu_type != 2)
        return 1;

    this->ctx = modbus_new_tcp(ip, port);
    if (ctx == NULL)
        return 2;
    return 0;
}

int qtmodbus::ReadInputRegisters(int addr, int nb, uint16_t *dest)
{
    return modbus_read_input_registers(this->ctx, addr, nb, dest);
}

int qtmodbus::WriteHoldingRegisters(int addr, int nb, uint16_t *src)
{
    return modbus_write_registers(this->ctx, addr, nb, src);
}

int qtmodbus::ReadHoldingRegisters(int addr, int nb, uint16_t *dest)
{
    return modbus_read_registers(this->ctx, addr, nb, dest);
}

int qtmodbus::ReadCoils(int addr, int nb, uint8_t *dest)
{
    return modbus_read_bits(this->ctx, addr, nb, dest);
}

int qtmodbus::WriteCoils(int addr, int nb, uint8_t *src)
{
    return modbus_write_bits(this->ctx, addr, nb, src);
}

int qtmodbus::ReadDiscrete(int addr, int nb, uint8_t *dest)
{
    return modbus_read_input_bits(this->ctx, addr, nb, dest);
}

int qtmodbus::Connect()
{
    errno = 0;
    modbus_connect(this->ctx);
    return errno;
}

void qtmodbus::Close()
{
    if (this->ctx == NULL)
        return;
    modbus_close(this->ctx);
    modbus_free(this->ctx);
}

int qtmodbus::SetSlave(int addr)
{
    errno = 0;
    modbus_set_slave(this->ctx, addr);
    return errno;
}

const char *qtmodbus::GetErrMsg(int err)
{
    return modbus_strerror(err);
}
