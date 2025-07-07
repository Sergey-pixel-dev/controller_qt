#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    my_core = new core();
    my_core->conn_params = new conn_struct{.type = 1,
                                           .com_params = new com_struct{
                                               .device = "/dev/ttyUSB0",
                                               .baud_rate = 19200,

                                               .polarity = 'N',
                                               .data_bits = 8,
                                               .stop_bits = 1,
                                           }};
    ui->setupUi(this);
    timer = new QTimer();
    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimerAlarm()));

    msgBox = new QMessageBox();

    ui->comboBox->addItem(QString("ДМ25-400"));
    ui->comboBox->addItem(QString("ДМ25-401"));
    ui->comboBox->addItem(QString("ДМ25-600"));

    my_core->fill_std_values();
    UpdateScreenValues();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete my_core;
    delete timer;
}

void MainWindow::on_button_connect_clicked()
{
    if (my_core->status != CONNECTED) {
        int a = my_core->open();
        if (!a) {
            a = my_core->connect();
            if (!a) {
                HasBeenConnected();
                ui->button_connect->setText("Отключить");
            } else
                showErrMsgBox("Ошибка подключения", my_core->modbus->GetErrMsg(a));

        } else
            showErrMsgBox("Ошибка подключения", my_core->modbus->GetErrMsg(a));

    } else if (my_core->status == CONNECTED) {
        HasBeenDisconnected();
        ui->button_connect->setText("Подключить");
    }
}

void MainWindow::HasBeenConnected()
{
    if (!my_core->UpdateValues()) {
        UpdateScreenValues();
        start_signal_struct signal_str = my_core->GetSignals();
        ui->spinBox->setValue(signal_str.frequency);
        ui->doubleSpinBox_27->setValue(signal_str.duration / 10.0);
        ui->spinBox_2->setValue(signal_str.interval);
        ui->checkBox->setCheckState(signal_str.IsEnabled ? Qt::Checked : Qt::Unchecked);
        ui->pushButton->setEnabled(true);
        ui->checkBox->setEnabled(true);
        //ui->checkBox_2->setEnabled(true);
    }
    timer->start(1000);
}

void MainWindow::HasBeenDisconnected()
{    
    timer->stop();
    my_core->close();
    my_core->fill_std_values();
    UpdateScreenValues();

    start_signal_struct signal_str = my_core->GetSignals();
    ui->spinBox->setValue(signal_str.frequency);
    ui->doubleSpinBox_27->setValue(signal_str.duration / 10.0);
    ui->spinBox_2->setValue(signal_str.interval);
    ui->checkBox->setCheckState(signal_str.IsEnabled ? Qt::Checked : Qt::Unchecked);

    ui->pushButton->setEnabled(false);
    ui->checkBox->setEnabled(false);
    //ui->checkBox_2->setEnabled(false);
}

void MainWindow::slotTimerAlarm()
{
    if (!my_core->UpdateValues()) {
        UpdateScreenValues();
    }
}

void MainWindow::UpdateScreenValues()
{
    if (my_core->heater_block.IsEnabled) {
        ui->widget->setActive(true);
        ui->label->setText(QString("ВКЛ"));
    } else {
        ui->widget->setActive(false);
        ui->label->setText(QString("ВЫКЛ"));
    }
    ui->doubleSpinBox_1->setValue(my_core->heater_block.control_i / 1000.0);
    ui->doubleSpinBox_3->setValue(my_core->heater_block.control_i * my_core->coef.coef_i_set
                                  / 1000.0);
    ui->doubleSpinBox_4->setValue(my_core->heater_block.measure_i / 1000.0);
    ui->doubleSpinBox_5->setValue(my_core->heater_block.measure_i * my_core->coef.coef_i_meas
                                  / 1000.0);
    ui->doubleSpinBox_9->setValue(my_core->heater_block.measure_u / 1000.0);
    ui->doubleSpinBox_10->setValue(my_core->heater_block.measure_u
                                   * my_core->coef.coef_u_heater_meas / 1000.0);

    if (my_core->energy_block.IsEnabled) {
        ui->widget_5->setActive(true);
        ui->label_5->setText(QString("ВКЛ"));

    } else {
        ui->widget_5->setActive(false);
        ui->label_5->setText(QString("ВЫКЛ"));
    }
    ui->doubleSpinBox_11->setValue(my_core->energy_block.control_he / 1000.0);
    ui->doubleSpinBox_15->setValue(my_core->energy_block.control_he * my_core->coef.coef_u_he_set
                                   / 1000.0);
    ui->doubleSpinBox_17->setValue(my_core->energy_block.measure_he / 1000.0);
    ui->doubleSpinBox_22->setValue(my_core->energy_block.measure_he * my_core->coef.coef_u_he_meas
                                   / 1000.0);
    ui->doubleSpinBox_19->setValue(my_core->energy_block.control_le / 1000.0);
    ui->doubleSpinBox_18->setValue(my_core->energy_block.control_le * my_core->coef.coef_u_le_set
                                   / 1000.0);
    ui->doubleSpinBox_21->setValue(my_core->energy_block.measure_le / 1000.0);
    ui->doubleSpinBox_23->setValue(my_core->energy_block.measure_le * my_core->coef.coef_u_le_meas
                                   / 1000.0);

    if (my_core->cathode_block.IsEnabled) {
        ui->widget_4->setActive(true);
        ui->label_6->setText(QString("ВКЛ"));
    } else {
        ui->widget_4->setActive(false);
        ui->label_6->setText(QString("ВЫКЛ"));
    }
    ui->doubleSpinBox_24->setValue(my_core->cathode_block.control_cathode / 1000.0);
    ui->doubleSpinBox_25->setValue(my_core->cathode_block.control_cathode
                                   * my_core->coef.coef_u_cat_set / 1000.0);
    ui->doubleSpinBox_26->setValue(my_core->cathode_block.measure_cathode / 1000.0);
    ui->doubleSpinBox_16->setValue(my_core->cathode_block.measure_cathode
                                   * my_core->coef.coef_u_cat_meas / 1000.0);
}

void MainWindow::showErrMsgBox(const char *title, const char *msg)
{
    msgBox->setWindowTitle(title);
    msgBox->setText(msg);
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setStyleSheet("QLabel{font-size: 16px;}");
    int ret = msgBox->exec();

    if (ret == QMessageBox::Ok) {
        // мб что-то добавить
    }
}

void MainWindow::on_comboBox_activated(int index)
{
    switch (index) {
    case 0:
        this->my_core->current_model = DM25_400;
        break;
    case 1:
        this->my_core->current_model = DM25_401;
        break;
    case 2:
        this->my_core->current_model = DM25_600;
        break;
    }
    this->my_core->fill_coef();
}

void MainWindow::on_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        my_core->StartSignals();
    } else {
        my_core->StopSignals();
    }
}

void MainWindow::on_pushButton_clicked()
{
    int a = my_core->SetSignals(start_signal_struct{false,
                                                    (uint16_t) ui->spinBox->value(),
                                                    (uint16_t) (ui->doubleSpinBox_27->value() * 10),
                                                    (uint16_t) ui->spinBox_2->value()});
    if (a == 3) {
        showErrMsgBox("Ошибка", "Неправильные параметры сигналов.");
    } else if (a == 2) {
        showErrMsgBox("Ошибка подключения", "Данные не отправились");
    }
}
