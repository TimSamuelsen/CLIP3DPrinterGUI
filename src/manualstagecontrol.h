#ifndef MANUALSTAGECONTROL_H
#define MANUALSTAGECONTROL_H

#include <QWidget>
#include "SMC100C.h"

typedef enum StageType{
    STAGE_SMC,
    STAGE_GCODE,
}Stage_t;

namespace Ui {
class ManualStageControl;
}

class ManualStageControl : public QWidget
{
    Q_OBJECT

public:
    explicit ManualStageControl(QWidget *parent = nullptr);
    ~ManualStageControl();
    SMC100C SMC;
    serialib StageSerial;
private slots:
    void on_MoveRelative_clicked();

    void on_MoveAbsolute_clicked();

    void on_SetMinEndOfRun_clicked();

    void on_SetMaxEndOfRun_clicked();

    void on_SetVelocity_clicked();

    void on_SetAcceleration_clicked();

    void on_ConnectButton_clicked();

    void on_GetMinEndOfRun_clicked();

    void on_GetMaxEndOfRun_clicked();

    void on_GetVelocity_clicked();

    void on_GetAcceleration_clicked();

    void on_GetPosition_clicked();

    void on_StopMotion_clicked();

    void on_SMCSelect_clicked();

    void on_GcodeSelect_clicked();

private:
    Ui::ManualStageControl *ui;
    void GetValues();

    int StageInit(const char* COMPort,Stage_t StageType);
    int StageClose(Stage_t StageType);
    int StageHome(Stage_t StageType);
    int StageStop(Stage_t StageType);
    int SetStageVelocity(float VelocityToSet, Stage_t StageType);
    int SetStageAcceleration(float AccelerationToSet, Stage_t StageType);
    int SetStagePositiveLimit(float PositiveLimit, Stage_t StageType);
    int SetStageNegativeLimit(float NegativeLimit, Stage_t StageType);
    int StageAbsoluteMove(float AbsoluteMovePosition, Stage_t StageType);
    int StageRelativeMove(float RelativeMoveDistance, Stage_t StageType);
    char* StageGetPosition(Stage_t);
};

#endif // MANUALSTAGECONTROL_H
