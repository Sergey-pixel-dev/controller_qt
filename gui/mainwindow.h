#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QSerialPortInfo>
#include <QThread>
#include <QTimer>
#include "../core/core.h"
#include "../core/signalprocessor.h"
#include "../helping/common_macro.h"
#include "chart.h"
#include <thread>
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

protected:
    void closeEvent(QCloseEvent *event) override;
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

    void on_horizontalSlider_valueChanged(int value);
    void on_doubleSpinBox_editingFinished();
    void on_pushButton_5_clicked();
    void on_comboBox_6_currentIndexChanged(int index);
    void on_comboBox_4_currentIndexChanged(int index);
    void on_comboBox_7_currentIndexChanged(int index);

    void adcThreadLoop();
    void handleThreadResult();
    void updateChart(QVector<QPointF> points);
    void adcThreadStop();
    void adcThreadStart();
    void startModbusUpdateThread();
    void stopModbusUpdateThread();
    void on_pushButton_6_clicked();

    void on_comboBox_2_currentIndexChanged(int index);

signals:
    void requestChartUpdate(QVector<QPointF> points);

private:
    void UpdateScreenValues();
    void HasBeenConnected();
    void HasBeenDisconnected();
    QByteArray kostil_name_device;
    SignalProcessor *processor;
    QTimer *timer;
    core *my_core;
    int count;
    QMessageBox *msgBox;
    Ui::MainWindow *ui;
    Chart *my_chart;

    std::thread adcThread;
    std::atomic<bool> abortFlag{false};

    std::thread modbusThread;
    std::atomic<bool> abortModbusFlag{false};

    bool StateADC;
    void showErrMsgBox(const char *title, const char *msg);
};
#endif // MAINWINDOW_H
