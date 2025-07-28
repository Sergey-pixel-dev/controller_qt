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
    const int N = 101;                                            // нечётное число коэффициентов
    const long double Fd = (STM32_TIM_FREQ / STM32_TIM_N) * 1e6L; // Гц, частота дискретизации
    // const long double Fs = 400e3L;                                // Гц, частота полосы пропускания
    // const long double Fx = 400e3L + 0.054L; // Гц, частота начала полосы затухания

    const long double Fs = 5e6L;          // Гц, частота полосы пропускания
    const long double Fx = Fs + 0.05445L; // Гц, частота начала полосы затухания

    long double H[N] = {0};    // итоговые коэффициенты
    long double H_id[N] = {0}; // идеальная (несмещённая) импульсная характеристика
    long double W[N] = {0};    // окно Блэкмана

    // нормированная частота среза (в единицах от Fd)
    const long double Fc = ((Fs + Fx) / 2.0L) / Fd;

    // вычисляем идеальную характеристику и окно
    for (int n = 0; n < N; n++) {
        // идеальный sinc
        if (n == 0) {
            H_id[n] = 2.0L * Fc;
        } else {
            H_id[n] = sinl(2.0L * M_PI * Fc * n) / (M_PI * n);
        }
        // окно Блэкмана
        W[n] = 0.42L + 0.50L * cosl(2.0L * M_PI * n / (N - 1))
               + 0.08L * cosl(4.0L * M_PI * n / (N - 1));
        // итоговый коэффициент до нормировки
        H[n] = H_id[n] * W[n];
    }

    // нормировка так, чтобы сумма H[n] = 1
    long double sum = 0.0L;
    for (int n = 0; n < N; n++) {
        sum += H[n];
    }
    for (int n = 0; n < N; n++) {
        H[n] /= sum;
    }

    QVector<uint16_t> newData;
    newData.resize(data.size());

    for (int i = 0; i < data.size(); i++) {
        long double acc = 0.0L;
        for (int j = 0; j < N; j++) {
            if (i - j >= 0) {
                acc += H[j] * data[i - j];
            }
        }
        newData[i] = static_cast<uint16_t>(acc);
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
