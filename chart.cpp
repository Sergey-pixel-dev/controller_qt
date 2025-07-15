#include "chart.h"

Chart::Chart()
{
    chart = new QChart();
}

Chart::~Chart()
{
    delete chart;
}

void Chart::ClearChart()
{
    const auto series = chart->series();
    for (QAbstractSeries *s : series) {
        QXYSeries *xy = qobject_cast<QXYSeries *>(s);
        if (xy) {
            chart->removeSeries(xy);
            xy->clear();
        }
    }
    const auto axes = chart->axes();
    for (QAbstractAxis *axis : axes) {
        chart->removeAxis(axis);
    }
}

void Chart::DrawChart(QVector<QPointF> points)
{
    ClearChart();

    auto *series = new QLineSeries();
    for (int i = 0; i < points.size(); ++i)
        series->append(points[i]);

    series->setPointsVisible(true);
    series->setPointLabelsVisible(true);
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle("Сигнал");

    // 4) Оси X и Y
    auto *axisX = new QValueAxis();
    axisX->setRange(0, 8);
    axisX->setLabelFormat("%.3f");
    axisX->setTickCount(20);
    axisX->setTitleText("Время, мкс");

    auto *axisY = new QValueAxis();
    axisY->setRange(0 - 100, 3300 + 100);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(10);
    axisY->setTitleText("Напряжение, мВ");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
}

QChart *Chart::GetChart()
{
    return this->chart;
}
