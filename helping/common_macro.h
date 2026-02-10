#ifndef COMMON_MACRO_H
#define COMMON_MACRO_H
#define STM32_ADC_FREQ 36 //частота ацп
#define STM32_CYCL_ADC 13 //циклов на оциффровку
#define ADC_FRAME_N 26    //кол-во стробоскопических замеров (кадров)
#define ADC_SAMPLES 24    //кол-во выборок в одном стробоскопическом замере
#define STM32_TIM_FREQ 72 //частота таймера
#define STM32_TIM_N 1     //на сколько отсчетов таймера будет сделан новый замер

#define GRID_DIVISIONS 6
#endif // COMMON_MACRO_H
