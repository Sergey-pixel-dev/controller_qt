#ifndef CHART_H
#define CHART_H

#include <QChart>
#include <QLineSeries>
#include <QPointF>
#include <QSlider>
#include <QValueAxis>
#include <QVector>
#include <QtCharts/QChartView>

class Chart : public QObject // Наследуемся от QObject для слотов
{
    Q_OBJECT

public:
    void DrawChart(const QVector<QPointF> &points); // Изменено на const &
    void DrawAverage(const QPointF &average_point); // Тоже лучше сделать const
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
