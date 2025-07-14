#include "signalprocessor.h"
#include "core.h"
SignalProcessor::SignalProcessor()
{
    raw_size = 0;
    data_size = 0;
    raw_data = NULL;
    data = NULL;
    time = NULL;
}

SignalProcessor::~SignalProcessor()
{
    if (raw_data != NULL)
        delete raw_data;
    if (data != NULL)
        delete data;
    if (time != NULL)
        delete time;
}

void SignalProcessor::setData(uint8_t *data, int size)
{
    raw_data = data;
    this->raw_size = size;
}

QVector<QPointF> SignalProcessor::GetPoints() const
{
    if (data == NULL)
        return QVector<QPointF>();
    QVector<QPointF> points;
    points.resize(data_size);
    for (int i = 0; i < data_size; i++) {
        points[i] = QPointF(time[i] / 1000.0, data[i]);
    }
    return points;
}

void SignalProcessor::RawDataToData()
{
    data_size = raw_size / 2;
    if (data == NULL) {
        data = new uint16_t[data_size];
        time = new uint16_t[data_size];
    }
    for (int i = 0; i < data_size; i++) {
        uint16_t out = raw_data[2 * i];
        out |= raw_data[2 * i + 1] << 8;
        data[ADC_FREQ * (i % ADC_SAMPLES) + i / ADC_SAMPLES] = out;
        time[ADC_FREQ * (i % ADC_SAMPLES) + i / ADC_SAMPLES] = ((i % ADC_SAMPLES) * STM32_CYCL_ADC
                                                                    / STM32_ADC_FREQ
                                                                + (i / ADC_SAMPLES) * STM32_CYCL_ADC
                                                                      / STM32_ADC_FREQ / ADC_FREQ)
                                                               * 1000;
    }
}

void SignalProcessor::ThresholdFilter()
{
    uint16_t j = data_size;
    for (int i = 0; i < data_size - 1; i++) {
        if (abs(data[i + 1] - data[i]) > 1000) {
            data[i + 1] = 0;
            i++; //точку убрали -> скип
            j--;
        }
    }
    uint16_t *tmp_data = new uint16_t[j];
    uint16_t *tmp_time = new uint16_t[j];
    j = 0;
    for (int i = 0; i < data_size; i++) {
        if (data[i]) {
            tmp_data[j] = data[i];
            tmp_time[j] = time[i];
            j++;
        }
    }
    data_size = j;
    delete[] data;
    delete[] time;
    data = tmp_data;
    time = tmp_time;
}

uint16_t SignalProcessor::minValue() const
{
    return 0;
}

uint16_t SignalProcessor::maxValue() const
{
    return 0;
}

//QVector<double> SignalProcessor::movingAverage(int windowSize) const
//{
// int n = m_data.size();
// if (windowSize <= 1 || windowSize > n) {
//     // Нечего сглаживать — возвращаем оригинал
//     return m_data;
// }

// QVector<double> result(n);
// double sum = 0.0;

// // Наращиваем сумму для первых windowSize точек
// for (int i = 0; i < n; ++i) {
//     sum += m_data[i];
//     if (i >= windowSize)
//         sum -= m_data[i - windowSize];
//     // делим на число реально задействованных точек (для первых <windowSize)
//     result[i] = sum / qMin(i + 1, windowSize);
// }

// return result;
//}
