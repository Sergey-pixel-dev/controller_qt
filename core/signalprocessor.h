#ifndef SIGNALPROCESSOR_H
#define SIGNALPROCESSOR_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include "../helping/common_macro.h"

struct FIR_Filter_params_struct
{
    bool IsActive;
    int N;
};

struct MovAvrFilter_params_struct
{
    bool IsActive;
    int windowSize;
};

struct ThresholdFilter_params_struct
{
    bool IsActive;
    int threshold;
};

class SignalProcessor
{
public:
    SignalProcessor();
    ~SignalProcessor();
    void setData(uint8_t *data, int size);

    QVector<QPointF> GetPoints();
    void RawDataToData(int n_samples, int averaging);

    void ClearData();

    //Фильтры
    void SetFIR_Filter(bool IsSet, int N);
    void SetThresholdFilter(bool IsSet, int threshold);
    void SetMovingAverageFilter(bool IsSet, int windowSize);

    QVector<uint16_t> origin_time;
    QVector<uint16_t> origin_data;

private:
    int raw_size;
    uint8_t *raw_data; //должен быть в ДИНАМ ПАМЯТИ!!

    QVector<uint16_t> time;
    QVector<uint16_t> data;
    void ThresholdFilter();
    void FIR_Filter();
    void MovingAverageFilter();

    FIR_Filter_params_struct FIR_Filt_par;
    MovAvrFilter_params_struct MovAvrFil_par;
    ThresholdFilter_params_struct ThresholdFIl_par;
};

#endif // SIGNALPROCESSOR_H
