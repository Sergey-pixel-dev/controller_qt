#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    StateADC = false;
    ui->setupUi(this);
    my_core = new core();
    my_core->conn_params = new conn_struct{.type = 1,
                                           .com_params = new com_struct{
                                               .device = "/dev/ttyUSB0",
                                               .baud_rate = 115200,

                                               .polarity = 'N',
                                               .data_bits = 8,
                                               .stop_bits = 1,
                                           }};
    timer = new QTimer();
    my_chart = new Chart(ui->horizontalSlider);
    processor = new SignalProcessor();

    connect(timer, SIGNAL(timeout()), this, SLOT(slotTimerAlarm()));
    connect(this,
            &MainWindow::requestChartUpdate,
            this,
            &MainWindow::updateChart,
            Qt::QueuedConnection);

    msgBox = new QMessageBox();

    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    ui->section->setContentLayout(ui->widget_6->layout());
    ui->section->setTitle("Параметры оцифровки");

    ui->section_2->setContentLayout(ui->widget_8->layout());
    ui->section_2->setTitle("Параметры управляющих сигналов");

    ui->comboBox->addItem(QString("ДМ25-400"));
    ui->comboBox->addItem(QString("ДМ25-401"));
    ui->comboBox->addItem(QString("ДМ25-600"));

    ui->comboBox_2->addItem(QString("Импульс У.Э."));
    ui->comboBox_2->addItem(QString("Импульс I К."));
    ui->comboBox_2->addItem(QString("Импульс U К."));

    ui->comboBox_6->addItem(QString("3 мкс."));
    ui->comboBox_6->addItem(QString("6 мкс."));
    ui->comboBox_6->addItem(QString("9 мкс."));
    ui->comboBox_6->setCurrentIndex(2);

    ui->comboBox_4->addItem(QString("x1"));
    ui->comboBox_4->addItem(QString("x4"));
    ui->comboBox_4->addItem(QString("x16"));
    ui->comboBox_4->addItem(QString("x64"));
    ui->comboBox_4->addItem(QString("x256"));

    ui->comboBox_7->addItem(QString("Без фильтра"));
    ui->comboBox_7->addItem(QString("Скользящая средняя"));
    ui->comboBox_7->addItem(QString("КИХ-фильтр нижних частот"));

    ui->chartView->setRubberBand(QChartView::RectangleRubberBand);
    ui->chartView->setChart(my_chart->GetChart());
    ui->spinBox_2->setEnabled(false);
    on_pushButton_3_clicked();
    my_core->fill_std_values();
    UpdateScreenValues();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete my_core;
    delete timer;
    delete my_chart;
    delete processor;
    StateADC = false;
    abortFlag = true;
    if (adcThread.joinable())
        adcThread.join();
}

void MainWindow::on_button_connect_clicked()
{
    if (my_core->status == DISCONNECTED) {
        if (ui->tabWidget->currentIndex() == 0) {
            int a = my_core->init_modbus();
            if (!a) {
                a = my_core->connect_modbus();
                if (!a) {
                    HasBeenConnected();
                    ui->button_connect->setText("Отключить");
                } else
                    showErrMsgBox("Ошибка подключения", my_core->modbus->GetErrMsg(a));

            } else
                showErrMsgBox("Ошибка подключения", my_core->modbus->GetErrMsg(a));
        } else {
            if (!my_core->connect_sport()) {
                ui->button_connect->setText("Отключить");
                HasBeenConnected();

            } else
                showErrMsgBox("Ошибка подключения", "Не удалось подключиться.");
        }

    } else if (my_core->status == CONNECTED_MODBUS) {
        my_core->close_modbus();
        HasBeenDisconnected();
        ui->button_connect->setText("Подключить");
    } else if (my_core->status == CONNECTED_SPORT) {
        if (StateADC)
            adcThreadStop();
        my_core->close_sprot();
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
        ui->pushButton_2->setEnabled(true);
        ui->label_4->setText("Соединение установлено");
    } else {
        showErrMsgBox("Ошибка подключения", "Данные не были получены.");
    }
    timer->start(1000);
}

