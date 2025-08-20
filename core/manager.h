/*************************************************
 *  manager.h  —  поток-приёмник UART             *
 *************************************************/
#ifndef MANAGER_H
#define MANAGER_H

#include "serialib.h"
#include "list.h"
#include "modbus_crc.h" // crc16()
#include <deque>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

#define MAX_ADC_BUFFER_SIZE 2048 // полезные байты одного ADC-кадра
#define MAX_MB_BUFFER_SIZE 128   // максимальный размер Modbus-кадра
#define RXBUF_HWM 8192           // high-water-mark накопительного буфера
#define RXBUF_TRIM 4096          // срез, если буфер перерос HWM

#define SLAVE_ID_MB 10
#define ADC_START_1 0xAA
#define ADC_START_2 0x55

class Manager // экземлпяр в динам память
{
public:
    Manager();
    ~Manager();

    void initSPORT(const char *device, int baud_rate,
                   SerialParity parity,
                   SerialDataBits data_bits,
                   SerialStopBits stop_bits);

    int start();
    int stop();
    int open(const char *, int, SerialParity, SerialDataBits, SerialStopBits);
    void close();
    void queueWrite(std::unique_ptr<Package> pack);

    std::unique_ptr<Package> getADCpackage();
    std::unique_ptr<Package> getMBpackage();
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
    std::queue<std::unique_ptr<Package>> writeQueue;
    std::mutex mtxWriteQueue;

    const char *device;
    int baud_rate;
    SerialParity parity;
    SerialDataBits data_bits;
    SerialStopBits stop_bits;

    List lstADC;
    List lstMB;
};

#endif /* MANAGER_H */
