#ifndef CHART_H
#define CHART_H
#include <QRandomGenerator>
#include <QVector>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include "signalprocessor.h"
class Chart : public QChart
{
public:
    void DrawChart(QVector<QPointF> points);
    QChart *GetChart();
    Chart();
    ~Chart();

private:
    void ClearChart();
    QChart *chart;
};

#endif // CHART_H
