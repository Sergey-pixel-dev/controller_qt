#ifndef CORE_H
#define CORE_H

#include <QMetaObject>
#include <QObject>
#include "manager.h"
#include "modbus.h"
#include <atomic>
#include <mutex>
#include <thread>

// MODBUS определения
#define SLAVE_ID 10
#define ILLEGAL_FUNCTION 0x01
#define ILLEGAL_DATA_ADDRESS 0x02
#define ILLEGAL_DATA_VALUE 0x03

#define REG_INPUT_START 31001
#define REG_HOLDING_START 41001
#define COILS_START 00001
#define DISCRETE_START 10001

#define COILS_N 2
#define DISCRETE_N 6
#define REG_INPUT_NREGS 9
#define REG_HOLDING_NREGS 6

enum CONN_STATUS_ENUM {
    DISCONNECTED,
    CONNECTED,
    ERR,
};

enum enum_model {
    DM25_400,
    DM25_401,
    DM25_600,
};

struct coef_struct
{
    float coef_i_set;
    float coef_i_meas;
    float coef_u_heater_meas;
    float coef_u_he_set;
    float coef_u_he_meas;
    float coef_u_le_set;
    float coef_u_le_meas;
    float coef_u_cat_set;
    float coef_u_cat_meas;
};

class core : public QObject
{
    Q_OBJECT

private:
    Manager *mngr;
    ModbusClient *mb_cli;

    // Потоки управления
    std::thread adcThread;
    std::thread modbusThread;
    std::atomic<bool> abortFlag{false};
    std::atomic<bool> abortModbusFlag{false};
    std::atomic<bool> stateADC{false};
    std::atomic<bool> urgent_sync_flag{false};

    void adcThreadLoop();
    void modbusUpdateLoop();
    void startModbusUpdateThread();
    void stopModbusUpdateThread();

    // Синхронизация MODBUS регистров
    void syncRegistersWithDevice();
    void initialSyncFromDevice();

public:
    explicit core(QObject *parent = nullptr);
    ~core();

    enum_model current_model;
    coef_struct coef;
    CONN_STATUS_ENUM conn_status;

    // Методы подключения
    int connectDevice(const QString &port, int baudRate = 2000000);
    void disconnectDevice();
    bool isConnected() const;

    // Методы управления АЦП
    int startADC(int channel);
    int stopADC();
    bool isADCRunning() const;

    // Методы управления сигналами
    int startSignals();
    int stopSignals();
    int setSignals(uint16_t frequency, uint16_t duration, uint16_t interval);
    bool getSignalEnabled();
    uint16_t getSignalFrequency();
    uint16_t getSignalDuration();

    // Основные методы работы
    void fill_std_values();
    void fill_coef();

    // Низкоуровневые методы
    int open(const char *dev, int br, SerialParity par, SerialDataBits db, SerialStopBits sb);
    void close();
    int startManager();
    int stopManager();
    void clearADCbuf();
    void clearMBbuf();
    std::unique_ptr<Package<uint8_t>> GetADCBytes(int samples);

    int succes_count;
    int error_count;
    std::mutex modbus_mutex;

    // MODBUS регистры (единственный источник данных)
    uint16_t usRegInputBuf[REG_INPUT_NREGS]; // index 0-8: входы IN0-IN8
    uint16_t usRegHoldingBuf
        [REG_HOLDING_NREGS]; // 0-HZ, 1-длительность, 2-channel АЦП, 3-n_samples, 4 - сдвиг триггера, 5 - усреднение
    uint16_t usCoilsBuf[1];    // bit 0-ВКЛ ИМПУЛЬС, bit 1-ВКЛ АЦП
    uint16_t usDiscreteBuf[1]; // bits 0-4: НАКАЛ,У.Э.,-25кВ,HE,LE

    // Импульсные значения и позиции (не в MODBUS регистрах)
    uint16_t energy_impulse = 0;
    uint16_t cathode_impulse_i = 0;
    uint16_t cathode_impulse_u = 0;
    int energy_impulse_pos = 0;
    int cathode_impulse_i_pos = 0;
    int cathode_impulse_u_pos = 0;

signals:
    void adcDataReady(List<Package<uint8_t>> *queue, int samples, int averaging);
    void dataUpdated();
};

#endif // CORE_H
