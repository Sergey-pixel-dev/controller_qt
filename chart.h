#ifndef CHART_H
#define CHART_H
#include <QRandomGenerator>
#include <QSlider>
#include <QVector>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include "signalprocessor.h"
class Chart : public QChart
{
public:
    void DrawChart(QVector<QPointF> points);
    void DrawAverage(QPointF average_point);
    QChart *GetChart();
    Chart(QSlider *slider);
    ~Chart();
    QSlider *slider;

private slots:
    void updateSliderRange(qreal min, qreal max);

private:
    QValueAxis *axisX;
    QValueAxis *axisY;
    void ClearChart();
    QChart *chart;
};

#endif // CHART_H
