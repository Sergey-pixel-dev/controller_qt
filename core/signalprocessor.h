#ifndef SIGNALPROCESSOR_H
#define SIGNALPROCESSOR_H

#include <QObject>
#include <QPointF>
#include <QVector>
#include "../helping/package.h"
#include "../libs/list.h"
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
    void setData(List<Package<uint8_t>> *queue);

    QVector<QPointF> GetPoints();
    void RawDataToData(int n_samples, int averaging);

    void ClearData();

    //Фильтры
    void SetFIR_Filter(bool IsSet, int N);
    void SetThresholdFilter(bool IsSet, int threshold);
    void SetMovingAverageFilter(bool IsSet, int windowSize);

    QVector<uint32_t> origin_time;
    QVector<uint32_t> origin_data;

    int32_t voltageShift = 0;
    int32_t timeShift = 0;

private:
    List<Package<uint8_t>> *queue_data;

    QVector<uint32_t> time;
    QVector<uint32_t> data;
    void ThresholdFilter();
    void FIR_Filter();
    void MovingAverageFilter();

    FIR_Filter_params_struct FIR_Filt_par;
    MovAvrFilter_params_struct MovAvrFil_par;
    ThresholdFilter_params_struct ThresholdFIl_par;
};

#endif // SIGNALPROCESSOR_H