void MainWindow::HasBeenDisconnected()
{
    timer->stop();
    my_core->fill_std_values();
    UpdateScreenValues();
    QApplication::processEvents();
    start_signal_struct signal_str = my_core->GetSignals();
    ui->spinBox->setValue(signal_str.frequency);
    ui->doubleSpinBox_27->setValue(signal_str.duration / 10.0);
    ui->spinBox_2->setValue(signal_str.interval);
    ui->checkBox->setCheckState(signal_str.IsEnabled ? Qt::Checked : Qt::Unchecked);

    ui->pushButton_2->setEnabled(false);
    ui->pushButton->setEnabled(false);
    ui->checkBox->setEnabled(false);
    ui->label_4->setText("Соединение отсутствует");

    //ui->checkBox_2->setEnabled(false);
}

void MainWindow::slotTimerAlarm()
{
    //исправь потом, в tab index == 1, будет всегда писаться, что соединения нет, хотя это может быть не так
    if (!my_core->UpdateValues()) {
        UpdateScreenValues();
        if (ui->label_4->text() == "Соединение отсутствует") { // сравнение - костыль
            ui->label_4->setText("Соединение установлено");
            HasBeenConnected();
        }
    } else {
        HasBeenDisconnected();
        ui->label_4->setText("Соединение отсутствует");
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
    if (my_core->heater_block.IsReady)
        ui->label_8->setText(QString("ГОТОВ"));
    else
        ui->label_8->setText(QString("НЕ ГОТОВ"));

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
    if (my_core->energy_block.LE_or_HE)
        ui->label_3->setText(QString("Выбрана ВЭ"));
    else
        ui->label_3->setText(QString("Выбрана НЭ"));
    ui->doubleSpinBox_11->setValue(my_core->energy_block.control_he / 1000.0);
    ui->doubleSpinBox_15->setValue(my_core->energy_block.control_he * my_core->coef.coef_u_he_set
                                   / 1000000.0);
    ui->doubleSpinBox_17->setValue(my_core->energy_block.measure_he / 1000.0);
    ui->doubleSpinBox_22->setValue(my_core->energy_block.measure_he * my_core->coef.coef_u_he_meas
                                   / 1000000.0);
    ui->doubleSpinBox_19->setValue(my_core->energy_block.control_le / 1000.0);
    ui->doubleSpinBox_18->setValue(my_core->energy_block.control_le * my_core->coef.coef_u_le_set
                                   / 1000000.0);
    ui->doubleSpinBox_21->setValue(my_core->energy_block.measure_le / 1000.0);
    ui->doubleSpinBox_23->setValue(my_core->energy_block.measure_le * my_core->coef.coef_u_le_meas
                                   / 1000000.0);

    if (my_core->cathode_block.IsEnabled) {
        ui->widget_4->setActive(true);
        ui->label_6->setText(QString("ВКЛ"));
    } else {
        ui->widget_4->setActive(false);
        ui->label_6->setText(QString("ВЫКЛ"));
    }
    ui->doubleSpinBox_24->setValue(my_core->cathode_block.control_cathode / 1000.0);
    ui->doubleSpinBox_25->setValue(my_core->cathode_block.control_cathode
                                   * my_core->coef.coef_u_cat_set / 1000000.0);
    ui->doubleSpinBox_26->setValue(my_core->cathode_block.measure_cathode / 1000.0);
    ui->doubleSpinBox_16->setValue(my_core->cathode_block.measure_cathode
                                   * my_core->coef.coef_u_cat_meas / 1000000.0);
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
        if (my_core->StartSignals() == -2) {
            showErrMsgBox("Ошибка", "Неверные параметры - включить невозможно!");
            ui->checkBox->setCheckState(Qt::Unchecked);
            return;
        }
        ui->checkBox->setText(QString("ВКЛ"));
    } else {
        ui->checkBox->setText(QString("ВЫКЛ"));
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

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 1) {
        if (my_core->status == CONNECTED_MODBUS) {
            my_core->close_modbus();
            HasBeenDisconnected();
            my_core->connect_sport();
            HasBeenConnected();
        }

    } else if (index == 0) {
        if (my_core->status == CONNECTED_SPORT) {
            my_core->close_sprot();
            my_core->init_modbus();
            my_core->connect_modbus(); //возможно проверить результат подключения
            HasBeenConnected();
        }
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    if (my_core->status != CONNECTED_SPORT) {
        showErrMsgBox("Ошибка подключения", "Устройство отключено.");
        return;
    }
    if (!StateADC) {
        ui->section->toggle(false);
        ui->section->SetDisable(true);
        adcThreadStart();
    } else {
        adcThreadStop();
        ui->section->SetDisable(false);
    }
}

void MainWindow::on_pushButton_3_clicked()
{
    ui->comboBox_3->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    if (!ports.isEmpty()) {
        for (const QSerialPortInfo &info : ports) {
            ui->comboBox_3->addItem(info.portName());
        }
    }
}

void MainWindow::on_comboBox_3_currentIndexChanged(int index)
{
#if defined(_WIN32) || defined(_WIN64)
    kostil_name_device = (ui->comboBox_3->currentText()).toLocal8Bit();
#elif defined(__linux__)
    kostil_name_device = ("/dev/" + ui->comboBox_3->currentText()).toLocal8Bit();
#endif
    my_core->conn_params->com_params->device = kostil_name_device.constData();
}

void MainWindow::on_doubleSpinBox_valueChanged(double arg1)
{
    uint16_t i_offset = arg1 * STM32_TIM_FREQ;
    //мб вынести текущий канал в core

    switch (ui->comboBox_2->currentIndex()) {
    case 0:
        my_core->energy_block.impulse_pos = i_offset;
        break;
    case 1:
        my_core->cathode_block.impulse_i_pos = i_offset;
        break;
    case 2:
        my_core->cathode_block.impulse_u_pos = i_offset;
        break;
    }
    {
        QSignalBlocker blocker(ui->horizontalSlider);
        ui->horizontalSlider->setValue(i_offset);
    }
}
void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    uint16_t i_offset = value;

    switch (ui->comboBox_2->currentIndex()) {
    case 0:
        my_core->energy_block.impulse_pos = i_offset;
        break;
    case 1:
        my_core->cathode_block.impulse_i_pos = i_offset;
        break;
    case 2:
        my_core->cathode_block.impulse_u_pos = i_offset;
        break;
    }
    {
        QSignalBlocker blocker(ui->doubleSpinBox);
        ui->doubleSpinBox->setValue(value / (double) STM32_TIM_FREQ);
    }
}

