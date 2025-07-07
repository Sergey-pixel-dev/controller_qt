#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
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

private:
    void UpdateScreenValues();
    void HasBeenConnected();
    void HasBeenDisconnected();
    QTimer *timer;
    core *my_core;
    QMessageBox *msgBox;
    Ui::MainWindow *ui;
    void showErrMsgBox(const char *title, const char *msg);
};
#endif // MAINWINDOW_H
