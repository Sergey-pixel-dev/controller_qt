#ifndef CIRCLEINDICATOR_H
#define CIRCLEINDICATOR_H

#include <QWidget>

class CircleIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit CircleIndicator(QWidget *parent = nullptr);
    void setActive(bool on);

protected:
    void paintEvent(QPaintEvent *) override;

private:
    bool m_active;
};

#endif // CIRCLEINDICATOR_H
