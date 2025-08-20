#ifndef CHART_H
#define CHART_H
#include <QSlider>
#include <QVector>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
class Chart : public QChart
{
public:
    void DrawChart(QVector<QPointF> &points);
    void DrawAverage(QPointF average_point);
    QChart *GetChart();
    Chart(QSlider *slider);
    ~Chart();
    QSlider *slider;
    QValueAxis *axisX;
    QValueAxis *axisY;
    QLineSeries *series;
    QLineSeries *average_series;
    QVector<QPointF> single_point;

private slots:
    void updateSliderRange(qreal min, qreal max);

private:
    QChart *chart;
};

#endif // CHART_H
