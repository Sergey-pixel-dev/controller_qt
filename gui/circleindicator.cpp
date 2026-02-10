#include "circleindicator.h"
#include <QPainter>

CircleIndicator::CircleIndicator(QWidget *parent)
    : QWidget(parent)
    , m_active(false)
{
    // фиксируем квадратный размер (12×12 px)
    setFixedSize(12, 12);
}

void CircleIndicator::setActive(bool on)
{
    if (m_active == on)
        return;
    m_active = on;
    update();
}

void CircleIndicator::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor fill = m_active ? QColor(0, 200, 0)      // зелёный
                           : QColor(150, 150, 150); // серый
    p.setBrush(fill);
    p.setPen(Qt::NoPen);
    p.drawEllipse(rect());
}
