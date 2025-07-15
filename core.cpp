#include "core.h"

core::core()
{
    this->modbus = NULL;
    this->status = DISCONNECTED;
    this->conn_params = NULL;
    this->current_model = DM25_400;
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
    if (this->conn_params == NULL) {
        return -1;
    }
    int feedback = -1;
    modbus = new qtmodbus(this->conn_params->type);
    if (this->conn_params->type == 0 || this->conn_params->type == 1) {
        if (this->conn_params->com_params == NULL || this->conn_params->com_params->device == NULL)
            return -1;
        feedback = modbus->COM_Init(this->conn_params->com_params->device,
                                    this->conn_params->com_params->baud_rate,
                                    this->conn_params->com_params->polarity,
                                    this->conn_params->com_params->data_bits,
                                    this->conn_params->com_params->stop_bits);
    }
    if (this->conn_params->type == 2) {
        if (this->conn_params->tcp_params == NULL || conn_params->tcp_params->ip == NULL)
            return -1;

        feedback = modbus->TCP_Init(conn_params->tcp_params->ip, conn_params->tcp_params->port);
    }
    if (feedback == 0) {
        modbus->SetSlave(10); //АДРЕС
        return 0;
    }
    this->status = DISCONNECTED;
    return -1;
}

int core::connect_modbus()
{
    if (modbus == NULL)
        return -1;

    int a = modbus->Connect();
    if (!a) {
        this->status = CONNECTED_MODBUS;
        this->load_timers_param();
        return 0;
    }
    this->status = DISCONNECTED;
    return a;
}

void core::close_modbus()
{
    if (modbus == NULL)
        return;
    this->status = DISCONNECTED;
    modbus->Close();

    this->modbus = NULL;
}

void core::fill_std_values()
{
    //мб обнулить по-умному - через чистку памяти сразу по указателю
    StopSignals();
    SetSignals(start_signal_struct{false, 100, 30, 150});

    this->heater_block.IsEnabled = 0;
    this->heater_block.control_i = 0;
    this->heater_block.measure_i = 0;
    this->heater_block.measure_u = 0;

    this->energy_block.IsEnabled = 0;
    this->energy_block.control_le = 0;
    this->energy_block.control_he = 0;
    this->energy_block.measure_le = 0;
    this->energy_block.measure_he = 0;

    this->cathode_block.IsEnabled = 0;
    this->cathode_block.control_cathode = 0;
    this->cathode_block.measure_cathode = 0;
}

void core::fill_coef()
{
    if (this->current_model == DM25_400) {
        this->coef.coef_i_set = 2.8 / 2.2;
        this->coef.coef_i_meas = 2.8 / 2.0;
    } else {
        this->coef.coef_i_set = 5.0 / 2.2;
        this->coef.coef_i_meas = 5.0 / 2.0;
    }
    this->coef.coef_u_heater_meas = 6.0 / 2.0;
    this->coef.coef_u_le_set = 2.3e3 / 2.5;
    this->coef.coef_u_le_meas = 2.3e3 / 2.0;
    this->coef.coef_u_he_set = 2.3e3 / 2.5;
    this->coef.coef_u_he_meas = 2.0e3 / 2.0;
    this->coef.coef_u_cat_set = 25e3 / 2.0;
    this->coef.coef_u_cat_meas = 25e3 / 1.89;
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
    if (this->status == CONNECTED_MODBUS) {
        uint16_t buffer_reg[9];
        uint8_t buffer_discrete[4];

        uint16_t signal_params[3];
        uint8_t signal_enabled;
        if (modbus->ReadInputRegisters(31001, 9, buffer_reg) != 9)
            return -1;
        if (modbus->ReadDiscrete(10001, 3, buffer_discrete) != 3)
            return -1;
        if (load_timers_param())
            return -1;
        this->heater_block.IsEnabled = buffer_discrete[0];
        this->heater_block.control_i = buffer_reg[0];
        this->heater_block.measure_i = buffer_reg[1];
        this->heater_block.measure_u = buffer_reg[2];

        this->energy_block.IsEnabled = buffer_discrete[1];
        this->energy_block.control_le = buffer_reg[3];
        this->energy_block.control_he = buffer_reg[4];
        this->energy_block.measure_le = buffer_reg[5];
        this->energy_block.measure_he = buffer_reg[6];

        this->cathode_block.IsEnabled = buffer_discrete[2];
        this->cathode_block.control_cathode = buffer_reg[7];
        this->cathode_block.measure_cathode = buffer_reg[8];
    }
    return 0;
}

int core::StartSignals()
{
    if (this->status == CONNECTED_MODBUS) {
        this->start_signal.IsEnabled = true;
        uint8_t a = 1;
        if (this->modbus->WriteCoils(1, 1, &a) != 1)
            return -1;
    }
    return 0;
}

int core::StopSignals()
{
    this->start_signal.IsEnabled = false;
    if (this->status == CONNECTED_MODBUS) {
        uint8_t a = 0;
        if (this->modbus->WriteCoils(1, 1, &a) != 1)
            return -1;
    }
    return 0;
}

int core::SetSignals(start_signal_struct s)
{
    uint16_t a[3];
    if (s.interval <= 500 && s.interval >= 150 && s.duration <= 150 && s.frequency <= 9999
        && s.interval <= 1000000 / s.frequency - 30) {
        this->start_signal.frequency = s.frequency;
        this->start_signal.interval = s.interval;
        this->start_signal.duration = s.duration;
    } else
        return 3;
    if (this->status == CONNECTED_MODBUS) {
        a[0] = this->start_signal.frequency;
        a[1] = this->start_signal.duration;
        a[2] = this->start_signal.interval;
        if (this->modbus->WriteHoldingRegisters(41001, 3, a) != 3)
            return 2;
    }
    return 0;
}

start_signal_struct core::GetSignals()
{
    return start_signal_struct{this->start_signal.IsEnabled,
                               this->start_signal.frequency,
                               this->start_signal.duration,
                               this->start_signal.interval};
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

int core::GetADCBytes_sport(uint8_t *buffer)
{
    if (status == CONNECTED_SPORT) {
        sport.flushReceiver();
        return sport.readBytes(buffer,
                               2 * ADC_FREQ * ADC_SAMPLES,
                               5000,
                               1000); // 2 *, так как ADC_VALUE - 2 байта, а uart принимает 1 байт
    }
    return 0;
}

int core::StartADCProcessoring(int channel)
{
    if (status == CONNECTED_SPORT) {
        return sport.writeChar(channel); //1 - начать оцифровку
    }
    return 0;
}
