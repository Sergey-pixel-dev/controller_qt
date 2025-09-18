#include "core.h"
#include "../helping/common_macro.h"
core::core(QObject *parent)
    : QObject(parent)
{
    mngr = new Manager();
    mb_cli = new ModbusClient();
    mb_cli->mngr = mngr;

    conn_status = DISCONNECTED;
    current_model = DM25_400;

    // Инициализация MODBUS регистров
    memset(usRegInputBuf, 0, sizeof(usRegInputBuf));
    memset(usRegHoldingBuf, 0, sizeof(usRegHoldingBuf));
    memset(usCoilsBuf, 0, sizeof(usCoilsBuf));
    memset(usDiscreteBuf, 0, sizeof(usDiscreteBuf));

    succes_count = 0;
    error_count = 0;

    fill_std_values();
    fill_coef();
}

core::~core()
{
    abortFlag = true;
    abortModbusFlag = true;

    if (adcThread.joinable())
        adcThread.join();
    if (modbusThread.joinable())
        modbusThread.join();

    delete mngr;
    delete mb_cli;
}

int core::connectDevice(const QString &port, int baudRate)
{
    if (conn_status == CONNECTED)
        return -1;

    if (open(port.toUtf8().constData(),
             baudRate,
             SERIAL_PARITY_NONE,
             SERIAL_DATABITS_8,
             SERIAL_STOPBITS_1)
        == 1) {
        if (startManager() == 0) {
            initialSyncFromDevice();
            startModbusUpdateThread();
            return 0;
        } else {
            return -3;
        }
    } else {
        return -4;
    }
}

void core::disconnectDevice()
{
    if (stateADC.load()) {
        stopADC();
    }

    stopModbusUpdateThread();
    stopManager();
    close();
    fill_std_values();
}

bool core::isConnected() const
{
    return conn_status == CONNECTED;
}

int core::startADC(int channel)
{
    if (!isConnected())
        return -1;
    if (!(usCoilsBuf[0] & 1)) // Проверяем бит 0 - сигнал СТАРТ
        return -2;
    if (stateADC.load())
        return -3;

    clearADCbuf();

    {
        std::lock_guard<std::mutex> lock(modbus_mutex);
        usRegHoldingBuf[2] = channel;
        usCoilsBuf[0] |= (1 << 1); // Включаем АЦП
    }

    // Сигнализируем о срочной синхронизации
    urgent_sync_flag.store(true);

    // Ждем пока устройство подтвердит запуск АЦП
    auto start_time = std::chrono::steady_clock::now();
    bool adc_started = false;

    while (!adc_started && conn_status == CONNECTED) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 1000) {
            return -5;
        }

        {
            std::lock_guard<std::mutex> lock(modbus_mutex);
            if (usDiscreteBuf[0] & 0x20) { // Проверяем бит 5 (АЦП включен)
                adc_started = true;
            }
        }

        if (!adc_started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            urgent_sync_flag.store(true);
        }
    }

    if (!adc_started) {
        return -5;
    }

    abortFlag.store(false);
    stateADC.store(true);
    adcThread = std::thread(&core::adcThreadLoop, this);
    return 0;
}

int core::stopADC()
{
    if (!stateADC.load())
        return 0;

    {
        std::lock_guard<std::mutex> lock(modbus_mutex);
        usCoilsBuf[0] &= ~(1 << 1); // ВЫКЛ АЦП (бит 1)
    }

    urgent_sync_flag.store(true);

    // Ждем пока устройство подтвердит остановку АЦП
    auto start_time = std::chrono::steady_clock::now();
    bool adc_stopped = false;

    while (!adc_stopped && conn_status == CONNECTED) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 1000) {
            break;
        }
        {
            std::lock_guard<std::mutex> lock(modbus_mutex);
            if ((usDiscreteBuf[0] & 0x20) == 0) { // Проверяем что бит 5 сброшен
                adc_stopped = true;
            }
        }

        if (!adc_stopped) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            urgent_sync_flag.store(true);
        }
    }

    // Останавливаем поток
    abortFlag.store(true);
    if (adcThread.joinable()) {
        adcThread.join();
    }

    stateADC.store(false);
    return adc_stopped ? 0 : -5;
}

bool core::isADCRunning() const
{
    return stateADC.load();
}

int core::startSignals()
{
    if (conn_status != CONNECTED)
        return -1;

    // Проверяем параметры из регистров
    if (!(usRegHoldingBuf[1] <= 1500 && usRegHoldingBuf[0] <= 9999 && usRegHoldingBuf[1] > 0
          && usRegHoldingBuf[0] > 0)) {
        return -2;
    }

    std::lock_guard<std::mutex> lock(modbus_mutex);
    usCoilsBuf[0] |= 1; // Устанавливаем бит 0 (ВКЛ ИМПУЛЬС)
    urgent_sync_flag.store(true);
    return 0;
}

