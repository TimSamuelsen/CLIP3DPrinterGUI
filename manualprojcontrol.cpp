#include "manualprojcontrol.h"
#include "ui_manualprojcontrol.h"
#include "API.h"

manualLEcontrol::manualLEcontrol(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::manualLEcontrol)
{
    ui->setupUi(this);
    StatusCheck();

}

manualLEcontrol::~manualLEcontrol()
{
    delete ui;
}

int manualLEcontrol::StatusCheck()
{

    unsigned char HWStatus, SysStatus, MainStatus;
    if (LCR_GetStatus(&HWStatus, &SysStatus, &MainStatus) == 0)
    {
        if(SysStatus & BIT0)
                   ui->InternalMemoryTest->setChecked(true);
        else
                    ui->InternalMemoryTest->setChecked(false);

        if(HWStatus & BIT0)
                    ui->InternalInit->setChecked(false);
        else
                    ui->InternalInit->setChecked(true);
        if(HWStatus & BIT1)
                    ui->ControllerDMD->setChecked(false);
        else
                    ui->ControllerDMD->setChecked(true);

        if(HWStatus & BIT4)
                    ui->SlavePresent->setChecked(true);
        else
                    ui->SlavePresent->setChecked(false);
        if(HWStatus & BIT2)
                    ui->DMDReset->setChecked(false);
        else
                    ui->DMDReset->setChecked(true);

        if(HWStatus & BIT3)
                    ui->ForcedSwapErrorTest->setChecked(false);
        else
                    ui->ForcedSwapErrorTest->setChecked(true);

        if(HWStatus & BIT6)
                    ui->SequencerAbortFlag->setChecked(false);
        else
                    ui->SequencerAbortFlag->setChecked(true);

        if(HWStatus & BIT7)
                    ui->SequencerErrorTest->setChecked(false);
        else
                    ui->SequencerErrorTest->setChecked(true);

        if(MainStatus & BIT0)
                    ui->DMDParked->setChecked(true);
        else
                    ui->DMDParked->setChecked(false);

        if(MainStatus & BIT1)
                    ui->SequencerRunning->setChecked(true);
        else
                    ui->SequencerRunning->setChecked(false);

        if(MainStatus & BIT2)
                    ui->VideoRunning->setChecked(false);
        else
                    ui->VideoRunning->setChecked(true);
        if(MainStatus & BIT3)
                    ui->LockedToExternal->setChecked(true);
        else
                    ui->LockedToExternal->setChecked(false);
        }
}
