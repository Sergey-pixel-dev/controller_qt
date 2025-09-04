#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , appCore(nullptr)
    , signalProcessor(nullptr)
    , my_chart(nullptr)
    , msgBox(nullptr)
{
    ui->setupUi(this);

    // Создаем компоненты
    appCore = new core(this);
    signalProcessor = new SignalProcessor();
    my_chart = new Chart(ui->horizontalSlider);
    msgBox = new QMessageBox(this);

    setupUI();
    connectSignals();

    // Инициализация
    appCore->fill_std_values();
    signalProcessor->ClearData();

    on_doubleSpinBox_14_valueChanged(0.100);
    on_doubleSpinBox_13_valueChanged(0.100);
}

MainWindow::~MainWindow()
{
    delete ui;
    delete my_chart;
    delete signalProcessor;
}

void MainWindow::setupUI()
{
    // Настройка прокрутки
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    // Настройка секций
    ui->section->setContentLayout(ui->widget_6->layout());
    ui->section->setTitle("Параметры оцифровки");
    ui->section_2->setContentLayout(ui->widget_8->layout());
    ui->section_2->setTitle("Параметры управляющих сигналов");

    // Заполнение комбобоксов
    ui->comboBox->addItem(QString("ДМ25-400"));
    ui->comboBox->addItem(QString("ДМ25-401"));
    ui->comboBox->addItem(QString("ДМ25-600"));

    ui->comboBox_2->addItem(QString("Импульс У.Э."));
    ui->comboBox_2->addItem(QString("Импульс I К."));
    ui->comboBox_2->addItem(QString("Импульс U К."));

    ui->comboBox_4->addItem(QString("x1"));
    ui->comboBox_4->addItem(QString("x4"));
    ui->comboBox_4->addItem(QString("x16"));
    ui->comboBox_4->addItem(QString("x64"));
    ui->comboBox_4->addItem(QString("x256"));

    ui->comboBox_7->addItem(QString("Без фильтра"));
    ui->comboBox_7->addItem(QString("Скользящая средняя"));
    ui->comboBox_7->addItem(QString("КИХ-фильтр нижних частот"));

    // Настройка графика
    ui->chartView->setRubberBand(QChartView::RectangleRubberBand);
    ui->chartView->setChart(my_chart->GetChart());
    my_chart->setTimeScale(0.500);
    my_chart->setVoltageScale(0.200);

    // Начальное состояние
    ui->spinBox_2->setEnabled(false);
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    ui->checkBox->setEnabled(false);
    ui->label_4->setText("Соединение отсутствует");

    // Загружаем доступные порты
    on_pushButton_3_clicked();
}

void MainWindow::connectSignals()
{
    // Связываем ADC поток с обработчиком данных
    connect(appCore, &core::adcDataReady, this, &MainWindow::onDataProcessed);

    // Связываем обновление данных с обновлением UI
    connect(appCore, &core::dataUpdated, this, &MainWindow::updateScreenValues);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (appCore->isConnected()) {
        appCore->disconnectDevice();
    }
    event->accept();
}

// UI Event Handlers
void MainWindow::on_button_connect_clicked()
{
    if (!appCore->isConnected()) {
        QString port = "/dev/" + ui->comboBox_3->currentText();
        ConnectResult result = static_cast<ConnectResult>(appCore->connectDevice(port));

        switch (result) {
        case ConnectResult::Success: {
            ui->button_connect->setText("Отключить");
            ui->pushButton->setEnabled(true);
            ui->pushButton_2->setEnabled(true);
            ui->checkBox->setEnabled(true);
            ui->label_4->setText("Соединение установлено");

            // Читаем значения сигналов из регистров
            QSignalBlocker blocker(ui->checkBox);
            ui->spinBox->setValue(appCore->getSignalFrequency());
            ui->doubleSpinBox_27->setValue(appCore->getSignalDuration() / 10.0);
            ui->checkBox->setCheckState(appCore->getSignalEnabled() ? Qt::Checked : Qt::Unchecked);
            ui->checkBox->setText(appCore->getSignalEnabled() ? "ВКЛ" : "ВЫКЛ");

            break;
        }
        case ConnectResult::AlreadyConnected:
            showErrMsgBox("Предупреждение", "Устройство уже подключено");
            break;
        case ConnectResult::DataNotReceived:
            showErrMsgBox("Ошибка", "Данные не были получены");
            break;
        case ConnectResult::ManagerStartError:
            showErrMsgBox("Ошибка", "Ошибка запуска менеджера");
            break;
        case ConnectResult::PortOpenError:
            showErrMsgBox("Ошибка", "Ошибка подключения к порту");
            break;
        }
    } else {
        appCore->disconnectDevice();
        setDisconnectedState();
    }
}

