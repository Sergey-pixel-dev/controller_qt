#ifndef CORE_H
#define CORE_H

#include <QObject>
#include "manager.h"
#include "modbus.h"
#include <atomic>
#include <cstring>
#include <thread>

enum CONN_STATUS_ENUM {
    DISCONNECTED,
    CONNECTED,
    ERR,
};

struct heater_block_struct
{
    bool IsEnabled;
    bool IsReady;
    uint16_t measure_i;
    uint16_t measure_u;
    uint16_t control_i;
};

struct energy_block_struct
{
    bool IsEnabled;
    bool IsStarted;
    bool LE_or_HE; // 1 - LE, 0 - HE
    uint16_t control_he;
    uint16_t control_le;
    uint16_t measure_he;
    uint16_t measure_le;
    uint16_t impulse;
    int impulse_pos;
};

struct cathode_block_struct
{
    bool IsEnabled;
    uint16_t control_cathode;
    uint16_t measure_cathode;
    uint16_t impulse_i;
    uint16_t impulse_u;
    int impulse_i_pos;
    int impulse_u_pos;
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

enum enum_model {
    DM25_400,
    DM25_401,
    DM25_600,
};

struct start_signal_struct
{
    bool IsEnabled;
    uint16_t frequency;
    uint16_t duration;
    uint16_t interval;
};

class core : public QObject
{
    Q_OBJECT

private:
    start_signal_struct start_signal;
    Manager *mngr;
    ModbusClient *mb_cli;

    // Потоки управления
    std::thread adcThread;
    std::thread modbusThread;
    std::atomic<bool> abortFlag{false};
    std::atomic<bool> abortModbusFlag{false};
    std::atomic<bool> stateADC{false};

    void adcThreadLoop();
    void modbusUpdateLoop();
    void startModbusUpdateThread();
    void stopModbusUpdateThread();
    int load_timers_param();

public:
    explicit core(QObject *parent = nullptr);
    ~core();

    // Основные структуры данных
    heater_block_struct heater_block;
    energy_block_struct energy_block;
    cathode_block_struct cathode_block;
    enum_model current_model;
    coef_struct coef;
    CONN_STATUS_ENUM conn_status;
    int n_samples;
    int averaging;
    int current_channel = 1;

    // Методы подключения
    int connectDevice(const QString &port, int baudRate = 115200);
    void disconnectDevice();
    bool isConnected() const;

    // Методы управления АЦП
    int startADC(int channel);
    void stopADC();
    bool isADCRunning() const;

    // Методы управления сигналами (убираем дублирование!)
    int startSignals();
    int stopSignals();
    int setSignals(const start_signal_struct &signal);
    start_signal_struct getSignals() const;

    // Основные методы работы
    void fill_std_values();
    void fill_coef();
    int UpdateValues();

    // Низкоуровневые методы (для внутреннего использования)
    int open(const char *dev, int br, SerialParity par, SerialDataBits db, SerialStopBits sb);
    void close();
    int startManager();
    int stopManager();
    void clearADCbuf();
    void clearMBbuf();
    int StartADCBytes(int channel);
    std::unique_ptr<PackageBuf> GetADCBytes();
    int StopADCBytes();

signals:
    void adcDataReady(List<PackageBuf> *queue, int samples, int averaging);
};

#endif // CORE_H
