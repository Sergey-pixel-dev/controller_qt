#include "core.h"
#include <QObject>
#include "../helping/common_macro.h"
#include <cstring>

core::core(QObject *parent)
    : QObject(parent)
{
    mngr = new Manager();
    mb_cli = new ModbusClient();
    mb_cli->mngr = mngr;
    conn_status = DISCONNECTED;
    current_model = DM25_400;

    // Инициализация настроек АЦП
    adc_settings.n_samples = ADC_SAMPLES;
    adc_settings.averaging = 1;
    adc_settings.channel = 1;

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
            updateStructuresFromRegisters();
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

        // Ждем пока устройство действительно выключит АЦП
        bool device_adc_stopped = false;
        int attempts = 0;
        const int max_attempts = 10;

        while (!device_adc_stopped && attempts < max_attempts && conn_status == CONNECTED) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            uint8_t device_coils[COILS_N];
            if (mb_cli->ReadCoils(device_coils, COILS_START, COILS_N, 1000) == COILS_N) {
                if ((device_coils[1] & 1) == 0) {
                    device_adc_stopped = true;
                }
            }
            attempts++;
        }
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
    if (!start_signal.IsEnabled)
        return -2;
    if (stateADC.load())
        return -3;

    clearADCbuf();

    // Устанавливаем регистры для АЦП и обновляем структуру
    {
        std::lock_guard<std::mutex> lock(modbus_mutex);
        usRegHoldingBuf[2] = channel;                // канал АЦП
        usRegHoldingBuf[3] = adc_settings.n_samples; // количество отсчетов
        usCoilsBuf[0] |= (1 << 1);                   // ВКЛ АЦП (бит 1)

        // Обновляем структуру
        adc_settings.channel = channel;
        adc_settings.enabled = true; // Команда включения отправлена
    }

    // Сигнализируем о срочной синхронизации
    urgent_sync_flag.store(true);

    // Ждем пока устройство подтвердит запуск АЦП (бит 5 в usDiscrete)
    auto start_time = std::chrono::steady_clock::now();
    bool adc_started = false;

    while (!adc_started && conn_status == CONNECTED) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 1000) {
            return -5; // Таймаут запуска АЦП
        }
        // Проверяем локальный usDiscrete (обновляется ModbusLoop потоком)
        {
            std::lock_guard<std::mutex> lock(modbus_mutex);
            if (usDiscreteBuf[0] & 0x20) { // Проверяем 4-й бит (АЦП включен)
                adc_started = true;
                adc_settings.running = true; // Обновляем состояние в структуре
            }
        }

        if (!adc_started) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            // Каждые 50мс сигнализируем о необходимости синхронизации
            urgent_sync_flag.store(true);
        }
    }

    if (!adc_started) {
        return -5; // Не удалось запустить
    }

    abortFlag.store(false);
    stateADC.store(true);
    adcThread = std::thread(&core::adcThreadLoop, this);
    return 0;
}

int core::stopADC()
{
    if (!stateADC.load())
        return 0; // Уже остановлен

    // Выключаем АЦП в регистрах и обновляем структуру
    {
        std::lock_guard<std::mutex> lock(modbus_mutex);
        usCoilsBuf[0] &= ~(1 << 1);   // ВЫКЛ АЦП (бит 1)
        adc_settings.enabled = false; // Команда выключения отправлена
    }

    // Сигнализируем о срочной синхронизации
    urgent_sync_flag.store(true);

    // Ждем пока устройство подтвердит остановку АЦП (бит 4 в usDiscrete)
    auto start_time = std::chrono::steady_clock::now();
    bool adc_stopped = false;

    while (!adc_stopped && conn_status == CONNECTED) {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 1000) {
            // Таймаут - но все равно останавливаем поток
            break;
        }

        // Проверяем локальный usDiscrete (обновляется ModbusLoop потоком)
        {
            std::lock_guard<std::mutex> lock(modbus_mutex);
            if ((usDiscreteBuf[0] & 0x20) == 0) { // Проверяем что 4-й бит сброшен (АЦП выключен)
                adc_stopped = true;
                adc_settings.running = false; // Обновляем состояние в структуре
            }
        }

        if (!adc_stopped) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            // Каждые 50мс сигнализируем о необходимости синхронизации
            urgent_sync_flag.store(true);
        }
    }

    // Останавливаем поток независимо от результата проверки устройства
    abortFlag.store(true);
    if (adcThread.joinable()) {
        adcThread.join();
    }
    stateADC.store(false);

    // Обновляем состояние в структуре при принудительной остановке
    if (!adc_stopped) {
        std::lock_guard<std::mutex> lock(modbus_mutex);
        adc_settings.running = false;
    }

    // Возвращаем результат
    return adc_stopped ? 0 : -5; // 0 = успех, -5 = таймаут остановки
}

