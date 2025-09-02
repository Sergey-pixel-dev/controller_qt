#ifndef CORE_H
#define CORE_H

#include <QObject>
#include <QTimer>
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
#define REG_HOLDING_NREGS 4

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
struct adc_settings_struct
{
    int n_samples; // количество отсчётов
    int averaging; // усреднение
    int channel;   // текущий канал
    bool enabled;  // команда вкл/выкл АЦП (usCoils бит 1)
    bool running;  // состояние АЦП на устройстве (usDiscrete бит 5)
};
class core : public QObject
{
    Q_OBJECT

private:
    start_signal_struct start_signal;
    Manager *mngr;
    ModbusClient *mb_cli;

    // MODBUS регистры
    uint16_t usRegInputBuf[REG_INPUT_NREGS];     // index 0-8: входы IN0-IN8
    uint16_t usRegHoldingBuf[REG_HOLDING_NREGS]; // 0-HZ, 1-длительность, 2-channel АЦП, 3-n_samples
    uint16_t usCoilsBuf[1];                      // bit 0-ВКЛ ИМПУЛЬС, bit 1-ВКЛ АЦП
    uint16_t usDiscreteBuf[1];                   // bits 0-4: НАКАЛ,У.Э.,-25кВ,HE,LE

    std::mutex modbus_mutex; // Для синхронизации доступа к регистрам

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
    void updateStructuresFromRegisters();
    void initialSyncFromDevice(); // Для первого

public:
    explicit core(QObject *parent = nullptr);
    ~core();

    // Основные структуры данных
    heater_block_struct heater_block;
    energy_block_struct energy_block;
    cathode_block_struct cathode_block;
    adc_settings_struct adc_settings;
    enum_model current_model;
    coef_struct coef;
    CONN_STATUS_ENUM conn_status;

    // Методы подключения
    int connectDevice(const QString &port, int baudRate = 115200);
    void disconnectDevice();
    bool isConnected() const;

    // Методы управления АЦП
    int startADC(int channel);
    int stopADC();
    bool isADCRunning() const;

    // Методы управления сигналами
    int startSignals();
    int stopSignals();
    int setSignals(const start_signal_struct &signal);
    start_signal_struct getSignals() const;

    // Основные методы работы
    void fill_std_values();
    void fill_coef();

    // Низкоуровневые методы (для внутреннего использования)
    int open(const char *dev, int br, SerialParity par, SerialDataBits db, SerialStopBits sb);
    void close();
    int startManager();
    int stopManager();
    void clearADCbuf();
    void clearMBbuf();
    std::unique_ptr<Package<uint8_t>> GetADCBytes();

    int succes_count;
    int error_count;

signals:
    void adcDataReady(List<Package<uint8_t>> *queue, int samples, int averaging);
    void dataUpdated(); // Новый сигнал для обновления UI
};

#endif // CORE_H
