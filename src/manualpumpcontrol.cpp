#include "manualpumpcontrol.h"
#include "ui_manualpumpcontrol.h"
#include "pumpcommands.h"

PumpCommands& mp_Pump = PumpCommands::Instance();

static bool ConnectionFlag;
static bool UseTargetTime;
static bool UseTargetVolume;

static double TargetTime;
static double TargetVolume;
static double InfusionRate;
static double WithdrawRate;

manualpumpcontrol::manualpumpcontrol(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::manualpumpcontrol)
{
    ui->setupUi(this);
}

manualpumpcontrol::~manualpumpcontrol()
{
    delete ui;
}

/******************************************Main Controls******************************************/

void manualpumpcontrol::on_ConnectButton_clicked()
{
    QString COMSelect = ui->COMSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (mp_Pump.PumpSerial.openDevice(COM,9600) == 1)
    {
        ui->TerminalOut->append("Serial Port Connected");

        ui->ConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->ConnectionIndicator->setText("Connected");

        ConnectionFlag = true;
    }
    else
    {
        ui->TerminalOut->append("Serial Port Connection Failed");

        ui->ConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->ConnectionIndicator->setText("Disconnected");

        ConnectionFlag = false;
    }
}

void manualpumpcontrol::on_SelectTargetTime_clicked() //Maybe add clear target volume here?
{
    UseTargetTime = true;
    UseTargetVolume = false;

    ui->CurrentTargetTime->setEnabled(true);
    ui->NewTargetTime->setEnabled(true);
    ui->GetTargetTime->setEnabled(true);
    ui->SetTargetTime->setEnabled(true);

    ui->CurrentTargetVolume->setEnabled(false);
    ui->NewTargetVolume->setEnabled(false);
    ui->GetTargetVolume->setEnabled(false);
    ui->SetTargetVolume->setEnabled(false);
}

void manualpumpcontrol::on_SelectTargetVolume_clicked() //Maybe add clear target time here?
{
    UseTargetTime = false;
    UseTargetVolume = true;

    ui->CurrentTargetTime->setEnabled(false);
    ui->NewTargetTime->setEnabled(false);
    ui->GetTargetTime->setEnabled(false);
    ui->SetTargetTime->setEnabled(false);

    ui->CurrentTargetVolume->setEnabled(true);
    ui->NewTargetVolume->setEnabled(true);
    ui->GetTargetVolume->setEnabled(true);
    ui->SetTargetVolume->setEnabled(true);
}

void manualpumpcontrol::on_ClearVolume_clicked()
{
    mp_Pump.ClearVolume();
    ui->TerminalOut->append("Clearing Infused and Withdrawn Volume");
}

void manualpumpcontrol::on_ClearTime_clicked()
{
    mp_Pump.ClearTime();
    ui->TerminalOut->append("Clearing Infused and Withdrawn Time");
}

void manualpumpcontrol::on_SendCustom_clicked()
{
   QString WrittenCommand = ui->CustomCommandLine->text();
   mp_Pump.CustomCommand(WrittenCommand);
   ui->TerminalOut->append("Sending Custom Command: " + WrittenCommand);
}

/******************************************Active Controls******************************************/
void manualpumpcontrol::on_StartInfusion_clicked()
{
    mp_Pump.StartInfusion();
    ui->TerminalOut->append("Starting Infusion");
}

void manualpumpcontrol::on_StartWithdraw_clicked()
{
    mp_Pump.StartWithdraw();
    ui->TerminalOut->append("Starting Withdraw");
}

void manualpumpcontrol::on_StopInfusion_clicked()
{
    mp_Pump.Stop();
    ui->TerminalOut->append("Stopping Pump");
}

/******************************************Configuration Controls******************************************/
void manualpumpcontrol::on_GetTargetTime_clicked()
{
    QString T_Time = mp_Pump.GetTargetTime();
    ui->TerminalOut->append(T_Time);
    ui->CurrentTargetTime->setText(T_Time);
}

void manualpumpcontrol::on_SetTargetTime_clicked()
{
    TargetTime = ui->NewTargetTime->value();
    QString TargetTimeString= "Set Target Time to: " + QString::number(TargetTime);
    ui->TerminalOut->append(TargetTimeString);

    mp_Pump.SetTargetTime(TargetTime);
}

void manualpumpcontrol::on_GetTargetVolume_clicked()
{
    QString T_Vol = mp_Pump.GetTargetVolume();
    ui->TerminalOut->append(T_Vol);
    ui->CurrentTargetVolume->setText(T_Vol);
}

void manualpumpcontrol::on_SetTargetVolume_clicked()
{
    TargetVolume = ui->NewTargetVolume->value();
    QString TargetVolumeString= "Set Target Volume to: " + QString::number(TargetVolume);
    ui->TerminalOut->append(TargetVolumeString);

    mp_Pump.SetTargetVolume(TargetVolume);
}

void manualpumpcontrol::on_GetInfuseRate_clicked()
{
    QString I_Rate = mp_Pump.GetInfuseRate();
    ui->TerminalOut->append(I_Rate);
    ui->CurrentInfuseRate->setText(I_Rate);
}

void manualpumpcontrol::on_SetInfuseRate_clicked()
{
    InfusionRate = ui->NewInfuseRateParam->value();
    QString InfuseRateString= "Set Infusion Rate to: " + QString::number(InfusionRate);
    ui->TerminalOut->append(InfuseRateString);

    mp_Pump.SetInfuseRate(InfusionRate);
}

void manualpumpcontrol::on_GetWithdrawRate_clicked()
{
    QString W_Rate = mp_Pump.GetWithdrawRate();
    ui->TerminalOut->append(W_Rate);
    ui->CurrentWithdrawRate->setText(W_Rate);
}

void manualpumpcontrol::on_SetWithdrawRate_clicked()
{
    WithdrawRate = ui->NewWithdrawRateParam->value();
    QString WithdrawRateString= "Set Withdraw Rate to: " + QString::number(WithdrawRate);
    ui->TerminalOut->append(WithdrawRateString);

    mp_Pump.SetWithdrawRate(WithdrawRate);
}


