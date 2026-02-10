#ifndef MYCHARTVIEW_H
#define MYCHARTVIEW_H

#include <QDebug>
#include <QMouseEvent>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>

class MyChartView : public QChartView
{
    Q_OBJECT
public:
    explicit MyChartView(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
};

#endif // MYCHARTVIEW_H
