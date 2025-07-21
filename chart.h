#ifndef CHART_H
#define CHART_H
#include <QRandomGenerator>
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
    Chart();
    ~Chart();

private:
    QValueAxis *axisX;
    QValueAxis *axisY;
    void ClearChart();
    QChart *chart;
};

#endif // CHART_H
