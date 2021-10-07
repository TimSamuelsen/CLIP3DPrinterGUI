#ifndef MANUALSTAGECONTROL_H
#define MANUALSTAGECONTROL_H

#include <QWidget>
#include "PrintElements.h"


namespace Ui {
class ManualStageControl;
}

class ManualStageControl : public QWidget
{
    Q_OBJECT

public:
    explicit ManualStageControl(QWidget *parent = nullptr);
    ~ManualStageControl();

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

    void on_SetPositionValue_clicked();

    void on_SendCustomCommand_clicked();

    void on_EnableEndStopCheckbox_clicked();

    void on_DisableEndstopCheckbox_clicked();

private:
    Ui::ManualStageControl *ui;

};

#endif // MANUALSTAGECONTROL_H
