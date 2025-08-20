/*************************************************
 *                   manager.cpp                 *
 *************************************************/
#include "manager.h"
#include "modbus_crc.h"
#include <algorithm> // search, find
#include <atomic>
#include <deque>
#include <thread>

static const uint8_t ADC_HDR[2] = {ADC_START_1, ADC_START_2};
static const uint8_t ADC_FTR[2] = {ADC_START_2, ADC_START_1};

/*------------------ ctor / dtor ------------------*/
Manager::Manager()
    : isAborted(false), isStarted(false) {}

Manager::~Manager() { stop(); }

int Manager::start()
{
    if (isStarted.load())
        return -1;

    isAborted = false;
    open(device, baud_rate, parity, data_bits, stop_bits);
    main_thread = std::thread(&Manager::main_loop, this);
    isStarted = true;
    return 0;
}

int Manager::stop()
{
    if (!isStarted.load())
        return 0;

    isAborted = true;
    if (main_thread.joinable())
        main_thread.join();
    isStarted = false;
    return 0;
}

void Manager::queueWrite(std::unique_ptr<Package> pack)
{
    std::lock_guard<std::mutex> lock(mtxWriteQueue);
    writeQueue.push(std::move(pack));
}

std::unique_ptr<Package> Manager::getADCpackage()
{
    std::lock_guard<std::mutex> lock(mtxADC);
    return lstADC.pop();
}

std::unique_ptr<Package> Manager::getMBpackage()
{
    std::lock_guard<std::mutex> lock(mtxMB);
    return lstMB.pop();
}

void Manager::clearADClst()
{
    std::lock_guard<std::mutex> lock(mtxADC);
    lstADC.free();
}

void Manager::clearMBlst()
{
    std::lock_guard<std::mutex> lock(mtxMB);
    lstMB.free();
}

int Manager::open(const char *dev, int br, SerialParity p, SerialDataBits db, SerialStopBits sb)
{

    if (!sport.isDeviceOpen())
        return sport.openDevice(dev, br, db, p, sb);
    return 0;
}

void Manager::close()
{
    sport.closeDevice();
}

bool Manager::extractADC(std::deque<uint8_t> &buf)
{
    auto beg = std::search(buf.begin(), buf.end(), std::begin(ADC_HDR), std::end(ADC_HDR));
    if (beg == buf.end())
        return false;

    auto end = std::search(beg + 2, buf.end(), std::begin(ADC_FTR), std::end(ADC_FTR));
    if (end == buf.end())
        return false;

    end += 2;

    int packet_size = int(end - beg);
    auto pack = std::make_unique<Package>();
    pack->size = packet_size;
    pack->packageBuf = new uint8_t[packet_size];
    std::copy(beg, end, pack->packageBuf);

    lstADC.add(std::move(pack));

    buf.erase(beg, end);
    return true;
}

bool Manager::extractModbus(std::deque<uint8_t> &buf)
{
    auto it = std::find(buf.begin(), buf.end(), SLAVE_ID_MB);
    if (it == buf.end())
        return false;

    int available = int(buf.end() - it);
    if (available < 5)
        return false; // минимальная длина

    uint8_t func = *(it + 1);
    int len = 0;

    if (func & 0x80) // сообщение об ошибке запроса
        len = 5;
    else if (func == 0x06 || func == 0x10 || func == 0x0F)
        len = 8;
    else if (func == 0x01 || func == 0x02 || func == 0x03 || func == 0x04)
        len = 5 + *(it + 2);
    else
    {
        buf.erase(it);
        return true;
    }

    if (available < len)
        return false; // ждём остаток

    uint16_t crcFrame = (uint16_t)*(it + len - 1) << 8 | *(it + len - 2);

    if (crc16(&(*it), len - 2) != crcFrame)
    {
        buf.erase(it); // неверный CRC, сдвигаем окно
        return true;
    }

    auto p = std::make_unique<Package>();
    p->size = len;
    p->packageBuf = new uint8_t[len];
    std::copy(it, it + len, p->packageBuf);

    {
        std::lock_guard<std::mutex> lock(mtxMB);
        lstMB.add(std::move(p));
    }

    buf.erase(it, it + len);
    return true;
}

int Manager::main_loop()
{
    std::deque<uint8_t> rxBuf;
    uint8_t chunk[1400]; // Размер больше максимального пакета!!!
    int idle_count = 0;
    int readed = 0;

    if (open(device, baud_rate, parity, data_bits, stop_bits) != 1)
        return -1;

    sport.flushReceiver();

    while (!isAborted.load())
    {
        {
            std::lock_guard<std::mutex> lock(mtxWriteQueue);
            while (!writeQueue.empty())
            {
                std::unique_ptr<Package> pack = std::move(writeQueue.front());
                writeQueue.pop();
                sport.writeBytes(pack->packageBuf, pack->size);
            }
        }

        readed = sport.readBytes(chunk, sizeof(chunk), 2, 500);

        if (readed < 0)
            break;

        if (readed > 0)
        {
            rxBuf.insert(rxBuf.end(), chunk, chunk + readed);
            idle_count = 0;
        }
        else
        {
            idle_count++;
        }

        // Обрабатываем пакеты только когда нет входящих данных
        if (idle_count > 3 || rxBuf.size() > 5000)
        {
            bool progress = true;
            int process_iterations = 0;
            while (progress && !isAborted.load() && process_iterations < 10)
            {
                progress = false;
                progress |= extractADC(rxBuf);
                progress |= extractModbus(rxBuf);
                process_iterations++;
            }
            idle_count = 0;
        }

        if (rxBuf.size() > RXBUF_HWM)
        {
            rxBuf.erase(rxBuf.begin(), rxBuf.begin() + RXBUF_TRIM);
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    close();
    return 0;
}
