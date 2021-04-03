#include "manualstagecontrol.h"
#include "ui_manualstagecontrol.h"
#include "SMC100C.h"
#include "mainwindow.h"

static bool ConnectionFlag = false;

ManualStageControl::ManualStageControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ManualStageControl)
{
    ui->setupUi(this);

}

ManualStageControl::~ManualStageControl()
{
    SMC.SMC100CClose();
    delete ui;
}

void ManualStageControl::on_ConnectButton_clicked()
{
    if (ConnectionFlag == false)
    {
        QString COMSelect = ui->COMPortSelect->currentText();
        QByteArray array = COMSelect.toLocal8Bit();
        char* COM = array.data();
        bool connectTest = SMC.SMC100CInit(COM);
        if (connectTest)
        {
            ui->TerminalOut->append("Stage Connected");
            ui->ConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
            ui->ConnectionIndicator->setText("Connected");
            ui->ConnectButton->setText("Disconnect");
            ConnectionFlag = true;
        }
        else
        {
            ui->TerminalOut->append("Stage Connection Failed");
            ui->ConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
            ui->ConnectionIndicator->setText("Disconnected");
            ui->ConnectButton->setText("Connect");
            ConnectionFlag = false;
        }
    }
    else
    {
        SMC.SMC100CClose();
        ui->TerminalOut->append("Stage Connection Closed");
        ui->ConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->ConnectionIndicator->setText("Disconnected");
        ui->ConnectButton->setText("Connect");
        ConnectionFlag = false;
    }
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

void ManualStageControl::on_GetMinEndOfRun_clicked()
{
    QString CurrentMinEndOfRun = SMC.GetNegativeLimit();
    CurrentMinEndOfRun = CurrentMinEndOfRun.remove(0,3);
    ui->TerminalOut->append("Current Min End of Run: " + CurrentMinEndOfRun);
    ui->CurrentMinEndOfRun->setText(CurrentMinEndOfRun);
}

void ManualStageControl::on_GetMaxEndOfRun_clicked()
{
    QString CurrentMaxEndOfRun  = SMC.GetPositiveLimit();
    CurrentMaxEndOfRun = CurrentMaxEndOfRun.remove(0,3);
    ui->TerminalOut->append("Current Max End of Run: " + CurrentMaxEndOfRun );
    ui->CurrentMaxEndOfRun->setText(CurrentMaxEndOfRun );
}

void ManualStageControl::on_GetVelocity_clicked()
{
    QString CurrentVelocity  = SMC.GetVelocity();
    CurrentVelocity = CurrentVelocity.remove(0,3);
    ui->TerminalOut->append("Current Velocity: " + CurrentVelocity);
    ui->CurrentVelocity->setText(CurrentVelocity);
}

void ManualStageControl::on_GetAcceleration_clicked()
{
    QString CurrentAcceleration  = SMC.GetAcceleration();
    CurrentAcceleration = CurrentAcceleration.remove(0,3);
    ui->TerminalOut->append("Current Acceleration: " + CurrentAcceleration);
    ui->CurrentAcceleration->setText(CurrentAcceleration);
}

void ManualStageControl::on_GetPosition_clicked()
{
    QString CurrentPosition = SMC.GetPosition();
    CurrentPosition = CurrentPosition.remove(0,3);
    ui->TerminalOut->append("Stage is at: " + CurrentPosition);
    ui->CurrentPositionIndicator->setText(CurrentPosition);
}
