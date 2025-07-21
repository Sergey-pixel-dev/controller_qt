#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSerialPortInfo>
#include <QTimer>
#include "chart.h"
#include "core.h"
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_button_connect_clicked();
    void slotTimerAlarm();
    void on_comboBox_activated(int index);
    void on_checkBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_pushButton_clicked();
    void on_tabWidget_currentChanged(int index);

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_comboBox_3_currentIndexChanged(int index);

    void on_doubleSpinBox_valueChanged(double arg1);

    void on_pushButton_4_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_doubleSpinBox_editingFinished();

private:
    void UpdateScreenValues();
    void HasBeenConnected();
    void HasBeenDisconnected();
    QByteArray kostil_name_device;
    SignalProcessor *processor;
    QTimer *timer;
    core *my_core;
    uint8_t *buffer; // вынести потом отсюда куда нибудь
    QMessageBox *msgBox;
    Ui::MainWindow *ui;
    Chart *my_chart;
    void showErrMsgBox(const char *title, const char *msg);
};
#endif // MAINWINDOW_H
