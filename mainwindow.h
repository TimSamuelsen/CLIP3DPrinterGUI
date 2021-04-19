#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "manualstagecontrol.h"
#include "manualprojcontrol.h"
#include "manualpumpcontrol.h"
#include "patternelement.h"
#include "SMC100C.h"
#include "dlp9000.h"

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
    void PrintToTerminal(QString StringToPrint);
    SMC100C SMC;
    //DLP9000 DLP;
    manualpumpcontrol Pump;

public slots:
    void showError(QString errMsg);

private slots:
    void timerTimeout(void);

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

    void on_ClearImageFiles_clicked();

    void ExposureTimeSlot();

    void DarkTimeSlot();

    void PrintProcess();

    void CheckDLPStatus();

    void on_ManualLightEngine_clicked();

    void saveText();

    void on_LogFileBrowse_clicked();

    void on_SetStartingPosButton_clicked();

    void on_AbortPrint_clicked();

    void on_SetIntialAdhesionTimeButton_clicked();

    void on_AutoCheckBox_stateChanged(int arg1);

    void on_setPrintSpeed_clicked();

    void on_pushButton_clicked();

    void on_PumpConnectButton_clicked();

    void on_GetPosition_clicked();

    void on_SetMaxImageUpload_clicked();

    void on_SetPrintHeight_clicked();

    void on_AutoCheckBox_clicked();

    void initStageSlot();

    void fineMovement();

    void verifyStageParams();

    void on_UsePrintScript_clicked();

private:
    Ui::MainWindow *ui;
    ManualStageControl *ManualStageUI;
    manualLEcontrol *ManualProjUI;
    manualpumpcontrol *ManualPumpUI;
    bool ValidateSettings(void);
    void loadSettings();
    void saveSettings();
    void initSettings();
    void initPlot();
    void updatePlot();
    void addPlotData();
    QVector<double> qv_x, qv_y;
    void printParameters();
    void validateStartingPosition();
    bool initConfirmationScreen();
    void AutoMode();
};
#endif // MAINWINDOW_H
