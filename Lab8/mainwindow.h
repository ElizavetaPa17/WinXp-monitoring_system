#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "winutility.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
    WinUtility* win_itility_;
    QTimer* timer_;
    QVector<double> x_coord_;
    QVector<double> y_coord_mem_usage_;
    QVector<double> y_coord_cpu_usage_;

    void setupConnection();
    void setupDesign();
    void setupPlot();

    void updateBatteryInfo();
    void updateRAMInfo();
    void setProcessRamInfo();
    void updatePlotInfo();
    void updateDriveInfo();

    void setStaticInfo();

signals:
    void signalUpdateDynamicInfo();

private slots:
    void slotDynamicInfoUpdate();
    void slotDynamicInfoSet();
};

#endif // MAINWINDOW_H
