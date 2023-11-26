#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <sstream>

#include "qcustomplot.h"
#include "QDebug"
#include "QTimer "

const int X_COORD_COUNT = 10;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow),
      win_itility_(WinUtility::getInstance()),
      timer_(NULL)
{
    ui->setupUi(this);
    timer_ = new QTimer(this);

    setupConnection();
    emit signalUpdateDynamicInfo();

    setupDesign();
    setupPlot();

    setStaticInfo();
    timer_->start(1500);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupConnection() {
    connect(timer_, SIGNAL(timeout()), this, SLOT(slotDynamicInfoUpdate()));
    connect(this, SIGNAL(signalUpdateDynamicInfo()), win_itility_, SLOT(slotUpdateProcessInfo()));
    connect(win_itility_, SIGNAL(signalProcessInfoReady()), this, SLOT(slotDynamicInfoSet()));
}

void MainWindow::setupDesign() {
    QStringList hrz_labels;
    hrz_labels << "Process" << "CPU(%)" << "RAM(%)" << "Disk Activity (KB)";
    ui->processUsageTable->setColumnCount(4);
    ui->processUsageTable->setHorizontalHeaderLabels(hrz_labels);
    ui->processUsageTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->tabWidget->setTabText(0, "Dynamic Info");
    ui->tabWidget->setTabText(1, "Static Info");
}

void MainWindow::setupPlot() { 
    x_coord_.resize(X_COORD_COUNT);
    y_coord_mem_usage_.resize(X_COORD_COUNT);
    y_coord_cpu_usage_.resize(X_COORD_COUNT);

    for (int i = 0; i < X_COORD_COUNT; ++i) {
        x_coord_[i] = i;
    }

    ui->dynamicPlotWidget->setLocale(QLocale(QLocale::English, QLocale::UnitedKingdom));
    ui->dynamicPlotWidget->legend->setVisible(true);
    ui->dynamicPlotWidget->addGraph();
    ui->dynamicPlotWidget->addGraph();

    ui->dynamicPlotWidget->graph(0)->setName("RAM Usage");
    ui->dynamicPlotWidget->graph(1)->setName("CPU Usage");

    QPen pen;
    pen.setWidth(3);
    pen.setColor(QColor(Qt::green));
    ui->dynamicPlotWidget->graph(0)->setPen(pen);
    pen.setColor(QColor(Qt::blue));
    ui->dynamicPlotWidget->graph(1)->setPen(pen);

    ui->dynamicPlotWidget->xAxis->setLabel("t");
    ui->dynamicPlotWidget->yAxis->setLabel("%");
    ui->dynamicPlotWidget->xAxis->setRange(0, 10);
    ui->dynamicPlotWidget->yAxis->setRange(0, 100);
}

void MainWindow::updateBatteryInfo() {
    QPalette palette;

    std::vector<BYTE> battery_status = win_itility_->getBatteryStatus();
    switch(battery_status[0]) {
        case 0:
            ui->acPowerLabel->setText("offline");
            palette.setColor(QPalette::WindowText, Qt::darkBlue);
            break;
        case 1:
            ui->acPowerLabel->setText("online");
            palette.setColor(QPalette::WindowText, Qt::darkGreen);
            break;
        default:
            ui->acPowerLabel->setText("unknoww status");
            palette.setColor(QPalette::WindowText, Qt::darkRed);
    }

    ui->acPowerLabel->setPalette(palette);

    switch(battery_status[1]) {
        case 1:
            ui->chargeStatusLabel->setText("high");
            palette.setColor(QPalette::WindowText, Qt::darkGreen);
            break;
        case 2:
            ui->chargeStatusLabel->setText("low");
            palette.setColor(QPalette::WindowText, Qt::darkRed);
            break;
        case 4:
            ui->chargeStatusLabel->setText("critical");
            palette.setColor(QPalette::WindowText, Qt::red);
            break;
        case 8:
            ui->chargeStatusLabel->setText("charging");
            palette.setColor(QPalette::WindowText, Qt::darkGreen);
            break;
        case 128:
            ui->chargeStatusLabel->setText("no system battery");
            palette.setColor(QPalette::WindowText, Qt::darkBlue);
            break;
        default:
            ui->chargeStatusLabel->setText("unknown status");
            palette.setColor(QPalette::WindowText, Qt::black);
    }

    ui->chargeStatusLabel->setPalette(palette);

    if (battery_status[2] >= 66) {
        palette.setColor(QPalette::WindowText, Qt::darkGreen);
    } else if (battery_status[2] <= 33) {
        palette.setColor(QPalette::WindowText, Qt::darkRed);
    } else {
        palette.setColor(QPalette::WindowText, Qt::darkBlue);
    }
    ui->lifePercentLabel->setText(QString::number(battery_status[2]) + "%");
    ui->lifeTimeLabel->setText(QString::number(win_itility_->getBatteryLifeTime()) + "s");
}

void MainWindow::updateRAMInfo() {
    std::vector<DWORDLONG> mem_info = win_itility_->getMemoryInfo();
    ui->ramUsageLabel->setText(QString::number(mem_info[0]) + "%");
    ui->ramAvailabalelLabel->setText(QString::number(mem_info[1]) + "B");

    setProcessRamInfo();
}

void MainWindow::setProcessRamInfo() {
    std::vector<std::string> proc_names = win_itility_->getAllProccessName();
    std::vector<double> proc_cpu_usage  = win_itility_->getAllProcessCPUUsage();
    std::vector<double> proc_mem_usage  = win_itility_->getAllProccessMemoryUsage();
    std::vector<double> proc_disk_act   = win_itility_->getAllProcessDiskActivity();

    ui->processUsageTable->setRowCount(proc_names.size());
    QTableWidgetItem* ptable_item;
    for (int i = 0; i < proc_names.size(); ++i) {
        ptable_item = new QTableWidgetItem(QString(proc_names[i].data()));
        ui->processUsageTable->setItem(i, 0, ptable_item);

        ptable_item = new QTableWidgetItem(QString::number(proc_cpu_usage[i]));
        ui->processUsageTable->setItem(i, 1, ptable_item);

        ptable_item = new QTableWidgetItem(QString::number(proc_mem_usage[i]));
        ui->processUsageTable->setItem(i, 2, ptable_item);

        ptable_item = new QTableWidgetItem(QString::number(proc_disk_act[i]));
        ui->processUsageTable->setItem(i, 3, ptable_item);
    }
}

void MainWindow::updatePlotInfo() {
    std::vector<DWORDLONG> mem_info = win_itility_->getMemoryInfo();
    if (y_coord_mem_usage_.size() == 10) {
        y_coord_mem_usage_.pop_front();
        y_coord_cpu_usage_.pop_front();
    }

    y_coord_mem_usage_.push_back(mem_info[0]);
    y_coord_cpu_usage_.push_back(win_itility_->getCPUUsage());

    ui->dynamicPlotWidget->graph(0)->setData(x_coord_, y_coord_mem_usage_);
    ui->dynamicPlotWidget->graph(1)->setData(x_coord_, y_coord_cpu_usage_);
    ui->dynamicPlotWidget->replot();

    qDebug() << win_itility_->getCPUUsage();
}

void MainWindow::updateDriveInfo() {
    DWORD drive_space;
    win_itility_->getFreeDriveSpace("C:\\", &drive_space);
    ui->driveFreeSizeLabel->setText(QString::number(drive_space) + " B");
}

void MainWindow::setStaticInfo() {
    // general info setup
    std::vector<DWORD> version_info = win_itility_->getOSVersion();
    ui->osVersionLabel->setText(QString::number(version_info[0]) + "." + QString::number(version_info[1]) + " / " + QString::number(version_info[2]));

    WinUtility::ScreenResolution screen_res = win_itility_->getMainScreenRes();
    ui->screenSizeLabel->setText(QString::number(screen_res.width) + " / " + QString::number(screen_res.height));

    LPVOID min_address, max_address;
    win_itility_->getApplicationAddressBorder(&min_address, &max_address);

    std::stringstream str_stream;
    str_stream << (void*)min_address << "-" << (void*)max_address;
    ui->AddressBorderLabel->setText(QString(str_stream.str().data()));

    // cpu info setup
    char p_name[100];
    win_itility_->getVendorName(p_name);
    ui->cpuVendorLabel->setText(QString(p_name));

    win_itility_->getCPUName(p_name);
    ui->cpuModelLabel->setText(QString(p_name));

    ui->cpuCountLabel->setText(QString::number(win_itility_->getCPUCount()));
    ui->firthCPuCoreCountLabel->setText(QString(win_itility_->getFirstCPUCoreCount().data()));

    // battery and ram info setup
    ui->totalRAMSizeLabel->setText(QString::number(win_itility_->getTotalRAMSpace()) + " B");
    ui->batteryFullLifetimeLabel->setText(QString::number(win_itility_->getBatteryFullLifeTime()) + " s");

    // drive info setup
    ui->pageSizeLabel->setText(QString::number(win_itility_->getPageSize()) + " B");
    ui->diskCTypeLabel->setText(QString(win_itility_->getDriveTypeString("C:\\")));

    DWORD size_buff;
    win_itility_->getTotalDriveSpace("C:\\", &size_buff);
    ui->totalDiskCSizeLabel->setText(QString::number(size_buff) + " B");
}
void MainWindow::slotDynamicInfoUpdate() {
    emit signalUpdateDynamicInfo();
}

void MainWindow::slotDynamicInfoSet() {
    updateBatteryInfo();
    updateRAMInfo();
    updatePlotInfo();
    updateDriveInfo();
}
