#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H
#define STM32_ADC_FREQ 36 //частота ацп
#define STM32_CYCL_ADC 13 //циклов на оциффровку
#define ADC_FRAME_N 26    //кол-во стробоскопических замеров (кадров)
#define ADC_SAMPLES 25    //кол-во выборок в одном стробоскопическом замере
#define STM32_TIM_FREQ 72 //частота таймера
#define STM32_TIM_N 1     //на сколько отсчетов таймера будет сделан новый замер

//команды
#define OP_MODBUS 0x10    //slave-id
#define OP_ADC_START 0x01 //оцифровка
#define OP_AVERAGE 0x02   //среднее
#define OP_ADC_STOP 0x03
#endif // COMMON_MACRO_H
