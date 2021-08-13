#include "manualstagecontrol.h"
#include "ui_manualstagecontrol.h"
#include "stagecommands.h"
#include <QTimer>

#define ON 1
#define OFF 0

StageCommands& ms_Stage = StageCommands::Instance();

static bool ConnectionFlag = false;
static Stage_t StageType = STAGE_SMC; //Initialize stage type as SMC
static bool EndStopState;


ManualStageControl::ManualStageControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ManualStageControl)
{
    ui->setupUi(this);
}

ManualStageControl::~ManualStageControl()
{
    ms_Stage.StageClose(StageType);
    delete ui;
}

void ManualStageControl::on_SMCSelect_clicked()
{
    if (ui->SMCSelect->isChecked())
    {
        StageType = STAGE_SMC;
        ui->GcodeSelect->setChecked(false);
    }
}

void ManualStageControl::on_GcodeSelect_clicked()
{
    if (ui->GcodeSelect->isChecked())
    {
        StageType = STAGE_GCODE;
        ui->SMCSelect->setChecked(false);
    }
}

void ManualStageControl::on_ConnectButton_clicked()
{
    if (ConnectionFlag == false)
    {
        QString COMSelect = ui->COMPortSelect->currentText();
        QByteArray array = COMSelect.toLocal8Bit();
        char* COM = array.data();
        //bool connectTest = SMC.SMC100CInit(COM);
        bool connectTest = ms_Stage.StageInit(COM, StageType);
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
        ms_Stage.StageClose(StageType);
        ui->TerminalOut->append("Stage Connection Closed");
        ui->ConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->ConnectionIndicator->setText("Disconnected");
        ui->ConnectButton->setText("Connect");
        ConnectionFlag = false;
    }
}

void ManualStageControl::on_MoveRelative_clicked()
{
    double RelativeMoveDistance = (ui->RelativeMoveParam->value());
    //SMC.RelativeMove(RelativeMoveDistance);
    ms_Stage.StageRelativeMove(RelativeMoveDistance, StageType);
    QString RelativeMoveString = "Relative Move: " + QString::number(RelativeMoveDistance) + "mm";
    ui->TerminalOut->append(RelativeMoveString);
}

void ManualStageControl::on_MoveAbsolute_clicked()
{
    double AbsoluteMoveDistance = (ui->AbsoluteMoveParam->value());
    ms_Stage.StageAbsoluteMove(AbsoluteMoveDistance, StageType);
    QString AbsoluteMoveString = "Absolute Move: " + QString::number(AbsoluteMoveDistance) + "mm";
    ui->TerminalOut->append(AbsoluteMoveString);
}

void ManualStageControl::on_SetMinEndOfRun_clicked()
{
    double MinEndOfRun = (ui->NewMinEndOfRunParam->value());
    ms_Stage.SetStageNegativeLimit(MinEndOfRun, StageType);
    QString MinEndOfRunString =QString::number(MinEndOfRun);
    ui->TerminalOut->append("MinEndOfRun: " + MinEndOfRunString + "mm");
    ui->CurrentMinEndOfRun->setText(MinEndOfRunString);
}

void ManualStageControl::on_SetMaxEndOfRun_clicked()
{
    double MaxEndOfRun = (ui->NewMaxEndOfRunParam->value());
    ms_Stage.SetStagePositiveLimit(MaxEndOfRun, StageType);
    QString MaxEndOfRunString = QString::number(MaxEndOfRun);
    ui->TerminalOut->append("MaxEndOfRun: " + MaxEndOfRunString + "mm");
    ui->CurrentMaxEndOfRun->setText(MaxEndOfRunString);
}

void ManualStageControl::on_SetVelocity_clicked()
{
    double StageVelocity = (ui->NewVelocityParam->value());
    ms_Stage.SetStageVelocity(StageVelocity, StageType);
    QString VelocityString = QString::number(StageVelocity);
    ui->TerminalOut->append("Set Velocity: " + VelocityString  + "mm/s");
    ui->CurrentVelocity->setText(VelocityString);
}

void ManualStageControl::on_SetAcceleration_clicked()
{
    double StageAcceleration = (ui->NewAccelParam->value());
    ms_Stage.SetStageAcceleration(StageAcceleration, StageType);
    QString AccelerationString = QString::number(StageAcceleration);
    ui->TerminalOut->append("Set Acceleration: " + AccelerationString + "mm/s^2");
    ui->CurrentAcceleration->setText(AccelerationString);
}

