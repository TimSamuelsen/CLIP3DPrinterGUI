#ifndef MANUALSTAGECONTROL_H
#define MANUALSTAGECONTROL_H

#include <QWidget>
#include "SMC100C.h"

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
private slots:
    void on_MoveRelative_clicked();

    void on_MoveAbsolute_clicked();

    void on_SetMinEndOfRun_clicked();

    void on_SetMaxEndOfRun_clicked();

    void on_SetVelocity_clicked();

    void on_SetAcceleration_clicked();

    void on_pushButton_clicked();

    void on_ConnectButton_clicked();

    void on_GetMinEndOfRun_clicked();

    void on_GetMaxEndOfRun_clicked();

    void on_GetVelocity_clicked();

    void on_GetAcceleration_clicked();

private:
    Ui::ManualStageControl *ui;
    void GetValues();
};

#endif // MANUALSTAGECONTROL_H
