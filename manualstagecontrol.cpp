#include "manualstagecontrol.h"
#include "ui_manualstagecontrol.h"
#include "SMC100C.h"
#include "mainwindow.h"

//static SMC100C SMC;

//Module variables
ManualStageControl::ManualStageControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ManualStageControl)
{
    ui->setupUi(this);
    SMC.SMC100CInit("COM3");

}

ManualStageControl::~ManualStageControl()
{
    delete ui;
}

void ManualStageControl::on_ConnectButton_clicked()
{
    bool connectTest = SMC.SMC100CInit("COM3");
    if (connectTest)
    {
        ui->TerminalOut->append("Stage Connected");
        ui->ConnectionCheckbox->setChecked(true);
    }
    else
    {
        ui->TerminalOut->append("Stage Connection Failed");
    }
}

void ManualStageControl::GetValues()
{
   // QString CurrentPosition = SMC.GetPosition();
   // CurrentPosition = CurrentPosition.remove(0,3);
   // ui->CurrentPositionIndicator->setText(CurrentPosition);
    //QString CurrentVelocity = SMC.GetVelocity();
    //CurrentVelocity = CurrentVelocity.remove(0,3);
    //ui->CurrentVelocity->setText(CurrentVelocity);

    //QString CurrentAcceleration = SMC.GetAcceleration();
    //CurrentAcceleration = CurrentVelocity.remove(0,3);
    //ui->CurrentAcceleration->setText(CurrentAcceleration);

    //QString CurrentPositiveLimit = SMC.GetPositiveLimit();
    //CurrentPositiveLimit = CurrentPositiveLimit.remove(0,3);
    //ui->CurrentMaxEndOfRun->setText(CurrentPositiveLimit);

   // QString CurrentNegativeLimit = SMC.GetNegativeLimit();
    //CurrentNegativeLimit = CurrentNegativeLimit.remove(0,3);
    //ui->CurrentMinEndOfRun->setText(CurrentNegativeLimit);
}


void ManualStageControl::on_MoveRelative_clicked()
{
    MainWindow Main;
    double RelativeMoveDistance = (ui->RelativeMoveParam->value());
    SMC.RelativeMove(RelativeMoveDistance);
    QString RelativeMoveString = "Relative Move: " + QString::number(RelativeMoveDistance) + "mm";
    ui->TerminalOut->append(RelativeMoveString);
}

void ManualStageControl::on_MoveAbsolute_clicked()
{
    double AbsoluteMoveDistance = (ui->AbsoluteMoveParam->value());
    SMC.AbsoluteMove(AbsoluteMoveDistance);
    QString AbsoluteMoveString = "Absolute Move: " + QString::number(AbsoluteMoveDistance) + "mm";
    ui->TerminalOut->append(AbsoluteMoveString);
}

void ManualStageControl::on_SetMinEndOfRun_clicked()
{
    double MinEndOfRun = (ui->NewMinEndOfRunParam->value());
    SMC.SetNegativeLimit(MinEndOfRun);
    QString MinEndOfRunString =QString::number(MinEndOfRun);
    ui->TerminalOut->append("MinEndOfRun: " + MinEndOfRunString + "mm");
    ui->CurrentMinEndOfRun->setText(MinEndOfRunString);
}

void ManualStageControl::on_SetMaxEndOfRun_clicked()
{
    double MaxEndOfRun = (ui->NewMaxEndOfRunParam->value());
    SMC.SetPositiveLimit(MaxEndOfRun);
    QString MaxEndOfRunString = QString::number(MaxEndOfRun);
    ui->TerminalOut->append("MaxEndOfRun: " + MaxEndOfRunString + "mm");
    ui->CurrentMaxEndOfRun->setText(MaxEndOfRunString);
}

void ManualStageControl::on_SetVelocity_clicked()
{
    double StageVelocity = (ui->NewVelocityParam->value());
    SMC.SetVelocity(StageVelocity);
    QString VelocityString = QString::number(StageVelocity);
    ui->TerminalOut->append("Set Velocity: " + VelocityString  + "mm/s");
    ui->CurrentVelocity->setText(VelocityString);
}

void ManualStageControl::on_SetAcceleration_clicked()
{
    double StageAcceleration = (ui->NewAccelParam->value());
    SMC.SetAcceleration(StageAcceleration);
    QString AccelerationString = QString::number(StageAcceleration);
    ui->TerminalOut->append("Set Acceleration: " + AccelerationString + "mm/s^2");
    ui->CurrentAcceleration->setText(AccelerationString);
}

void ManualStageControl::on_pushButton_clicked()
{
    QString CurrentPosition = SMC.GetPosition();
    CurrentPosition = CurrentPosition.remove(0,3);
    ui->TerminalOut->append("Stage is at: " + CurrentPosition);
    ui->CurrentPositionIndicator->setText(CurrentPosition);
/*
    Sleep(50);

    QString CurrentMinEndOfRun = Main.SMC.GetNegativeLimit();
    ui->CurrentMinEndOfRun->setText(CurrentMinEndOfRun.remove(0,3));

    Sleep(50);

    QString CurrentMaxEndOfRun = Main.SMC.GetPositiveLimit();
    ui->CurrentMaxEndOfRun->setText(CurrentMaxEndOfRun.remove(0,3));

    Sleep(50);

    QString CurrentVelocity = Main.SMC.GetVelocity();
    ui->CurrentVelocity->setText(CurrentVelocity.remove(0,3));

    Sleep(50);

    QString CurrentAcceleration = Main.SMC.GetAcceleration();
    ui->CurrentAcceleration->setText(CurrentAcceleration.remove(0,3));
    */
}


void ManualStageControl::on_pushButton_2_clicked()
{
    SMC.StopMotion();
}

void ManualStageControl::on_pushButton_3_clicked()
{
    SMC.SMC100CClose();
    ui->ConnectionCheckbox->setChecked(false);
}
