#ifndef SIGNALPROCESSOR_H
#define SIGNALPROCESSOR_H

#include <QObject>
#include <QPointF>
#include <QVector>

class SignalProcessor
{
public:
    SignalProcessor();
    ~SignalProcessor();
    void setData(uint8_t *data, int size);

    QVector<QPointF> GetPoints() const;
    void RawDataToData();
    void ThresholdFilter();
    void FIR_Filter();

    void MovingAverageFilter(int windowSize);

private:
    int raw_size;
    uint8_t *raw_data; //должен быть в ДИНАМ ПАМЯТИ!!
    QVector<uint16_t> time;
    QVector<uint16_t> data;
};

#endif // SIGNALPROCESSOR_H