void MainWindow::on_pushButton_2_clicked()
{
    if (!appCore->isConnected()) {
        showErrMsgBox("Ошибка", "Устройство не подключено");
        return;
    }

    if (!appCore->isADCRunning()) {
        if (!appCore->getSignalEnabled()) {
            showErrMsgBox("Ошибка", "Сигнал \"СТАРТ\" не установлен");
            return;
        }

        MeasurementResult result = static_cast<MeasurementResult>(
            appCore->startADC(ui->comboBox_2->currentIndex() + 1));
        handleMeasurementResult(result);
    } else {
        int stopResult = appCore->stopADC();
        if (stopResult == -5) {
            showErrMsgBox("Предупреждение",
                          "Таймаут остановки АЦП - устройство не подтвердило выключение");
        }
        setMeasurementStoppedState();
    }
}

void MainWindow::on_checkBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        SignalResult result = static_cast<SignalResult>(appCore->startSignals());
        switch (result) {
        case SignalResult::Success:
            ui->checkBox->setText("ВКЛ");
            break;
        case SignalResult::InvalidParameters:
            showErrMsgBox("Ошибка", "Неверные параметры - включить невозможно!");
            ui->checkBox->setCheckState(Qt::Unchecked);
            break;
        case SignalResult::NotConnected:
            showErrMsgBox("Ошибка", "Устройство не подключено");
            ui->checkBox->setCheckState(Qt::Unchecked);
            break;
        default:
            showErrMsgBox("Ошибка", "Не удалось запустить сигналы");
            ui->checkBox->setCheckState(Qt::Unchecked);
            break;
        }
    } else {
        SignalResult result = static_cast<SignalResult>(appCore->stopSignals());
        if (result != SignalResult::Success) {
            showErrMsgBox("Ошибка", "Не удалось остановить сигналы");
        }
        ui->checkBox->setText("ВЫКЛ");
    }
}

