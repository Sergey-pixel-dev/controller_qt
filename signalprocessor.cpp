#include "signalprocessor.h"
#include "core.h"
SignalProcessor::SignalProcessor()
{
    raw_size = 0;
    raw_data = NULL;
}
//data - перепеши в vector<uint16_t>
SignalProcessor::~SignalProcessor()
{
    if (raw_data != NULL)
        delete raw_data;
}

void SignalProcessor::setData(uint8_t *data, int size)
{
    raw_data = data;
    this->raw_size = size;
}

QVector<QPointF> SignalProcessor::GetPoints() const
{
    QVector<QPointF> points;
    points.resize(data.size());
    for (int i = 0; i < data.size(); i++) {
        points[i] = QPointF(time[i] / 1000.0, data[i]);
    }
    return points;
}

void SignalProcessor::RawDataToData()
{
    /*data.clear();
    time.clear();
    time.resize(raw_size / 2);
    data.resize(raw_size / 2);
    for (int i = 0; i < raw_size / 2; i++) {
        uint16_t out = raw_data[2 * i];
        out |= raw_data[2 * i + 1] << 8;
        data[ADC_FRAME_N * (i % ADC_SAMPLES) + i / ADC_SAMPLES] = out;
        time[ADC_FRAME_N * (i % ADC_SAMPLES) + i / ADC_SAMPLES]
            = ((i % ADC_SAMPLES) * STM32_CYCL_ADC / STM32_ADC_FREQ
               + (i / ADC_SAMPLES) * STM32_CYCL_ADC / STM32_ADC_FREQ / ADC_FRAME_N)
              * 1000;
    }*/
    time.clear();
    time.resize(raw_size / 2);
    data.resize(raw_size / 2);
    for (int i = 0; i < raw_size / 2; i++) {
        uint16_t out = raw_data[2 * i];
        out |= raw_data[2 * i + 1] << 8;
        // data[(ADC_FRAME_N) * (i % ADC_SAMPLES) + i / ADC_SAMPLES] = out;
        // time[(ADC_FRAME_N) * (i % ADC_SAMPLES) + i / ADC_SAMPLES] = (1 + i % ADC_SAMPLES)
        //                                                                 * STM32_CYCL_ADC * 1000
        //                                                                 / STM32_ADC_FREQ
        //                                                             + (i / ADC_SAMPLES)
        //                                                                   * STM32_TIM_N * 1000
        //                                                                   / STM32_TIM_FREQ;
        data[i] = out;
        time[i] = i * (STM32_TIM_N) * 1000 / STM32_TIM_FREQ;
    }
}

void SignalProcessor::ThresholdFilter()
{
    QVector<uint16_t> newData;
    QVector<uint16_t> newTime;
    newData.reserve(data.size());
    newTime.reserve(time.size());
    for (int i = 0; i < data.size() - 1; i++) {
        newData.append(data[i]);
        newTime.append(time[i]);
        if (abs(data[i] - data[i + 1]) > 800)
            i++; //data[i] добавляем, а data[i+1] пропускаем
    }
    data.swap(newData);
    time.swap(newTime);
}

void SignalProcessor::FIR_Filter()
{
    const int N = 20;                                      //Длина фильтра
    long double Fd = (STM32_TIM_FREQ / STM32_TIM_N) * 1e6; //Частота дискретизации входных данных
    long double Fs = 400e3;                                //Частота полосы пропускания
    long double Fx = 1e6;                                  //Частота полосы затухания

    long double H[N] = {0};    //Импульсная характеристика фильтра
    long double H_id[N] = {0}; //Идеальная импульсная характеристика
    long double W[N] = {0};    //Весовая функция

    //Расчет импульсной характеристики фильтра
    double Fc = (Fs + Fx) / (2 * Fd);

    for (int i = 0; i < N; i++) {
        if (i == 0)
            H_id[i] = 2 * M_PI * Fc;
        else
            H_id[i] = sinl(2 * M_PI * Fc * i) / (M_PI * i);
        // весовая функция Блекмена
        W[i] = 0.42 - 0.5 * cosl((2 * M_PI * i) / (N - 1)) + 0.08 * cosl((4 * M_PI * i) / (N - 1));
        H[i] = H_id[i] * W[i];
    }

    //Нормировка импульсной характеристики
    double SUM = 0;
    for (int i = 0; i < N; i++)
        SUM += H[i];
    for (int i = 0; i < N; i++)
        H[i] /= SUM; //сумма коэффициентов равна 1

    QVector<uint16_t> newData;
    newData.resize(data.size());
    //Фильтрация входных данных
    for (int i = 0; i < data.size(); i++) {
        newData[i] = 0.;
        for (int j = 0; j < N - 1; j++) {
            if (i - j >= 0) {
                newData[i] += H[j] * data[i - j];
            }
        }
    }
    data.swap(newData);
}
void SignalProcessor::MovingAverageFilter(int windowSize)
{
    int n = data.size();
    if (windowSize <= 1 || windowSize > n) {
        return;
    }
    int sum = 0.0;
    QVector<uint16_t> newData;
    newData.resize(data.size());
    for (int i = 0; i < n; ++i) {
        sum += data[i];
        if (i >= windowSize)
            sum -= data[i - windowSize];
        newData[i] = sum / qMin(i + 1, windowSize);
    }
    data.swap(newData);
}
