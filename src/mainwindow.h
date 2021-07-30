#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "manualstagecontrol.h"
#include "manualpumpcontrol.h"
#include "imageprocessing.h"
#include "imagepopout.h"
#include "patternelement.h"
#include "SMC100C.h"
#include "dlp9000.h"
#include "serialib.h"
#include "PrintElements.h"

typedef enum Parameter{
    EXPOSURE_TIME,
    LED_INTENSITY,
    DARK_TIME,
    LAYER_THICKNESS,
    STAGE_VELOCITY,
    STAGE_ACCELERATION,
    PUMP_HEIGHT,
    INJECTION_VOLUME,
    INJECTION_RATE,
    INITIAL_VOLUME,
    MAX_IMAGE,
    CONTINUOUS_INJECTION,
    STARTING_POSITION,
    MAX_END,
    MIN_END,
    INJECTION_DELAY,
}Parameter_t;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void PrintToTerminal(QString StringToPrint);
    imagepopout popout;
    manualpumpcontrol Pump;
    ManualStageControl Stage;



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

    void on_SelectPrintScript_clicked();

    void on_ClearPrintScript_clicked();

    void on_ImageProcess_clicked();

    void on_POTFcheckbox_clicked();

    void on_VP_HDMIcheckbox_clicked();

    void Check4VideoLock();

    void on_DICLIPSelect_clicked();

    void PrintProcessVP();

    void on_SetBitDepth_clicked();

    void on_SteppedMotion_clicked();

    void on_ContinuousMotion_clicked();

    void on_pumpingCheckBox_clicked();

    void on_setPumping_clicked();

    void pumpingSlot();
    void on_CLIPSelect_clicked();

    void on_SetInfuseRate_clicked();

    void on_SetVolPerLayer_clicked();

    void on_ContinuousInjection_clicked();

    void on_SetInitialVolume_clicked();

    void on_PreMovementCheckbox_clicked();

    void on_PostMovementCheckbox_clicked();

    void on_SetInjectionDelay_clicked();

    void PrintInfuse();

    void StageMove();

private:
    Ui::MainWindow *ui;
    ManualStageControl *ManualStageUI;
    manualpumpcontrol *ManualPumpUI;
    imageprocessing *ImageProcessUI;
    imagepopout *ImagePopoutUI;
    QList<PrintScripts> m_PrintScript;
    QList<PrintSettings> m_PrintSettings;
    QList<PrintControls> m_PrintControls;
    QList<InjectionSettings> m_InjectionSettings;
    QList<MotionSettings> m_MotionSetttings;

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
    void initImagePopout();
    void SetExposureTimer(int InitialExposureFlag, int PrintScript, int PumpingMode);
    void SetDarkTimer(int PrintScript, int MotionMode);
    void VP8bitWorkaround();
    bool PrintScriptApply(uint layerCount, QStringList Script, Parameter_t DynamicParam);
    void PrintComplete();
    void EnableParameter(Parameter_t Parameter, bool State);
};
#endif // MAINWINDOW_H
