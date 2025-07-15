#ifndef CORE_H
#define CORE_H
#define STM32_ADC_FREQ 12
#define STM32_CYCL_ADC 13.5
#define ADC_FREQ 36
#define ADC_SAMPLES 8
#include "qtmodbus.h"
#include "serialib.h"
enum STATUS_ENUM {
    DISCONNECTED,
    CONNECTED_MODBUS,
    CONNECTED_SPORT,
    ERR,
};

struct com_struct
{
    const char *device;
    int baud_rate;
    char polarity;
    char data_bits;
    char stop_bits;
};

struct tcp_struct
{
    const char *ip;
    int port;
};
struct conn_struct
{
    int type;
    com_struct *com_params;
    tcp_struct *tcp_params;
};

struct heater_block_struct
{
    bool IsEnabled;
    uint16_t measure_i;
    uint16_t measure_u;
    uint16_t control_i;
};
struct energy_block_struct
{
    bool IsEnabled;
    bool IsStarted;
    uint16_t control_he;
    uint16_t control_le;
    uint16_t measure_he;
    uint16_t measure_le;
};
struct cathode_block_struct
{
    bool IsEnabled;
    uint16_t control_cathode;
    uint16_t measure_cathode;
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

class core
{
private:
    start_signal_struct start_signal;

public:
    STATUS_ENUM status;
    conn_struct *conn_params;
    int init_modbus();
    int connect_modbus();
    int UpdateValues();
    int StartSignals();
    int StopSignals();
    int SetSignals(start_signal_struct);
    start_signal_struct GetSignals();
    void close_modbus();
    void fill_std_values();
    void fill_coef();
    int load_timers_param();
    qtmodbus *modbus;

    //Serial port
    int connect_sport();
    int close_sprot();
    int GetADCBytes_sport(uint8_t *buffer);
    int StartADCProcessoring(int channel);

    serialib sport; //в стеке потому что в динам памяти connectDevie -> fd = open падает с segm fault

    //может нужен указатель на heater_block
    heater_block_struct heater_block;
    energy_block_struct energy_block;
    cathode_block_struct cathode_block;
    enum_model current_model;
    coef_struct coef;
    core();
    ~core();
};

#endif // CORE_H
