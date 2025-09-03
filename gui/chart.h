#ifndef CHART_H
#define CHART_H

#include <QChart>
#include <QLineSeries>
#include <QPointF>
#include <QSlider>
#include <QValueAxis>
#include <QVector>
#include <QtCharts/QChartView>

class Chart : public QObject
{
    Q_OBJECT

public:
    void DrawChart(const QVector<QPointF> &points);
    void DrawAverage(const QPointF &average_point);

    void setTimeScale(double usPerDiv);
    void setVoltageScale(double vPerDiv);
    void updateGridAndLabels();

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
    double timePerDiv = 1.0;    // мкс/деление
    double voltagePerDiv = 1.0; // В/деление
    int gridDivisions = 6;      // количество делений на экране
    QChart *chart;
};

#endif // CHART_H