int core::stopSignals()
{
    if (conn_status != CONNECTED)
        return -1;

    std::lock_guard<std::mutex> lock(modbus_mutex);
    usCoilsBuf[0] &= ~1; // Сбрасываем бит 0 (ВЫКЛ ИМПУЛЬС)
    urgent_sync_flag.store(true);
    return 0;
}

int core::setSignals(uint16_t frequency, uint16_t duration, uint16_t interval)
{
    if (conn_status != CONNECTED)
        return -1;
    if (!(duration <= 150 && frequency <= 9999 && duration > 0 && frequency > 0))
        return -2;

    std::lock_guard<std::mutex> lock(modbus_mutex);
    usRegHoldingBuf[0] = frequency;
    usRegHoldingBuf[1] = duration;
    urgent_sync_flag.store(true);
    return 0;
}

bool core::getSignalEnabled()
{
    std::lock_guard<std::mutex> lock(modbus_mutex);
    return usCoilsBuf[0] & 1;
}

uint16_t core::getSignalFrequency()
{
    std::lock_guard<std::mutex> lock(modbus_mutex);
    return usRegHoldingBuf[0];
}

uint16_t core::getSignalDuration()
{
    std::lock_guard<std::mutex> lock(modbus_mutex);
    return usRegHoldingBuf[1];
}

void core::fill_std_values()
{
    energy_impulse = 0;
    cathode_impulse_i = 0;
    cathode_impulse_u = 0;
    energy_impulse_pos = 0;
    cathode_impulse_i_pos = 0;
    cathode_impulse_u_pos = 0;
}

void core::fill_coef()
{
    if (current_model == DM25_400) {
        coef.coef_i_set = 5.0 / 2.2;
        coef.coef_i_meas = 5.0 / 2.0;
    } else {
        coef.coef_i_set = 5.0 / 2.2;
        coef.coef_i_meas = 5.0 / 2.0;
    }

    coef.coef_u_heater_meas = 6.0 / 2.0;
    coef.coef_u_le_set = 2.3e3 / 2.5;
    coef.coef_u_le_meas = 2.0e3 / 2.0;
    coef.coef_u_he_set = 2.3e3 / 2.5;
    coef.coef_u_he_meas = 2.0e3 / 2.0;
    coef.coef_u_cat_set = 25e3 / 2.0;
    coef.coef_u_cat_meas = 25e3 / 1.89;
}

void core::syncRegistersWithDevice()
{
    if (conn_status != CONNECTED)
        return;

    std::lock_guard<std::mutex> lock(modbus_mutex);

    // Читаем все регистры с устройства
    uint16_t device_input[REG_INPUT_NREGS];
    uint8_t device_discrete[DISCRETE_N];
    uint16_t device_holding[REG_HOLDING_NREGS];
    uint8_t device_coils[COILS_N];

    // Читаем Input регистры
    if (mb_cli->ReadInputRegisters(device_input, REG_INPUT_START, REG_INPUT_NREGS, 300)
        == REG_INPUT_NREGS) {
        memcpy(usRegInputBuf, device_input, sizeof(usRegInputBuf));
        succes_count++;
    } else
        error_count++;

    // Читаем Discrete
    if (mb_cli->ReadDiscrete(device_discrete, DISCRETE_START, DISCRETE_N, 300) == DISCRETE_N) {
        usDiscreteBuf[0] = 0;
        for (int i = 0; i < DISCRETE_N; i++) {
            if (device_discrete[i])
                usDiscreteBuf[0] |= (1 << i);
        }
        succes_count++;
    } else
        error_count++;

    // Проверяем и синхронизируем Holding регистры
    if (mb_cli->ReadHoldingRegisters(device_holding, REG_HOLDING_START, REG_HOLDING_NREGS, 300)
        == REG_HOLDING_NREGS) {
        bool need_update_holding = false;
        succes_count++;

        for (int i = 0; i < REG_HOLDING_NREGS; i++) {
            if (device_holding[i] != usRegHoldingBuf[i]) {
                need_update_holding = true;
                break;
            }
        }

        if (need_update_holding) {
            if (mb_cli->WriteHoldingRegisters(usRegHoldingBuf,
                                              REG_HOLDING_START,
                                              REG_HOLDING_NREGS,
                                              300)
                == REG_HOLDING_NREGS)
                succes_count++;
            else
                error_count++;
        }
    } else
        error_count++;

    // Проверяем и синхронизируем Coils
    if (mb_cli->ReadCoils(device_coils, COILS_START, COILS_N, 300) == COILS_N) {
        uint16_t device_coils_word = 0;
        for (int i = 0; i < COILS_N; i++) {
            if (device_coils[i])
                device_coils_word |= (1 << i);
        }

        if (device_coils_word != usCoilsBuf[0]) {
            uint8_t coils_to_write[COILS_N];
            for (int i = 0; i < COILS_N; i++) {
                coils_to_write[i] = (usCoilsBuf[0] >> i) & 1;
            }

            if (mb_cli->WriteCoils(coils_to_write, COILS_START, COILS_N, 300) == COILS_N)
                succes_count++;
            else
                error_count++;
        }
    } else
        error_count++;
}

