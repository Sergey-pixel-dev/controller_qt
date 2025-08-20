#include "mychartview.h"

MyChartView::MyChartView(QWidget *parent)
    : QChartView(parent)
{
    setRenderHint(QPainter::Antialiasing);
    // Для обработки mouseMoveEvent без зажатой кнопки:
    setMouseTracking(true);
}

void MyChartView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        chart()->zoomReset();
        event->accept();
        return;
    }

    QChartView::mousePressEvent(event);
}

void MyChartView::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();

    if (delta > 0) {
        chart()->zoomIn();
    } else {
        chart()->zoomOut();
    }
    event->accept();
}