void MainWindow::on_pushButton_clicked()
{
    SignalResult result = static_cast<SignalResult>(
        appCore->setSignals((uint16_t) ui->spinBox->value(),
                            (uint16_t) (ui->doubleSpinBox_27->value() * 10),
                            (uint16_t) ui->spinBox_2->value()));

    switch (result) {
    case SignalResult::Success:
        // Всё хорошо
        break;
    case SignalResult::InvalidParameters:
        showErrMsgBox("Ошибка", "Неправильные параметры сигналов");
        break;
    case SignalResult::NotConnected:
        showErrMsgBox("Ошибка", "Устройство не подключено");
        break;
    default:
        showErrMsgBox("Ошибка", "Не удалось установить параметры сигналов");
        break;
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

void MainWindow::on_comboBox_activated(int index)
{
    switch (index) {
    case 0:
        appCore->current_model = DM25_400;
        break;
    case 1:
        appCore->current_model = DM25_401;
        break;
    case 2:
        appCore->current_model = DM25_600;
        break;
    }
    appCore->fill_coef();
}

void MainWindow::on_comboBox_2_currentIndexChanged(int index)
{
    {
        std::lock_guard<std::mutex> lock(appCore->modbus_mutex);
        appCore->usRegHoldingBuf[2] = index + 1;
    }
    updateImpulsePosition(index);
    updateChannelLabels(index);
}

void MainWindow::on_comboBox_3_currentIndexChanged(int index)
{
    // Обработчик изменения порта
}

void MainWindow::on_comboBox_4_currentIndexChanged(int index)
{
    {
        std::lock_guard<std::mutex> lock(appCore->modbus_mutex);
        appCore->usRegHoldingBuf[5] = pow(4, index);
    }
}

void MainWindow::on_comboBox_7_currentIndexChanged(int index)
{
    switch (index) {
    case 0: // Без фильтра
        signalProcessor->SetMovingAverageFilter(false, 3);
        signalProcessor->SetFIR_Filter(false, 101);
        signalProcessor->SetThresholdFilter(false, 0);
        break;
    case 1: // Скользящая средняя
        signalProcessor->SetMovingAverageFilter(true, 3);
        signalProcessor->SetFIR_Filter(false, 101);
        signalProcessor->SetThresholdFilter(false, 0);
        break;
    case 2: // КИХ-фильтр
        signalProcessor->SetMovingAverageFilter(false, 3);
        signalProcessor->SetFIR_Filter(true, 101);
        signalProcessor->SetThresholdFilter(false, 0);
        break;
    }
}

void MainWindow::on_doubleSpinBox_valueChanged(double arg1)
{
    uint16_t i_offset = arg1 * STM32_TIM_FREQ;
    switch (ui->comboBox_2->currentIndex()) {
    case 0:
        appCore->energy_impulse_pos = i_offset;
        break;
    case 1:
        appCore->cathode_impulse_i_pos = i_offset;
        break;
    case 2:
        appCore->cathode_impulse_u_pos = i_offset;
        break;
    }

    QSignalBlocker blocker(ui->horizontalSlider);
    ui->horizontalSlider->setValue(i_offset);
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    uint16_t i_offset = value;
    switch (ui->comboBox_2->currentIndex()) {
    case 0:
        appCore->energy_impulse_pos = i_offset;
        break;
    case 1:
        appCore->cathode_impulse_i_pos = i_offset;
        break;
    case 2:
        appCore->cathode_impulse_u_pos = i_offset;
        break;
    }

    QSignalBlocker blocker(ui->doubleSpinBox);
    ui->doubleSpinBox->setValue(value / (double) STM32_TIM_FREQ);
}

void MainWindow::on_doubleSpinBox_editingFinished()
{
    QSignalBlocker blocker(ui->doubleSpinBox);
    ui->doubleSpinBox->setValue(ui->horizontalSlider->value() / (double) STM32_TIM_FREQ);
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    // Обработчик переключения вкладок
}

// Private helper methods
void MainWindow::setDisconnectedState()
{
    ui->button_connect->setText("Подключить");
    ui->pushButton->setEnabled(false);
    ui->pushButton_2->setEnabled(false);
    ui->checkBox->setEnabled(false);
    ui->label_4->setText("Соединение отсутствует");
    setMeasurementStoppedState();

    // Сброс параметров
    ui->doubleSpinBox_27->setValue(0.0);
    ui->spinBox->setValue(0);
    ui->spinBox_2->setValue(0);
    QSignalBlocker blocker(ui->checkBox);
    ui->checkBox->setCheckState(Qt::Unchecked);
    ui->checkBox->setText("ВЫКЛ");
}

void MainWindow::handleMeasurementResult(MeasurementResult result)
{
    switch (result) {
    case MeasurementResult::Success:
        setMeasurementStartedState();
        break;
    case MeasurementResult::NotConnected:
        showErrMsgBox("Ошибка", "Устройство не подключено");
        break;
    case MeasurementResult::SignalNotSet:
        showErrMsgBox("Ошибка", "Сигнал СТАРТ не установлен");
        break;
    case MeasurementResult::AlreadyRunning:
        showErrMsgBox("Ошибка", "Измерение уже запущено");
        break;
    case MeasurementResult::StartError:
        showErrMsgBox("Ошибка", "Ошибка запуска измерения");
        break;
    case MeasurementResult::Timeout: // УНИВЕРСАЛЬНЫЙ таймаут
        showErrMsgBox("Ошибка", "Таймаут запуска АЦП - устройство не подтвердило включение");
        break;
    }
}

void MainWindow::setMeasurementStartedState()
{
    ui->pushButton_2->setText("СТОП");
    ui->label_47->setText("Оцифровка запущена");
    ui->checkBox->setDisabled(true);
}

void MainWindow::setMeasurementStoppedState()
{
    ui->pushButton_2->setText("СТАРТ");
    ui->label_47->setText("Оцифровка остановлена");
    ui->checkBox->setDisabled(false);
}

void MainWindow::updateScreenValues()
{
    std::lock_guard<std::mutex> lock(appCore->modbus_mutex);

    uint16_t discrete = appCore->usDiscreteBuf[0];

    // Heater block (данные из input регистров и discrete)
    bool heater_enabled = (discrete >> 0) & 1;
    bool heater_ready = (discrete >> 4) & 1;
    uint16_t heater_control_i = appCore->usRegInputBuf[1];
    uint16_t heater_measure_i = appCore->usRegInputBuf[5];
    uint16_t heater_measure_u = appCore->usRegInputBuf[7];

    ui->widget->setActive(heater_enabled);
    ui->label->setText(heater_enabled ? "ВКЛ" : "ВЫКЛ");
    ui->label_8->setText(heater_ready ? "ГОТОВ" : "НЕ ГОТОВ");
    ui->doubleSpinBox_1->setValue(heater_control_i / 1000.0);
    ui->doubleSpinBox_3->setValue(heater_control_i * appCore->coef.coef_i_set / 1000.0);
    ui->doubleSpinBox_4->setValue(heater_measure_i / 1000.0);
    ui->doubleSpinBox_5->setValue(heater_measure_i * appCore->coef.coef_i_meas / 1000.0);
    ui->doubleSpinBox_9->setValue(heater_measure_u / 1000.0);
    ui->doubleSpinBox_10->setValue(heater_measure_u * appCore->coef.coef_u_heater_meas / 1000.0);

    // Energy block (данные из input регистров и discrete)
    bool energy_enabled = (discrete >> 1) & 1;
    bool le_or_he = (discrete >> 3) & 1;
    uint16_t energy_control_le = appCore->usRegInputBuf[0];
    uint16_t energy_control_he = appCore->usRegInputBuf[4];
    uint16_t energy_measure_le = appCore->usRegInputBuf[6];
    uint16_t energy_measure_he = appCore->usRegInputBuf[8];

    ui->widget_5->setActive(energy_enabled);
    ui->label_5->setText(energy_enabled ? "ВКЛ" : "ВЫКЛ");
    ui->label_3->setText(le_or_he ? "Выбрана ВЭ" : "Выбрана НЭ");
    ui->doubleSpinBox_11->setValue(energy_control_he / 1000.0);
    ui->doubleSpinBox_15->setValue(energy_control_he * appCore->coef.coef_u_he_set / 1000000.0);
    ui->doubleSpinBox_17->setValue(energy_measure_he / 1000.0);
    ui->doubleSpinBox_22->setValue(energy_measure_he * appCore->coef.coef_u_he_meas / 1000000.0);
    ui->doubleSpinBox_19->setValue(energy_control_le / 1000.0);
    ui->doubleSpinBox_18->setValue(energy_control_le * appCore->coef.coef_u_le_set / 1000000.0);
    ui->doubleSpinBox_21->setValue(energy_measure_le / 1000.0);
    ui->doubleSpinBox_23->setValue(energy_measure_le * appCore->coef.coef_u_le_meas / 1000000.0);

    // Cathode block (данные из input регистров и discrete)
    bool cathode_enabled = (discrete >> 2) & 1;
    uint16_t cathode_control = appCore->usRegInputBuf[3];
    uint16_t cathode_measure = appCore->usRegInputBuf[2];

    ui->widget_4->setActive(cathode_enabled);
    ui->label_6->setText(cathode_enabled ? "ВКЛ" : "ВЫКЛ");
    ui->doubleSpinBox_24->setValue(cathode_control / 1000.0);
    ui->doubleSpinBox_25->setValue(cathode_control * appCore->coef.coef_u_cat_set / 1000000.0);
    ui->doubleSpinBox_26->setValue(cathode_measure / 1000.0);
    ui->doubleSpinBox_16->setValue(cathode_measure * appCore->coef.coef_u_cat_meas / 1000000.0);

    // Impulse values (локальные переменные)
    ui->doubleSpinBox_2->setValue(appCore->energy_impulse / 1000.0);
    ui->doubleSpinBox_7->setValue(appCore->cathode_impulse_i / 1000.0);
    ui->doubleSpinBox_6->setValue(appCore->cathode_impulse_u / 1000.0);

    // Error and success statistics
    ui->label_49->setText(QString::number(appCore->succes_count));
    ui->label_52->setText(QString::number(appCore->error_count));
}

void MainWindow::updateChart(const QVector<QPointF> &points)
{
    if (my_chart && ui->tabWidget->currentIndex() == 1) {
        my_chart->DrawChart(points);

        // Добавление маркеров импульсов
        if (!signalProcessor->origin_time.isEmpty()) {
            switch (ui->comboBox_2->currentIndex()) {
            case 0:
                if (appCore->energy_impulse_pos < signalProcessor->origin_time.size()) {
                    my_chart->DrawAverage(
                        QPointF(signalProcessor->origin_time[appCore->energy_impulse_pos] / 1000.0,
                                appCore->energy_impulse));
                }
                break;
            case 1:
                if (appCore->cathode_impulse_i_pos < signalProcessor->origin_time.size()) {
                    my_chart->DrawAverage(
                        QPointF(signalProcessor->origin_time[appCore->cathode_impulse_i_pos]
                                    / 1000.0,
                                appCore->cathode_impulse_i));
                }
                break;
            case 2:
                if (appCore->cathode_impulse_u_pos < signalProcessor->origin_time.size()) {
                    my_chart->DrawAverage(
                        QPointF(signalProcessor->origin_time[appCore->cathode_impulse_u_pos]
                                    / 1000.0,
                                appCore->cathode_impulse_u));
                }
                break;
            }
        }
    }
}

void MainWindow::onDataProcessed(List<PackageBuf> *queue, int samples, int averaging)
{
    if (!queue) {
        return;
    }
    signalProcessor->setData(queue);
    signalProcessor->RawDataToData(samples, averaging);
    QVector<QPointF> points = signalProcessor->GetPoints();

    updateChart(points);

    updateImpulseValues();
    delete queue;
}

void MainWindow::updateImpulseValues()
{
    if (!signalProcessor->origin_data.isEmpty()) {
        int current_channel;
        {
            std::lock_guard<std::mutex> lock(appCore->modbus_mutex);
            current_channel = appCore->usRegHoldingBuf[2]; // Канал из регистров
        }

        switch (current_channel) {
        case 1: // Импульс У.Э.
            if (appCore->energy_impulse_pos < signalProcessor->origin_data.size()) {
                uint16_t new_value = signalProcessor->origin_data[appCore->energy_impulse_pos];
                appCore->energy_impulse = (appCore->energy_impulse + new_value) / 2;
            }
            break;
        case 2: // Импульс I К.
            if (appCore->cathode_impulse_i_pos < signalProcessor->origin_data.size()) {
                uint16_t new_value = signalProcessor->origin_data[appCore->cathode_impulse_i_pos];
                appCore->cathode_impulse_i = (appCore->cathode_impulse_i + new_value) / 2;
            }
            break;
        case 3: // Импульс U К.
            if (appCore->cathode_impulse_u_pos < signalProcessor->origin_data.size()) {
                uint16_t new_value = signalProcessor->origin_data[appCore->cathode_impulse_u_pos];
                appCore->cathode_impulse_u = (appCore->cathode_impulse_u + new_value) / 2;
            }
            break;
        }
    }
}

void MainWindow::showErrMsgBox(const char *title, const char *msg)
{
    msgBox->setWindowTitle(title);
    msgBox->setText(msg);
    msgBox->setStandardButtons(QMessageBox::Ok);
    msgBox->setStyleSheet("QLabel{font-size: 16px;}");
    msgBox->exec();
}

void MainWindow::updateImpulsePosition(int channelIndex)
{
    QSignalBlocker blocker1(ui->horizontalSlider);
    QSignalBlocker blocker2(ui->doubleSpinBox);

    switch (channelIndex) {
    case 0:
        ui->doubleSpinBox->setValue(appCore->energy_impulse_pos / (double) STM32_TIM_FREQ);
        ui->horizontalSlider->setValue(appCore->energy_impulse_pos);
        break;
    case 1:
        ui->doubleSpinBox->setValue(appCore->cathode_impulse_i_pos / (double) STM32_TIM_FREQ);
        ui->horizontalSlider->setValue(appCore->cathode_impulse_i_pos);
        break;
    case 2:
        ui->doubleSpinBox->setValue(appCore->cathode_impulse_u_pos / (double) STM32_TIM_FREQ);
        ui->horizontalSlider->setValue(appCore->cathode_impulse_u_pos);
        break;
    }
}

void MainWindow::updateChannelLabels(int channelIndex)
{
    switch (channelIndex) {
    case 0:
        ui->label_15->setText("Импульс (выбран)");
        ui->label_18->setText("Импульс I");
        ui->label_16->setText("Импульс U");
        break;
    case 1:
        ui->label_18->setText("Импульс I (выбран)");
        ui->label_16->setText("Импульс U");
        ui->label_15->setText("Импульс");
        break;
    case 2:
        ui->label_16->setText("Импульс U (выбран)");
        ui->label_15->setText("Импульс");
        ui->label_18->setText("Импульс I");
        break;
    }
}

void MainWindow::on_doubleSpinBox_13_valueChanged(double arg1)
{
    my_chart->setVoltageScale(arg1);
}

void MainWindow::on_doubleSpinBox_14_valueChanged(double arg1)
{
    std::lock_guard<std::mutex> lock(appCore->modbus_mutex);
    appCore->usRegHoldingBuf[3] = GRID_DIVISIONS * arg1 * 3 + 3; // n_samples напрямую в регистр
    my_chart->setTimeScale(arg1);
}
void MainWindow::on_doubleSpinBox_12_valueChanged(double arg1)
{
    signalProcessor->voltageShift = arg1 * 1000;
}

void MainWindow::on_doubleSpinBox_8_valueChanged(double arg1)
{
    {
        std::lock_guard<std::mutex> lock2(appCore->modbus_mutex);
        appCore->usRegHoldingBuf[4] = -arg1 * 100;
    }
}
