#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "manualstagecontrol.h"
#include "patternelement.h"


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool nanosleep(double ns);

private slots:
    void on_ManualStage_clicked();

    void on_SetSliceThickness_clicked();

    void on_SetStageVelocity_clicked();

    void on_SetStageAcceleration_clicked();

    void on_SetMaxEndOfRun_clicked();

    void on_SetMinEndOfRun_clicked();

    void on_SetExposureTime_clicked();

    void on_SetUVIntensity_clicked();

    void on_SetDarkTime_clicked();

    void on_LiveValueList1_activated(const QString &arg1);

    void on_LiveValueList2_activated(const QString &arg1);

    void on_LiveValueList3_activated(const QString &arg1);

    void on_LiveValueList4_activated(const QString &arg1);

    void on_LiveValueList5_activated(const QString &arg1);

    void on_LiveValueList6_activated(const QString &arg1);

    void on_ResinSelect_activated(const QString &arg1);

    void on_InitializeAndSynchronize_clicked();

    void on_StartPrint_clicked();

    void on_StageConnectButton_clicked();

    void on_LightEngineConnectButton_clicked();

    void on_SelectFile_clicked();


private:
    Ui::MainWindow *ui;
    ManualStageControl *ManualStageUI;
    bool ValidateSettings(void);

};
#endif // MAINWINDOW_H