void MainWindow::on_doubleSpinBox_editingFinished()
{
    {
        QSignalBlocker blocker(ui->doubleSpinBox);
        ui->doubleSpinBox->setValue(ui->horizontalSlider->value() / (double) STM32_TIM_FREQ);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    abortFlag = true;
    if (adcThread.joinable())
        adcThread.join();
    event->accept();
}

void MainWindow::on_pushButton_5_clicked() {}

void MainWindow::on_comboBox_6_currentIndexChanged(int index)
{
    bool wasRunning = StateADC;

    if (wasRunning)
        adcThreadStop();
    switch (index) {
    case 0:
        my_core->n_samples = 8;
        my_chart->axisX->setRange(0, 3);

        break;
    case 1:
        my_core->n_samples = 16;
        my_chart->axisX->setRange(0, 6);

        break;
    case 2:
    default:
        my_chart->axisX->setRange(0, 9);
        my_core->n_samples = 24;
    }
    if (wasRunning) {
        adcThreadStart();
    }
}

void MainWindow::on_comboBox_4_currentIndexChanged(int index)
{
    bool wasRunning = StateADC;
    if (wasRunning)
        adcThreadStop();
    my_core->averaging = pow(4, index);
    if (wasRunning)
        adcThreadStart();
}

void MainWindow::on_comboBox_7_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        processor->SetMovingAverageFilter(false, 3);
        processor->SetFIR_Filter(false, 101);
        break;
    case 1:
        processor->SetMovingAverageFilter(true, 3);
        processor->SetFIR_Filter(false, 101);

        break;
    case 2:
        processor->SetMovingAverageFilter(false, 3);
        processor->SetFIR_Filter(true, 101);
        break;
    }
}

