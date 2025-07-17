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
    data.clear();
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
        if (abs(data[i] - data[i + 1]) > 500)
            i++; //data[i] добавляем, а data[i+1] пропускаем
    }
    data.swap(newData);
    time.swap(newTime);
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
