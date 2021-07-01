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
    serialib PSerial;

    int StartInfusion();
    int StartWithdraw();
    int Stop();

    int SetTargetTime(double T_Time);
    int SetTargetVolume(double T_Vol);
    int SetSyringeVolume(double S_Vol);
    int SetInfuseRate(double I_Rate);
    int SetWithdrawRate(double W_Rate);

    int ClearTime();
    int ClearVolume();
    int CustomCommand(QString NewCommand);
    int IndefiniteRun();

    QString GetTargetTime();
    QString GetTargetVolume();
    QString GetSyringeVolume();
    QString GetInfuseRate();
    QString GetWithdrawRate();

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
    char* SerialRead();
    void CommandBufferUpdate();
};

#endif // MANUALPUMPCONTROL_H