void MainWindow::adcThreadLoop()
{
    const int current_n_samples = my_core->n_samples;
    const int current_n_averaging = my_core->averaging;
    uint8_t *buffer = new uint8_t[2 * ADC_FRAME_N * current_n_samples * current_n_averaging];
    int res = 1;
    auto task = std::bind(&MainWindow::handleThreadResult, this);

    while (!abortFlag.load()) {
        for (int i = 0; i < current_n_averaging && res == 1; i++) {
            res = my_core->GetADCBytes(buffer + i * 2 * ADC_FRAME_N * current_n_samples);
        }
        if (res == 1) {
            processor->setData(buffer, 2 * ADC_FRAME_N * current_n_samples * current_n_averaging);
            processor->RawDataToData(current_n_samples, current_n_averaging);

            switch (ui->comboBox_2->currentIndex()) {
            case 0:
                my_core->energy_block.impulse
                    = (my_core->energy_block.impulse
                       + processor->origin_data[my_core->energy_block.impulse_pos])
                      / 2;
                break;
            case 1:
                my_core->cathode_block.impulse_i
                    = (my_core->cathode_block.impulse_i
                       + processor->origin_data[my_core->cathode_block.impulse_i_pos])
                      / 2;
                break;
            case 2:
                my_core->cathode_block.impulse_u
                    = (my_core->cathode_block.impulse_u
                       + processor->origin_data[my_core->cathode_block.impulse_u_pos])
                      / 2;
                break;
            }
            emit requestChartUpdate(processor->GetPoints());
        }
    }
    delete[] buffer;
    if (my_core->StopADCBytes() == 1) {
        StateADC = false;
        QMetaObject::invokeMethod(this, task, Qt::QueuedConnection);
    }
}

void MainWindow::handleThreadResult() {}

void MainWindow::updateChart(QVector<QPointF> points)
{
    if (my_chart) {
        my_chart->DrawChart(points);
        switch (ui->comboBox_2->currentIndex()) {
        case 0:
            my_chart->DrawAverage(
                QPointF(processor->origin_time[my_core->energy_block.impulse_pos] / 1000.0,
                        my_core->energy_block.impulse));
            break;
        case 1:
            my_chart->DrawAverage(
                QPointF(processor->origin_time[my_core->cathode_block.impulse_i_pos] / 1000.0,
                        my_core->cathode_block.impulse_i));
            break;
        case 2:
            my_chart->DrawAverage(
                QPointF(processor->origin_time[my_core->cathode_block.impulse_u_pos] / 1000.0,
                        my_core->cathode_block.impulse_u));
            break;
        }
    }
}

void MainWindow::adcThreadStop()
{
    ui->pushButton_2->setText("Останавливается...");
    ui->pushButton_2->setEnabled(false);
    QApplication::processEvents();
    abortFlag.store(true);
    if (adcThread.joinable()) {
        adcThread.join();
        StateADC = false;
    }
    ui->pushButton_2->setEnabled(true);
    ui->comboBox_2->setEnabled(true);
    ui->pushButton_2->setText("СТАРТ");
}

void MainWindow::adcThreadStart()
{
    my_core->sport.flushReceiver();
    if (my_core->StartADCBytes(ui->comboBox_2->currentIndex() + 1) != 1) {
        showErrMsgBox("Ошибка подключения", "Устройство отключено.");
        return;
    }
    abortFlag.store(false);
    StateADC = true;
    adcThread = std::thread(&MainWindow::adcThreadLoop, this);
    ui->pushButton_2->setText("СТОП");
    ui->comboBox_2->setEnabled(false);
}

void MainWindow::on_pushButton_6_clicked()
{
    my_core->energy_block.impulse = 0;
    my_core->cathode_block.impulse_i = 0;
    my_core->cathode_block.impulse_u = 0;
    if (StateADC) {
        adcThreadStop();
        processor->ClearData();
        adcThreadStart();
    } else
        processor->ClearData();
    emit requestChartUpdate(processor->GetPoints());
}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    switch (index) {
    case 0: {
        QSignalBlocker blocker1(ui->horizontalSlider);
        QSignalBlocker blocker2(ui->doubleSpinBox);
        ui->doubleSpinBox->setValue(my_core->energy_block.impulse_pos / (double) STM32_TIM_FREQ);
        ui->horizontalSlider->setValue(my_core->energy_block.impulse_pos);
        break;
    }
    case 1: {
        QSignalBlocker blocker1(ui->horizontalSlider);
        QSignalBlocker blocker2(ui->doubleSpinBox);
        ui->doubleSpinBox->setValue(my_core->cathode_block.impulse_i_pos / (double) STM32_TIM_FREQ);
        ui->horizontalSlider->setValue(my_core->cathode_block.impulse_i_pos);
        break;
    }

    case 2: {
        QSignalBlocker blocker1(ui->horizontalSlider);
        QSignalBlocker blocker2(ui->doubleSpinBox);
        ui->doubleSpinBox->setValue(my_core->cathode_block.impulse_u_pos / (double) STM32_TIM_FREQ);
        ui->horizontalSlider->setValue(my_core->cathode_block.impulse_u_pos);
        break;
    }
    }
}