void core::initialSyncFromDevice()
{
    if (conn_status != CONNECTED)
        return;

    std::lock_guard<std::mutex> lock(modbus_mutex);

    uint16_t device_input[REG_INPUT_NREGS];
    uint8_t device_discrete[DISCRETE_N];
    uint16_t device_holding[REG_HOLDING_NREGS];
    uint8_t device_coils[COILS_N];

    // Читаем Input регистры
    if (mb_cli->ReadInputRegisters(device_input, REG_INPUT_START, REG_INPUT_NREGS, 300)
        == REG_INPUT_NREGS) {
        memcpy(usRegInputBuf, device_input, sizeof(usRegInputBuf));
    }

    // Читаем Discrete
    if (mb_cli->ReadDiscrete(device_discrete, DISCRETE_START, DISCRETE_N, 300) == DISCRETE_N) {
        usDiscreteBuf[0] = 0;
        for (int i = 0; i < DISCRETE_N; i++) {
            if (device_discrete[i])
                usDiscreteBuf[0] |= (1 << i);
        }
    }

    // Читаем Holding регистры
    if (mb_cli->ReadHoldingRegisters(device_holding, REG_HOLDING_START, REG_HOLDING_NREGS, 300)
        == REG_HOLDING_NREGS) {
        memcpy(usRegHoldingBuf, device_holding, sizeof(usRegHoldingBuf));
    }

    // Читаем Coils
    if (mb_cli->ReadCoils(device_coils, COILS_START, COILS_N, 300) == COILS_N) {
        usCoilsBuf[0] = 0;
        for (int i = 0; i < COILS_N; i++) {
            if (device_coils[i])
                usCoilsBuf[0] |= (1 << i);
        }
    }
}

void core::adcThreadLoop()
{
    std::unique_ptr<Package<uint8_t>> pack = nullptr;
    List<Package<uint8_t>> *queue = new List<Package<uint8_t>>();

    int current_averaging;
    int current_n_samples;

    while (!abortFlag.load()) {
        {
            std::lock_guard<std::mutex> lock(modbus_mutex);
            current_n_samples = usRegHoldingBuf[3];
            current_averaging = usRegHoldingBuf[5];
        }

        for (int i = 0; i < current_averaging; i++) {
            pack = GetADCBytes(current_n_samples);
            if (pack)
                queue->add(std::move(pack));
        }

        if (abortFlag.load())
            break;

        if (queue->size() == current_averaging) {
            QMetaObject::invokeMethod(
                this,
                [this, queue, current_n_samples, current_averaging]() {
                    emit adcDataReady(queue, current_n_samples, current_averaging);
                },
                Qt::QueuedConnection);
            queue = new List<Package<uint8_t>>();
        }
    }

    delete queue;
}

void core::modbusUpdateLoop()
{
    auto last_sync_time = std::chrono::steady_clock::now();

    while (!abortModbusFlag.load()) {
        if (conn_status == CONNECTED) {
            bool need_sync = false;

            // Проверяем флаг срочной синхронизации
            if (urgent_sync_flag.load()) {
                need_sync = true;
                urgent_sync_flag.store(false);
            } else {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_sync_time);
                if (elapsed.count() >= period_mb_ms) {
                    need_sync = true;
                }
            }

            if (need_sync) {
                syncRegistersWithDevice();
                QMetaObject::invokeMethod(
                    this, [this]() { emit dataUpdated(); }, Qt::QueuedConnection);
                last_sync_time = std::chrono::steady_clock::now();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void core::startModbusUpdateThread()
{
    if (modbusThread.joinable())
        return;

    abortModbusFlag.store(false);
    modbusThread = std::thread(&core::modbusUpdateLoop, this);
}

void core::stopModbusUpdateThread()
{
    abortModbusFlag.store(true);
    if (modbusThread.joinable()) {
        modbusThread.join();
    }
}

int core::open(const char *dev, int br, SerialParity par, SerialDataBits db, SerialStopBits sb)
{
    if (mngr->open(dev, br, par, db, sb) == 1) {
        conn_status = CONNECTED;
        return 1;
    }
    return 0;
}

void core::close()
{
    conn_status = DISCONNECTED;
    mngr->close();
}

int core::startManager()
{
    return mngr->start();
}

int core::stopManager()
{
    return mngr->stop();
}

void core::clearADCbuf()
{
    mngr->clearADClst();
}

void core::clearMBbuf()
{
    mngr->clearMBlst();
}

std::unique_ptr<Package<uint8_t>> core::GetADCBytes(int samples)
{
    if (conn_status != CONNECTED)
        return nullptr;

    std::unique_ptr<Package<uint8_t>> pack = nullptr;
    pack = mngr->getADCpackage();
    if (!pack)
        return nullptr;
    if (pack->size != 2 * ADC_FRAME_N * samples + 4)
        return nullptr;
    return pack;
}
