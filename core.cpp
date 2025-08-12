#include "core.h"

int n_samples = ADC_SAMPLES;
int averaging = 1;

core::core()
{
    modbus = NULL;
    status = DISCONNECTED;
    conn_params = NULL;
    current_model = DM25_400;
    fill_std_values();
    fill_coef();
}
core::~core()
{
    if (modbus != NULL)
        delete modbus;
    if (conn_params != NULL) {
        if (conn_params->com_params != NULL)
            delete conn_params->com_params;
        if (conn_params->tcp_params != NULL)
            delete conn_params->tcp_params;
        delete conn_params;
    }
}

int core::init_modbus()
{
    if (conn_params == NULL) {
        return -1;
    }
    int feedback = -1;
    modbus = new qtmodbus(conn_params->type);
    if (conn_params->type == 0 || conn_params->type == 1) {
        if (conn_params->com_params == NULL || conn_params->com_params->device == NULL)
            return -1;
        feedback = modbus->COM_Init(conn_params->com_params->device,
                                    conn_params->com_params->baud_rate,
                                    conn_params->com_params->polarity,
                                    conn_params->com_params->data_bits,
                                    conn_params->com_params->stop_bits);
    }
    if (conn_params->type == 2) {
        if (conn_params->tcp_params == NULL || conn_params->tcp_params->ip == NULL)
            return -1;

        feedback = modbus->TCP_Init(conn_params->tcp_params->ip, conn_params->tcp_params->port);
    }
    if (feedback == 0) {
        modbus->SetSlave(10); //АДРЕС
        return 0;
    }
    status = DISCONNECTED;
    return -1;
}

int core::connect_modbus()
{
    if (modbus == NULL)
        return -1;

    int a = modbus->Connect();
    if (!a) {
        status = CONNECTED_MODBUS;
        load_timers_param();
        return 0;
    }
    status = DISCONNECTED;
    return a;
}

void core::close_modbus()
{
    if (modbus == NULL)
        return;
    status = DISCONNECTED;
    modbus->Close();

    modbus = NULL;
}

void core::fill_std_values()
{
    StopSignals();
    SetSignals(start_signal_struct{false, 100, 30, 0});
    heater_block.IsEnabled = 0;
    heater_block.IsReady = 0;
    heater_block.control_i = 0;
    heater_block.measure_i = 0;
    heater_block.measure_u = 0;

    energy_block.IsEnabled = 0;
    energy_block.LE_or_HE = 0;
    energy_block.control_le = 0;
    energy_block.control_he = 0;
    energy_block.measure_le = 0;
    energy_block.measure_he = 0;
    energy_block.impulse = 0;
    energy_block.impulse_pos = 0;

    cathode_block.IsEnabled = 0;
    cathode_block.control_cathode = 0;
    cathode_block.measure_cathode = 0;
    cathode_block.impulse_i = 0;
    cathode_block.impulse_u = 0;
    cathode_block.impulse_i_pos = 0;
    cathode_block.impulse_u_pos = 0;
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
    if (modbus->ReadHoldingRegisters(41001, 3, a) != 3)
        return -1;
    if (modbus->ReadCoils(1, 1, &b) != 1)
        return -1;
    start_signal.IsEnabled = b;
    start_signal.frequency = a[0];
    start_signal.duration = a[1];
    start_signal.interval = a[2];
    return 0;
}

int core::UpdateValues()
{
    if (status == CONNECTED_MODBUS) {
        uint16_t buffer_reg[9];
        uint8_t buffer_discrete[5];

        uint16_t signal_params[3];
        uint8_t signal_enabled;
        if (modbus->ReadInputRegisters(31001, 9, buffer_reg) != 9)
            return -1;
        if (modbus->ReadDiscrete(10001, 5, buffer_discrete) != 5)
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
    if (status == CONNECTED_MODBUS) {
        if (!(start_signal.duration <= 150 && start_signal.frequency <= 9999
              && start_signal.duration > 0
              && start_signal.frequency
                     > 0)) { // start_signal.interval <= 500 && start_signal.interval <= 1000000 / start_signal.frequency - 30
            return -2;
        }
        start_signal.IsEnabled = true;
        uint8_t a = 1;
        if (modbus->WriteCoils(1, 1, &a) != 1)
            return -1;
    }
    return 0;
}

int core::StopSignals()
{
    start_signal.IsEnabled = false;
    if (status == CONNECTED_MODBUS) {
        uint8_t a = 0;
        if (modbus->WriteCoils(1, 1, &a) != 1)
            return -1;
    }
    return 0;
}

int core::SetSignals(start_signal_struct s)
{
    uint16_t a[3];
    if (s.duration <= 150 && s.frequency <= 9999 && s.duration > 0 && s.frequency > 0) {
        start_signal.frequency = s.frequency;
        start_signal.interval = s.interval;
        start_signal.duration = s.duration;
    } else
        return 3;
    if (status == CONNECTED_MODBUS) {
        a[0] = start_signal.frequency;
        a[1] = start_signal.duration;
        a[2] = start_signal.interval;
        if (modbus->WriteHoldingRegisters(41001, 3, a) != 3)
            return 2;
    }
    return 0;
}

start_signal_struct core::GetSignals()
{
    return start_signal_struct{start_signal.IsEnabled,
                               start_signal.frequency,
                               start_signal.duration,
                               start_signal.interval};
}

//sport
int core::connect_sport()
{
    if (status == CONNECTED_MODBUS || status == CONNECTED_SPORT)
        return -1;
    if (sport.openDevice(conn_params->com_params->device, conn_params->com_params->baud_rate) != 1)
        return -1;
    status = CONNECTED_SPORT;
    return 0;
}

int core::close_sprot()
{
    if (status == CONNECTED_SPORT) {
        sport.closeDevice();
        status = DISCONNECTED;
    }
    return 0;
}

int core::StartADCBytes(int channel)
{
    if (status == CONNECTED_SPORT) {
        uint8_t buf[3];
        buf[0] = OP_ADC_START;
        buf[1] = channel;
        buf[2] = n_samples;
        return sport.writeBytes(buf, 3);
    }
    return -1;
}

int core::GetADCBytes(uint8_t *buffer)
{
    if (status == CONNECTED_SPORT) {;
        if (sport.readBytes(buffer, 2 * ADC_FRAME_N * n_samples, 2000, 1000)
            != 2 * ADC_FRAME_N * n_samples)
            return -1;
        return 1;
    }
    return -1;
}

int core::StopADCBytes()
{
    if (status == CONNECTED_SPORT) {
        uint8_t buf = OP_ADC_STOP;
        return sport.writeBytes(&buf, 1);
    }
    return -1;
}

uint16_t core::GetADCAverage()
{
    if (status == CONNECTED_SPORT) {
        uint8_t buf[2];
        uint16_t average;
        sport.readBytes(buf, 2, 5000, 1000);
        average = (buf[0] | buf[1] << 8);
        return average;
    }
    return 0;
}

int core::StartADCAverage(int channel, int i_offset)
{
    if (status == CONNECTED_SPORT) {
        uint8_t buf[4];
        buf[0] = OP_AVERAGE;
        buf[1] = channel;
        buf[2] = i_offset & 0xFF;
        buf[3] = (i_offset >> 8) & 0xFF;
        return sport.writeBytes(buf, 4);
    }
    return 0;
}
