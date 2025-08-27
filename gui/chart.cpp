#include "chart.h"

Chart::Chart(QSlider *sl)
{
    chart = new QChart();
    chart->legend()->hide();
    chart->setTitle("Сигнал");

    axisX = new QValueAxis();
    axisX->setRange(0, 9);
    axisX->setLabelFormat("%.3f");
    axisX->setTickCount(20);
    axisX->setTitleText("Время, мкс");

    axisY = new QValueAxis();
    axisY->setRange(0, 3350);
    axisY->setLabelFormat("%d");
    axisY->setTickCount(10);
    axisY->setTitleText("Напряжение, мВ");

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    connect(axisX, &QValueAxis::rangeChanged, this, &Chart::updateSliderRange);

    slider = sl;

    series = new QLineSeries();
    chart->addSeries(series);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // Для средней точки
    average_series = new QLineSeries();
    average_series->setPointsVisible(true);
    average_series->setPointLabelsVisible(true);
    average_series->setPointLabelsClipping(false);
    average_series->setMarkerSize(7.5);
    chart->addSeries(average_series);
    average_series->attachAxis(axisX);
    average_series->attachAxis(axisY);

    QFont font = average_series->pointLabelsFont();
    font.setPointSize(12);
    average_series->setPointLabelsFont(font);

    single_point.resize(1);
}

Chart::~Chart()
{
    delete chart;
}

void Chart::updateSliderRange(qreal min, qreal max)
{
    int min_int = min * 720 / 10;
    int max_int = max * 720 / 10;

    if (!(slider->value() >= min_int) || !(slider->value() <= max_int)) {
        slider->setValue(min_int);
    }

    slider->setMinimum(min_int);
    slider->setMaximum(max_int);
}

void Chart::DrawChart(const QVector<QPointF> &points)
{
    series->replace(points);
}

void Chart::DrawAverage(const QPointF &average_point)
{
    QString label = QString("(%1, %2)").arg(average_point.x(), 0, 'f', 3).arg(average_point.y());
    average_series->setPointLabelsFormat(label);
    single_point[0] = average_point;
    average_series->replace(single_point);
}

QChart *Chart::GetChart()
{
    return this->chart;
}
