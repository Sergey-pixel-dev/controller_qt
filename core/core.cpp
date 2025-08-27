#include "core.h"
#include <QMetaObject>
#include "../helping/common_macro.h"
#include <chrono>

core::core(QObject *parent)
    : QObject(parent)
{
    mngr = new Manager();
    mb_cli = new ModbusClient();
    mb_cli->mngr = mngr;
    conn_status = DISCONNECTED;
    current_model = DM25_400;
    n_samples = ADC_SAMPLES;
    averaging = 1;
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
            if (UpdateValues() == 0) {
                startModbusUpdateThread();
                return 0;
            } else {
                disconnectDevice();
                return -2;
            }
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

    if (!start_signal.IsEnabled)
        return -2;

    if (stateADC.load())
        return -3;

    clearADCbuf();
    current_channel = channel;

    if (StartADCBytes(channel) != 0)
        return -4;

    abortFlag.store(false);
    stateADC.store(true);
    adcThread = std::thread(&core::adcThreadLoop, this);

    return 0;
}

void core::stopADC()
{
    if (!stateADC.load())
        return;

    abortFlag.store(true);
    if (adcThread.joinable()) {
        adcThread.join();
    }

    stateADC.store(false);
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

    start_signal.IsEnabled = true;
    uint8_t a = 1;
    if (mb_cli->WriteCoils(&a, 1, 1, 1000) != 1)
        return -3;

    return 0;
}

int core::stopSignals()
{
    if (conn_status != CONNECTED)
        return -1;

    uint8_t a = 0;
    if (mb_cli->WriteCoils(&a, 1, 1, 1000) != 1)
        return -3;

    start_signal.IsEnabled = false;
    return 0;
}

int core::setSignals(const start_signal_struct &s)
{
    if (conn_status != CONNECTED)
        return -1;

    if (!(s.duration <= 150 && s.frequency <= 9999 && s.duration > 0 && s.frequency > 0))
        return -2;

    uint16_t a[3] = {s.frequency, s.duration, s.interval};

    if (mb_cli->WriteHoldingRegisters(a, 41001, 3, 1000) != 3)
        return -3;

    start_signal.frequency = s.frequency;
    start_signal.interval = s.interval;
    start_signal.duration = s.duration;

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

int core::load_timers_param()
{
    uint16_t a[3];
    uint8_t b;

    if (mb_cli->ReadHoldingRegisters(a, 41001, 3, 1000) != 3)
        return -1;
    if (mb_cli->ReadCoils(&b, 1, 1, 1000) != 1)
        return -1;

    start_signal.IsEnabled = b;
    start_signal.frequency = a[0];
    start_signal.duration = a[1];
    start_signal.interval = a[2];

    return 0;
}

int core::UpdateValues()
{
    if (conn_status != CONNECTED)
        return -1;

    uint16_t buffer_reg[9];
    uint8_t buffer_discrete[5];

    if (mb_cli->ReadInputRegisters(buffer_reg, 31001, 9, 1000) != 9)
        return -1;
    if (mb_cli->ReadDiscrete(buffer_discrete, 10001, 5, 1000) != 5)
        return -1;
    if (load_timers_param())
        return -1;

    heater_block.IsEnabled = buffer_discrete[0];
    heater_block.IsReady = buffer_discrete[4];
    heater_block.control_i = buffer_reg[1];
    heater_block.measure_i = buffer_reg[5];
    heater_block.measure_u = buffer_reg[7];

    energy_block.IsEnabled = buffer_discrete[1];
    energy_block.LE_or_HE = buffer_discrete[3];
    energy_block.control_le = buffer_reg[0];
    energy_block.control_he = buffer_reg[4];
    energy_block.measure_le = buffer_reg[6];
    energy_block.measure_he = buffer_reg[8];

    cathode_block.IsEnabled = buffer_discrete[2];
    cathode_block.control_cathode = buffer_reg[3];
    cathode_block.measure_cathode = buffer_reg[2];

    return 0;
}

void core::adcThreadLoop()
{
    const int current_n_samples = n_samples;
    const int current_n_averaging = averaging;
    std::unique_ptr<PackageBuf> pack = nullptr;
    List<PackageBuf> *queue = new List<PackageBuf>();

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

            queue = new List<PackageBuf>();
        }
    }

    delete queue;
    StopADCBytes();
}

void core::modbusUpdateLoop()
{
    while (!abortModbusFlag.load()) {
        if (conn_status == CONNECTED) {
            UpdateValues();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(700));
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

int core::StartADCBytes(int channel)
{
    if (conn_status != CONNECTED)
        return -1;

    uint8_t *buf = new uint8_t[3];
    std::unique_ptr<PackageBuf> pack = std::make_unique<PackageBuf>();
    pack->size = 3;
    pack->packageBuf = buf;
    buf[0] = OP_ADC_START;
    buf[1] = channel;
    buf[2] = n_samples;
    mngr->queueWrite(std::move(pack));

    return 0;
}

std::unique_ptr<PackageBuf> core::GetADCBytes()
{
    if (conn_status != CONNECTED)
        return nullptr;

    std::unique_ptr<PackageBuf> pack = nullptr;
    while (pack == nullptr)
        pack = mngr->getADCpackage();

    if (pack->size != 2 * ADC_FRAME_N * n_samples + 4)
        return nullptr;

    return pack;
}

int core::StopADCBytes()
{
    if (conn_status != CONNECTED)
        return -1;

    uint8_t *buf = new uint8_t;
    std::unique_ptr<PackageBuf> pack = std::make_unique<PackageBuf>();
    pack->size = 1;
    pack->packageBuf = buf;
    *buf = OP_ADC_STOP;
    mngr->queueWrite(std::move(pack));

    return 0;
}
