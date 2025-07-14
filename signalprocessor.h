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
    uint16_t minValue() const;
    uint16_t maxValue() const;

    //  QVector<double> movingAverage(int windowSize) const;

private:
    int raw_size;
    int data_size;
    uint8_t *raw_data; //должен быть в ДИНАМ ПАМЯТИ!!
    uint16_t *data;
    uint16_t *time;
};

#endif // SIGNALPROCESSOR_H