void ManualStageControl::on_GetMinEndOfRun_clicked()
{
    QString CurrentMinEndOfRun = ms_Stage.SMC.GetNegativeLimit();
    CurrentMinEndOfRun = CurrentMinEndOfRun.remove(0,3);
    ui->TerminalOut->append("Current Min End of Run: " + CurrentMinEndOfRun);
    ui->CurrentMinEndOfRun->setText(CurrentMinEndOfRun);
}

void ManualStageControl::on_GetMaxEndOfRun_clicked()
{
    QString CurrentMaxEndOfRun  = ms_Stage.SMC.GetPositiveLimit();
    CurrentMaxEndOfRun = CurrentMaxEndOfRun.remove(0,3);
    ui->TerminalOut->append("Current Max End of Run: " + CurrentMaxEndOfRun );
    ui->CurrentMaxEndOfRun->setText(CurrentMaxEndOfRun );
}

void ManualStageControl::on_GetVelocity_clicked()
{
    QString CurrentVelocity  = ms_Stage.SMC.GetVelocity();
    CurrentVelocity = CurrentVelocity.remove(0,3);
    ui->TerminalOut->append("Current Velocity: " + CurrentVelocity);
    ui->CurrentVelocity->setText(CurrentVelocity);
}

void ManualStageControl::on_GetAcceleration_clicked()
{
    QString CurrentAcceleration  = ms_Stage.SMC.GetAcceleration();
    CurrentAcceleration = CurrentAcceleration.remove(0,3);
    ui->TerminalOut->append("Current Acceleration: " + CurrentAcceleration);
    ui->CurrentAcceleration->setText(CurrentAcceleration);
}

void ManualStageControl::on_GetPosition_clicked()
{
    //QString CurrentPosition = SMC.GetPosition();
    //CurrentPosition = CurrentPosition.remove(0,3);
    QString CurrentPosition = ms_Stage.StageGetPosition(StageType);
    ui->TerminalOut->append("Stage is at: " + CurrentPosition);
    ui->CurrentPositionIndicator->setText(CurrentPosition);
}

void ManualStageControl::on_StopMotion_clicked()
{
    //SMC.StopMotion();
    ms_Stage.StageStop(StageType);
    ui->TerminalOut->append("Stopping Motion");
}

void ManualStageControl::on_SetPositionValue_clicked()
{
    double PositionVal = ui->SetPositionParam->value();
    QString SetPositionCommand = "G92 Z" + QString::number(PositionVal) + "\r\n";
    ms_Stage.StageSerial.writeString(SetPositionCommand.toLatin1().data());
    ui->TerminalOut->append("Set current position to: " + QString::number(PositionVal));
}

void ManualStageControl::on_SendCustomCommand_clicked()
{
    QString Command = ui->CustomCommandLine->text() + "\r";
    const char* CommandToSend = Command.toLatin1().data();
    ms_Stage.StageSerial.writeString(CommandToSend);
    ui->TerminalOut->append("Custom Command: " + ui->CustomCommandLine->text());
}

void ManualStageControl::on_EnableEndStopCheckbox_clicked()
{
    if(ui->EnableEndStopCheckbox->isChecked())
    {
        ui->DisableEndstopCheckbox->setChecked(false);
        EndStopState = ON;
        QString Command = "M121\r\n";
        int returnVal = ms_Stage.StageSerial.writeString(Command.toLatin1().data());
        if (returnVal >= 0){
            ui->TerminalOut->append("Endstop enabled");
        }
        else{
            ui->TerminalOut->append("Failed to send command");
        }
    }
}

void ManualStageControl::on_DisableEndstopCheckbox_clicked()
{
    if(ui->DisableEndstopCheckbox->isChecked())
    {
        ui->EnableEndStopCheckbox->setChecked(false);
        EndStopState = OFF;
        QString Command = "M120\r\n";
        int returnVal = ms_Stage.StageSerial.writeString(Command.toLatin1().data());
        if (returnVal >= 0){
            ui->TerminalOut->append("Endstops disabled");
        }
        else{
            ui->TerminalOut->append("Failed to send command");
        }
    }
}