bool core::isADCRunning() const
{
    return stateADC.load();
}

int core::startSignals()
{
    if (conn_status != CONNECTED)
        return -1;
    if (!(start_signal.duration <= 150 && start_signal.frequency <= 9999
          && start_signal.duration > 0 && start_signal.frequency > 0)) {
        return -2;
    }

    std::lock_guard<std::mutex> lock(modbus_mutex);

    // Устанавливаем бит 0 (ВКЛ ИМПУЛЬС)
    usCoilsBuf[0] |= 1;
    start_signal.IsEnabled = true;
    urgent_sync_flag.store(true);

    return 0;
}

int core::stopSignals()
{
    if (conn_status != CONNECTED)
        return -1;

    std::lock_guard<std::mutex> lock(modbus_mutex);

    // Сбрасываем бит 0 (ВЫКЛ ИМПУЛЬС)
    usCoilsBuf[0] &= ~1;
    start_signal.IsEnabled = false;
    urgent_sync_flag.store(true);

    return 0;
}

int core::setSignals(const start_signal_struct &s)
{
    if (conn_status != CONNECTED)
        return -1;

    if (!(s.duration <= 150 && s.frequency <= 9999 && s.duration > 0 && s.frequency > 0))
        return -2;

    std::lock_guard<std::mutex> lock(modbus_mutex);

    // Записываем в локальные регистры
    usRegHoldingBuf[0] = s.frequency;
    usRegHoldingBuf[1] = s.duration;

    start_signal.frequency = s.frequency;
    start_signal.duration = s.duration;
    start_signal.interval = s.interval;
    urgent_sync_flag.store(true);

    return 0;
}

start_signal_struct core::getSignals() const
{
    return start_signal_struct{start_signal.IsEnabled,
                               start_signal.frequency,
                               start_signal.duration,
                               start_signal.interval};
}

