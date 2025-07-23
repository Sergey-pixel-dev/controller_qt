#include "chart.h"

Chart::Chart(QSlider *slider)
{
    chart = new QChart();
    axisX = new QValueAxis();
    axisX->setRange(0, 8);
    axisX->setLabelFormat("%.3f");
    axisX->setTickCount(20);
    axisX->setTitleText("Время, мкс");

    axisY = new QValueAxis();
    axisY->setRange(0, 4000);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(10);
    axisY->setTitleText("Напряжение, мВ");
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    connect(axisX, &QValueAxis::rangeChanged, this, &Chart::updateSliderRange);

    this->slider = slider;
}

Chart::~Chart()
{
    delete chart;
    delete axisX;
    delete axisY;
}

void Chart::updateSliderRange(qreal min, qreal max)
{
    int min_int = min * 576 / 8;
    int max_int = max * 576 / 8;
    if (!(slider->value() >= min_int) || !(slider->value() <= max_int)) {
        slider->setValue(min_int);
    }
    slider->setMinimum(min_int);
    slider->setMaximum(max_int);
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
}

void Chart::DrawChart(QVector<QPointF> points)
{
    ClearChart();

    auto *series = new QLineSeries();
    for (int i = 0; i < points.size(); ++i)
        series->append(points[i]);

    //series->setPointsVisible(true);
    //series->setPointLabelsVisible(true);
    chart->addSeries(series);
    chart->legend()->hide();
    chart->setTitle("Сигнал");

    series->attachAxis(axisX);
    series->attachAxis(axisY);
}

void Chart::DrawAverage(QPointF average_point)
{
    auto *series = new QLineSeries();
    series->setPointsVisible(true);
    series->setPointLabelsVisible(true);
    series->setMarkerSize(7.5);
    QString label = QString("(%1, %2)").arg(average_point.x(), 0, 'f', 3).arg(average_point.y());
    series->setPointLabelsFormat(label);
    QFont font = series->pointLabelsFont();
    font.setPointSize(12);
    series->setPointLabelsFont(font);
    series->append(average_point);
    chart->addSeries(series);
    series->attachAxis(axisX);
    series->attachAxis(axisY);
    chart->update();
}

QChart *Chart::GetChart()
{
    return this->chart;
}
