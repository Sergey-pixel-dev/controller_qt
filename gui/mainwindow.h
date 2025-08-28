#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QMainWindow>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QSignalBlocker>
#include <QTimer>
#include <QVariant>
#include <cmath>

#include "../core/core.h"
#include "../core/signalprocessor.h"
#include "../helping/common_macro.h"
#include "../helping/errors.h"
#include "chart.h"

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
    // UI события
    void on_button_connect_clicked();
    void on_pushButton_clicked();   // Установить сигналы
    void on_pushButton_2_clicked(); // Старт/стоп оцифровки
    void on_pushButton_3_clicked(); // Обновить порты
    void on_pushButton_6_clicked(); // Очистить данные
    void on_checkBox_checkStateChanged(const Qt::CheckState &arg1);
    void on_tabWidget_currentChanged(int index);

    // Комбобоксы
    void on_comboBox_activated(int index);             // Модель устройства
    void on_comboBox_2_currentIndexChanged(int index); // Канал измерения
    void on_comboBox_3_currentIndexChanged(int index); // Серийный порт
    void on_comboBox_4_currentIndexChanged(int index); // Усреднение
    void on_comboBox_6_currentIndexChanged(int index); // Время измерения
    void on_comboBox_7_currentIndexChanged(int index); // Фильтр

    // Элементы управления позицией импульса
    void on_doubleSpinBox_valueChanged(double arg1);
    void on_horizontalSlider_valueChanged(int value);
    void on_doubleSpinBox_editingFinished();

    // Асинхронные обновления
    void updateChart(const QVector<QPointF> &points);
    void onDataProcessed(List<PackageBuf> *queue, int samples, int averaging);

private:
    void setupUI();
    void connectSignals();
    void showErrMsgBox(const char *title, const char *msg);
    void updateImpulsePosition(int channelIndex);
    void updateChannelLabels(int channelIndex);

    // Вспомогательные методы для управления состоянием
    void setDisconnectedState();
    void handleMeasurementResult(MeasurementResult result);
    void setMeasurementStartedState();
    void setMeasurementStoppedState();
    void updateImpulseValues();
    void updateScreenValues();

    Ui::MainWindow *ui;
    core *appCore;
    SignalProcessor *signalProcessor;
    Chart *my_chart;
    QMessageBox *msgBox;
};

#endif // MAINWINDOW_H
