#include "signalprocessor.h"
SignalProcessor::SignalProcessor()
{
    raw_size = 0;
    raw_data = NULL;
    FIR_Filt_par.IsActive = false;
    MovAvrFil_par.IsActive = false;
    ThresholdFIl_par.IsActive = false;
}
SignalProcessor::~SignalProcessor()
{

}

void SignalProcessor::setData(uint8_t *data, int size)
{
    raw_data = data;
    raw_size = size;
}

QVector<QPointF> SignalProcessor::GetPoints()
{
    QVector<QPointF> points;
    points.resize(data.size());
    if (ThresholdFIl_par.IsActive)
        ThresholdFilter();
    if (FIR_Filt_par.IsActive)
        FIR_Filter();
    if (MovAvrFil_par.IsActive)
        MovingAverageFilter();
    for (int i = 0; i < data.size(); i++) {
        points[i] = QPointF(time[i] / 1000.0, data[i]);
    }
    return points;
}

void SignalProcessor::RawDataToData(int n_samples, int averaging)
{
    uint16_t out = 0;
    uint16_t size = raw_size / 2 / averaging;
    uint32_t sum = 0;
    origin_time.resize(size);
    origin_data.resize(size);
    for (int i = 0; i < size; i++) {
        sum = 0;
        for (int j = 0; j < averaging; j++) {
            out = raw_data[2 * i + j * size * 2];
            out |= raw_data[2 * i + 1 + j * size * 2] << 8;
            sum += out;
        }
        out = sum / averaging;
        origin_data[(ADC_FRAME_N) * (i % n_samples) + i / n_samples] = out;
        origin_time[(ADC_FRAME_N) * (i % n_samples) + i / n_samples] = (i % n_samples)
                                                                           * STM32_CYCL_ADC * 1000
                                                                           / STM32_ADC_FREQ
                                                                       + (i / n_samples)
                                                                             * STM32_TIM_N * 1000
                                                                             / STM32_TIM_FREQ;
    }
    data = origin_data;
    time = origin_time;
}

void SignalProcessor::ClearData()
{
    //origin_data.clear();
    //origin_time.clear();
    data.clear();
    time.clear();
}

void SignalProcessor::SetFIR_Filter(bool IsSet, int N)
{
    FIR_Filt_par.N = N;
    FIR_Filt_par.IsActive = IsSet;
}

void SignalProcessor::SetThresholdFilter(bool IsSet, int threshold)
{
    ThresholdFIl_par.threshold = threshold;
    ThresholdFIl_par.IsActive = IsSet;
}

void SignalProcessor::SetMovingAverageFilter(bool IsSet, int windowSize)
{
    MovAvrFil_par.windowSize = windowSize;
    MovAvrFil_par.IsActive = IsSet;
}
void SignalProcessor::ThresholdFilter()
{
    QVector<uint16_t> newData;
    QVector<uint16_t> newTime;
    newData.reserve(origin_data.size());
    newTime.reserve(origin_time.size());
    for (int i = 0; i < origin_data.size() - 1; i++) {
        newData.append(origin_data[i]);
        newTime.append(origin_time[i]);
        if (abs(origin_data[i] - origin_data[i + 1]) > ThresholdFIl_par.threshold)
            i++; //data[i] добавляем, а data[i+1] пропускаем
    }
    data.swap(newData);
    time.swap(newTime);
}

void SignalProcessor::FIR_Filter()
{
    int N = FIR_Filt_par.N;                                       // нечётное число коэффициентов
    const long double Fd = (STM32_TIM_FREQ / STM32_TIM_N) * 1e6L; // Гц, частота дискретизации

    const long double Fs = 5e6L;          // Гц, частота полосы пропускания
    const long double Fx = Fs + 5.5L / N; // Гц, частота начала полосы затухания

    long double *H = new long double[N];    // итоговые коэффициенты
    long double *H_id = new long double[N]; // идеальная (несмещённая) импульсная характеристика
    long double *W = new long double[N];    // окно Блэкмана

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
    newData.resize(origin_data.size());

    for (int i = 0; i < origin_data.size(); i++) {
        long double acc = 0.0L;
        for (int j = 0; j < N; j++) {
            if (i - j >= 0) {
                acc += H[j] * origin_data[i - j];
            }
        }
        if (acc < 0)
            newData[i] = 0;
        else
            newData[i] = static_cast<uint16_t>(acc);
    }
    data.swap(newData);
    delete[] H;
    delete[] H_id;
    delete[] W;
}
void SignalProcessor::MovingAverageFilter()
{
    int n = origin_data.size();
    if (MovAvrFil_par.windowSize <= 1 || MovAvrFil_par.windowSize > n) {
        return;
    }
    int sum = 0.0;
    QVector<uint16_t> newData;
    newData.resize(origin_data.size());
    for (int i = 0; i < n; ++i) {
        sum += origin_data[i];
        if (i >= MovAvrFil_par.windowSize)
            sum -= origin_data[i - MovAvrFil_par.windowSize];
        newData[i] = sum / qMin(i + 1, MovAvrFil_par.windowSize);
    }
    data.swap(newData);
}
