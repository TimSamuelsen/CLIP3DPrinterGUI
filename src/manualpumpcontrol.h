#ifndef MANUALPUMPCONTROL_H
#define MANUALPUMPCONTROL_H

#include <QWidget>
#include "serialib.h"

namespace Ui {
class manualpumpcontrol;
}

class manualpumpcontrol : public QWidget
{
    Q_OBJECT

public:
    explicit manualpumpcontrol(QWidget *parent = nullptr);
    ~manualpumpcontrol();

private slots:
    void on_ConnectButton_clicked();

    void on_GetInfuseRate_clicked();

    void on_SetInfuseRate_clicked();

    void on_SelectTargetTime_clicked();

    void on_SelectTargetVolume_clicked();

    void on_GetTargetVolume_clicked();

    void on_SetTargetVolume_clicked();

    void on_GetTargetTime_clicked();

    void on_SetTargetTime_clicked();

    void on_GetWithdrawRate_clicked();

    void on_SetWithdrawRate_clicked();

    void on_StartInfusion_clicked();

    void on_StopInfusion_clicked();

    void on_StartWithdraw_clicked();

    void on_ClearVolume_clicked();

    void on_ClearTime_clicked();

    void on_SendCustom_clicked();

private:
    Ui::manualpumpcontrol *ui;
};

#endif // MANUALPUMPCONTROL_H
