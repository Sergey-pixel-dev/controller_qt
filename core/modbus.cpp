#include "modbus.h"
#include "modbus_crc.h"

int ModbusClient::ReadInputRegisters(uint16_t *buf, uint16_t startAdress, uint16_t NbReg)
{
    std::unique_ptr<Package> pack1 = std::make_unique<Package>();
    uint8_t *packBuf = new uint8_t[8]; // Исправлено: выделяем только нужное количество
    pack1->packageBuf = packBuf;
    pack1->size = 8;

    packBuf[0] = SLAVE_ID;
    packBuf[1] = 0x04;
    packBuf[2] = startAdress >> 8;
    packBuf[3] = startAdress & 0xFF; // Добавил & 0xFF для ясности
    packBuf[4] = NbReg >> 8;
    packBuf[5] = NbReg & 0xFF;

    uint16_t crc = crc16(packBuf, 6);
    packBuf[6] = crc & 0xFF;
    packBuf[7] = (crc >> 8) & 0xFF;

    mngr->clearMBlst();

    mngr->queueWrite(std::move(pack1));
    std::unique_ptr<Package> pack2 = nullptr;
    while (pack2 == nullptr)
        pack2 = mngr->getMBpackage();

    if (pack2->packageBuf[0] != SLAVE_ID)
        return -1;

    if (pack2->packageBuf[1] == (0x04 | 0x80))
        return pack2->packageBuf[2];

    int bytes_count = pack2->packageBuf[2];
    if (crc16(pack2->packageBuf, 3 + bytes_count) !=
        ((pack2->packageBuf[4 + bytes_count] << 8) | pack2->packageBuf[3 + bytes_count]))
        return -2;

    for (int i = 0; i < bytes_count / 2; i++)
    {
        buf[i] = (pack2->packageBuf[3 + 2 * i] << 8) | pack2->packageBuf[4 + 2 * i];
    }

    return NbReg;
}

int ModbusClient::ReadHoldingRegisters(uint16_t *buf, uint16_t startAdress, uint16_t NbReg)
{
    std::unique_ptr<Package> pack1 = std::make_unique<Package>();
    uint8_t *requestBuf = new uint8_t[8];
    pack1->packageBuf = requestBuf;
    pack1->size = 8;

    requestBuf[0] = SLAVE_ID;
    requestBuf[1] = 0x03;
    requestBuf[2] = startAdress >> 8;
    requestBuf[3] = startAdress & 0xFF;
    requestBuf[4] = NbReg >> 8;
    requestBuf[5] = NbReg & 0xFF;

    uint16_t crc = crc16(requestBuf, 6);
    requestBuf[6] = crc & 0xFF;
    requestBuf[7] = (crc >> 8) & 0xFF;

    mngr->clearMBlst();

    mngr->queueWrite(std::move(pack1));

    std::unique_ptr<Package> response = nullptr;
    while (response == nullptr)
        response = mngr->getMBpackage();

    if (response->packageBuf[0] != SLAVE_ID)
        return -1;

    if (response->packageBuf[1] == (0x03 | 0x80))
        return response->packageBuf[2];

    int bytes_count = response->packageBuf[2];
    if (crc16(response->packageBuf, 3 + bytes_count) !=
        ((response->packageBuf[4 + bytes_count] << 8) | response->packageBuf[3 + bytes_count]))
        return -2;

    for (int i = 0; i < bytes_count / 2; i++)
    {
        buf[i] = (response->packageBuf[3 + 2 * i] << 8) | response->packageBuf[4 + 2 * i];
    }

    return NbReg;
}

int ModbusClient::WriteHoldingRegisters(uint16_t *buf, uint16_t startAdress, uint16_t NbReg)
{
    int packet_size = 9 + NbReg * 2;
    std::unique_ptr<Package> pack1 = std::make_unique<Package>();
    uint8_t *requestBuf = new uint8_t[packet_size];
    pack1->packageBuf = requestBuf;
    pack1->size = packet_size;

    requestBuf[0] = SLAVE_ID;
    requestBuf[1] = 0x10;
    requestBuf[2] = startAdress >> 8;
    requestBuf[3] = startAdress & 0xFF;
    requestBuf[4] = NbReg >> 8;
    requestBuf[5] = NbReg & 0xFF;
    requestBuf[6] = NbReg * 2;

    for (int i = 0; i < NbReg; i++)
    {
        requestBuf[2 * i + 7] = (buf[i] >> 8) & 0xFF;
        requestBuf[2 * i + 8] = buf[i] & 0xFF;
    }

    uint16_t crc = crc16(requestBuf, 7 + NbReg * 2);
    requestBuf[7 + NbReg * 2] = crc & 0xFF;
    requestBuf[8 + NbReg * 2] = (crc >> 8) & 0xFF;

    mngr->clearMBlst();

    mngr->queueWrite(std::move(pack1));

    std::unique_ptr<Package> response = nullptr;
    while (response == nullptr)
        response = mngr->getMBpackage();

    if (response->packageBuf[0] != SLAVE_ID)
        return -1;

    if (response->packageBuf[1] == (0x10 | 0x80))
        return response->packageBuf[2];

    if (crc16(response->packageBuf, 6) !=
        ((response->packageBuf[7] << 8) | response->packageBuf[6]))
        return -2;

    return NbReg;
}