void core::fill_std_values()
{
    memset(&heater_block, 0, sizeof(heater_block));
    memset(&energy_block, 0, sizeof(energy_block));
    memset(&cathode_block, 0, sizeof(cathode_block));
    memset(&start_signal, 0, sizeof(start_signal));
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

void core::updateStructuresFromRegisters()
{
    std::lock_guard<std::mutex> lock(modbus_mutex);

    // Обновляем структуры из input регистров
    energy_block.control_le = usRegInputBuf[0];
    heater_block.control_i = usRegInputBuf[1];
    cathode_block.measure_cathode = usRegInputBuf[2];
    cathode_block.control_cathode = usRegInputBuf[3];
    energy_block.control_he = usRegInputBuf[4];
    heater_block.measure_i = usRegInputBuf[5];
    energy_block.measure_le = usRegInputBuf[6];
    heater_block.measure_u = usRegInputBuf[7];
    energy_block.measure_he = usRegInputBuf[8];

    // Обновляем из discrete
    uint16_t discrete = usDiscreteBuf[0];
    heater_block.IsEnabled = (discrete >> 0) & 1;
    energy_block.IsEnabled = (discrete >> 1) & 1;
    cathode_block.IsEnabled = (discrete >> 2) & 1;
    energy_block.LE_or_HE = (discrete >> 3) & 1;
    heater_block.IsReady = (discrete >> 4) & 1;

    // Обновляем из holding и coils для start_signal
    start_signal.frequency = usRegHoldingBuf[0];
    start_signal.duration = usRegHoldingBuf[1];
    start_signal.IsEnabled = usCoilsBuf[0] & 1;

    // Обновляем настройки АЦП из holding регистров
    adc_settings.channel = usRegHoldingBuf[2];
    adc_settings.n_samples = usRegHoldingBuf[3];
    // averaging хранится локально, не в регистрах устройства
}
void core::initialSyncFromDevice()
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
    if (mb_cli->ReadInputRegisters(device_input, REG_INPUT_START, REG_INPUT_NREGS, 1000)
        == REG_INPUT_NREGS) {
        memcpy(usRegInputBuf, device_input, sizeof(usRegInputBuf));
    }

    // Читаем Discrete
    if (mb_cli->ReadDiscrete(device_discrete, DISCRETE_START, DISCRETE_N, 1000) == DISCRETE_N) {
        usDiscreteBuf[0] = 0;
        for (int i = 0; i < DISCRETE_N; i++) {
            if (device_discrete[i])
                usDiscreteBuf[0] |= (1 << i);
        }
    }

    // Читаем Holding регистры (БЕЗ записи обратно на устройство)
    if (mb_cli->ReadHoldingRegisters(device_holding, REG_HOLDING_START, REG_HOLDING_NREGS, 1000)
        == REG_HOLDING_NREGS) {
        memcpy(usRegHoldingBuf, device_holding, sizeof(usRegHoldingBuf));
    }

    // Читаем Coils (БЕЗ записи обратно на устройство)
    if (mb_cli->ReadCoils(device_coils, COILS_START, COILS_N, 1000) == COILS_N) {
        usCoilsBuf[0] = 0;
        for (int i = 0; i < COILS_N; i++) {
            if (device_coils[i])
                usCoilsBuf[0] |= (1 << i);
        }
    }
}
void core::adcThreadLoop()
{
    const int current_n_samples = adc_settings.n_samples;
    const int current_n_averaging = adc_settings.averaging;

    std::unique_ptr<Package<uint8_t>> pack = nullptr;
    List<Package<uint8_t>> *queue = new List<Package<uint8_t>>();
    while (!abortFlag.load()) {
        for (int i = 0; i < current_n_averaging; i++) {
            while (pack == nullptr && !abortFlag.load()) {
                pack = GetADCBytes();
                if (abortFlag.load())
                    break;
            }
            if (abortFlag.load())
                break;
            queue->add(std::move(pack));
        }
        if (abortFlag.load())
            break;
        if (queue->size() == current_n_averaging) {
            QMetaObject::invokeMethod(
                this,
                [this, queue, current_n_samples, current_n_averaging]() {
                    emit adcDataReady(queue, current_n_samples, current_n_averaging);
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
                urgent_sync_flag.store(false); // Сбрасываем флаг
            } else {
                // Проверяем прошло ли 500мс с последней синхронизации
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - last_sync_time);
                if (elapsed.count() >= 500) {
                    need_sync = true;
                }
            }

            if (need_sync) {
                // Синхронизируем регистры с устройством
                syncRegistersWithDevice();
                // Обновляем структуры из регистров
                updateStructuresFromRegisters();
                // Уведомляем UI об обновлении данных
                QMetaObject::invokeMethod(
                    this, [this]() { emit dataUpdated(); }, Qt::QueuedConnection);

                last_sync_time = std::chrono::steady_clock::now();
            }
        }

        // Небольшая задержка для освобождения CPU
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

std::unique_ptr<Package<uint8_t>> core::GetADCBytes()
{
    if (conn_status != CONNECTED)
        return nullptr;
    std::unique_ptr<Package<uint8_t>> pack = nullptr;
        pack = mngr->getADCpackage();
        if (!pack)
            return nullptr;
        if (pack->size != 2 * ADC_FRAME_N * adc_settings.n_samples + 4) // ИСПОЛЬЗУЕМ adc_settings
            return nullptr;
        return pack;
}
