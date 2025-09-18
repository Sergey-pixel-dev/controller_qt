#ifndef MANAGER_H
#define MANAGER_H

#include "../helping/package.h"
#include "../libs/list.h"
#include "../libs/serialib.h"
#include <atomic>
#include <deque>
#include <mutex>
#include <queue>
#include <thread>

#define MAX_ADC_BUFFER_SIZE 2048
#define MAX_MB_BUFFER_SIZE 128
#define RXBUF_HWM 8192
#define RXBUF_TRIM 4096
#define SLAVE_ID_MB 10
#define ADC_START_1 0xAA
#define ADC_START_2 0x55

using PackageBuf = Package<uint8_t>;

class Manager
{
public:
    Manager();
    ~Manager();

    int start();
    int stop();
    int open(const char *dev, int br, SerialParity p, SerialDataBits db, SerialStopBits sb);
    void close();
    void queueWrite(std::unique_ptr<PackageBuf> pack);
    std::unique_ptr<PackageBuf> getADCpackage();
    std::unique_ptr<PackageBuf> getMBpackage();
    void clearADClst();
    void clearMBlst();

private:
    int main_loop();
    bool extractADC(std::deque<uint8_t> &buf);
    bool extractModbus(std::deque<uint8_t> &buf);

    serialib sport;
    std::thread main_thread;
    std::atomic_bool isAborted{false};
    std::atomic_bool isStarted{false};

    std::mutex mtxMB;
    std::mutex mtxADC;
    std::queue<std::unique_ptr<PackageBuf>> writeQueue;
    std::mutex mtxWriteQueue;

    // Добавляем отсутствующие поля
    const char *device = nullptr;
    int baud_rate = 115200;
    SerialParity parity = SERIAL_PARITY_NONE;
    SerialDataBits data_bits = SERIAL_DATABITS_8;
    SerialStopBits stop_bits = SERIAL_STOPBITS_1;

    List<PackageBuf> lstADC;
    List<PackageBuf> lstMB;
};

#endif /* MANAGER_H */