int ModbusClient::ReadDiscrete(uint8_t *buf, uint16_t startAdress, uint16_t NbDis)
{
    std::unique_ptr<Package> pack1 = std::make_unique<Package>();
    uint8_t *requestBuf = new uint8_t[8];
    pack1->packageBuf = requestBuf;
    pack1->size = 8;

    requestBuf[0] = SLAVE_ID;
    requestBuf[1] = 0x02;
    requestBuf[2] = startAdress >> 8;
    requestBuf[3] = startAdress & 0xFF;
    requestBuf[4] = NbDis >> 8;
    requestBuf[5] = NbDis & 0xFF;

    uint16_t crc = crc16(requestBuf, 6);
    requestBuf[6] = crc & 0xFF;
    requestBuf[7] = (crc >> 8) & 0xFF;

    mngr->clearMBlst();

    mngr->queueWrite(std::move(pack1));

    std::unique_ptr<Package> response = nullptr;
    while (response == nullptr)
        response = mngr->getMBpackage();

    if (response->packageBuf[0] != SLAVE_ID)
        return -1;

    if (response->packageBuf[1] == (0x02 | 0x80))
        return response->packageBuf[2];

    int bytes_count = response->packageBuf[2];
    if (crc16(response->packageBuf, 3 + bytes_count) !=
        ((response->packageBuf[4 + bytes_count] << 8) | response->packageBuf[3 + bytes_count]))
        return -2;

    for (int i = 0; i < NbDis; i++)
    {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        buf[i] = (response->packageBuf[3 + byte_idx] >> bit_idx) & 0x01;
    }

    return NbDis;
}

int ModbusClient::ReadCoils(uint8_t *buf, uint16_t startAdress, uint16_t NbCoils)
{
    std::unique_ptr<Package> pack1 = std::make_unique<Package>();
    uint8_t *requestBuf = new uint8_t[8];
    pack1->packageBuf = requestBuf;
    pack1->size = 8;

    requestBuf[0] = SLAVE_ID;
    requestBuf[1] = 0x01;
    requestBuf[2] = startAdress >> 8;
    requestBuf[3] = startAdress & 0xFF;
    requestBuf[4] = NbCoils >> 8;
    requestBuf[5] = NbCoils & 0xFF;

    uint16_t crc = crc16(requestBuf, 6);
    requestBuf[6] = crc & 0xFF;
    requestBuf[7] = (crc >> 8) & 0xFF;

    mngr->clearMBlst();

    mngr->queueWrite(std::move(pack1));

    std::unique_ptr<Package> response = nullptr;
    while (response == nullptr)
        response = mngr->getMBpackage();

    if (response->packageBuf[0] != SLAVE_ID)
        return -1;

    if (response->packageBuf[1] == (0x01 | 0x80))
        return response->packageBuf[2];

    int bytes_count = response->packageBuf[2];
    if (crc16(response->packageBuf, 3 + bytes_count) !=
        ((response->packageBuf[4 + bytes_count] << 8) | response->packageBuf[3 + bytes_count]))
        return -2;

    for (int i = 0; i < NbCoils; i++)
    {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        buf[i] = (response->packageBuf[3 + byte_idx] >> bit_idx) & 0x01;
    }

    return NbCoils;
}

int ModbusClient::WriteCoils(uint8_t *buf, uint16_t startAdress, uint16_t NbCoils)
{
    int bytes_count = (NbCoils + 7) / 8;
    int packet_size = 9 + bytes_count;

    std::unique_ptr<Package> pack1 = std::make_unique<Package>();
    uint8_t *requestBuf = new uint8_t[packet_size];
    pack1->packageBuf = requestBuf;
    pack1->size = packet_size;

    requestBuf[0] = SLAVE_ID;
    requestBuf[1] = 0x0F;
    requestBuf[2] = startAdress >> 8;
    requestBuf[3] = startAdress & 0xFF;
    requestBuf[4] = NbCoils >> 8;
    requestBuf[5] = NbCoils & 0xFF;
    requestBuf[6] = bytes_count;
    memset(requestBuf + 7, 0, bytes_count);

    for (int i_bits = 0; i_bits < NbCoils; i_bits++)
    {
        int i_bytes = i_bits / 8;
        int bit_pos = i_bits % 8;
        if (buf[i_bits])
            requestBuf[7 + i_bytes] |= (1 << bit_pos);
    }

    uint16_t crc = crc16(requestBuf, 7 + bytes_count);
    requestBuf[7 + bytes_count] = crc & 0xFF;
    requestBuf[8 + bytes_count] = (crc >> 8) & 0xFF;

    mngr->clearMBlst();

    mngr->queueWrite(std::move(pack1));

    std::unique_ptr<Package> response = nullptr;
    while (response == nullptr)
        response = mngr->getMBpackage();

    if (response->packageBuf[0] != SLAVE_ID)
        return -1;

    if (response->packageBuf[1] == (0x0F | 0x80))
        return response->packageBuf[2];

    if (crc16(response->packageBuf, 6) !=
        ((response->packageBuf[7] << 8) | response->packageBuf[6]))
        return -2;

    return NbCoils;
}

ModbusClient::ModbusClient()
{
}

ModbusClient::~ModbusClient()
{
}
