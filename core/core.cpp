#include "core.h"
#include "../helping/common_macro.h"

core::core()
{
    mngr = new Manager();
    mb_cli = new ModbusClient();
    mb_cli->mngr = mngr;
    conn_status = DISCONNECTED;
    current_model = DM25_400;
    int n_samples = ADC_SAMPLES;
    int averaging = 1;
    fill_std_values();
    fill_coef();
}
core::~core() {}

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
    if (conn_status == CONNECTED) {
        uint16_t buffer_reg[9];
        uint8_t buffer_discrete[5];

        uint16_t signal_params[3];
        uint8_t signal_enabled;
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
    }
    return 0;
}

int core::StartSignals()
{
    if (conn_status == CONNECTED) {
        if (!(start_signal.duration <= 150 && start_signal.frequency <= 9999
              && start_signal.duration > 0 && start_signal.frequency > 0)) {
            return -2;
        }
        start_signal.IsEnabled = true;
        uint8_t a = 1;
        if (mb_cli->WriteCoils(&a, 1, 1, 1000) != 1)
            return -1;
    }
    return 0;
}

int core::StopSignals()
{
    if (conn_status == CONNECTED) {
        uint8_t a = 0;
        if (mb_cli->WriteCoils(&a, 1, 1, 1000) != 1)
            return -1;
        start_signal.IsEnabled = false;
    }
    return 0;
}

int core::SetSignals(start_signal_struct s)
{
    uint16_t a[3];
    if (s.duration <= 150 && s.frequency <= 9999 && s.duration > 0 && s.frequency > 0) {
        if (conn_status == CONNECTED) {
            a[0] = s.frequency;
            a[1] = s.duration;
            a[2] = s.interval;
            int res = mb_cli->WriteHoldingRegisters(a, 41001, 3, 1000);
            if (res != 3)
                return 2;
            start_signal.frequency = s.frequency;
            start_signal.interval = s.interval;
            start_signal.duration = s.duration;
        }
    } else
        return 3;
    return 0;
}

start_signal_struct core::GetSignals()
{
    return start_signal_struct{start_signal.IsEnabled,
                               start_signal.frequency,
                               start_signal.duration,
                               start_signal.interval};
}

int core::open(const char *dev, int br, SerialParity par, SerialDataBits db, SerialStopBits sb)
{
    if (mngr->open(dev, br, par, db, sb) == 1) {
        conn_status = CONNECTED;
        return 1;
    } else
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
    if (conn_status == CONNECTED) {
        uint8_t *buf = new uint8_t[3];
        std::unique_ptr<Package<uint8_t>> pack = std::make_unique<Package<uint8_t>>();
        pack->size = 3;
        pack->packageBuf = buf;
        buf[0] = OP_ADC_START;
        buf[1] = channel;
        buf[2] = n_samples;
        mngr->queueWrite(std::move(pack));
        return 0;
    }
    return -1;
}

std::unique_ptr<Package<uint8_t>> core::GetADCBytes()
{
    if (conn_status == CONNECTED) {
        std::unique_ptr<Package<uint8_t>> pack = nullptr;
        while (pack == nullptr)
            pack = mngr->getADCpackage();
        if (pack->size != 2 * ADC_FRAME_N * n_samples + 4)
            return nullptr;
        return pack;
    }
    return nullptr;
}

int core::StopADCBytes()
{
    if (conn_status == CONNECTED) {
        uint8_t *buf = new uint8_t;
        std::unique_ptr<Package<uint8_t>> pack = std::make_unique<Package<uint8_t>>();
        pack->size = 1;
        pack->packageBuf = buf;
        *buf = OP_ADC_STOP;
        mngr->queueWrite(std::move(pack));
        return 0;
    }
    return -1;
}
