#include "manualstagecontrol.h"
#include "ui_manualstagecontrol.h"
#include "SMC100C.h"
#include "mainwindow.h"

#define ON 1
#define OFF 0

static bool ConnectionFlag = false;
static Stage_t StageType = STAGE_SMC; //Initialize stage type as SMC
static bool EndStopState;
static float FeedRate = 60;

ManualStageControl::ManualStageControl(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ManualStageControl)
{
    ui->setupUi(this);

}

ManualStageControl::~ManualStageControl()
{
    //SMC.SMC100CClose();
    StageClose(StageType);
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
        bool connectTest = StageInit(COM, StageType);
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
        StageClose(StageType);
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
    //SMC.RelativeMove(RelativeMoveDistance);
    StageRelativeMove(RelativeMoveDistance, StageType);
    QString RelativeMoveString = "Relative Move: " + QString::number(RelativeMoveDistance) + "mm";
    ui->TerminalOut->append(RelativeMoveString);
}

void ManualStageControl::on_MoveAbsolute_clicked()
{
    double AbsoluteMoveDistance = (ui->AbsoluteMoveParam->value());
    //SMC.AbsoluteMove(AbsoluteMoveDistance);
    StageAbsoluteMove(AbsoluteMoveDistance, StageType);
    QString AbsoluteMoveString = "Absolute Move: " + QString::number(AbsoluteMoveDistance) + "mm";
    ui->TerminalOut->append(AbsoluteMoveString);
}

void ManualStageControl::on_SetMinEndOfRun_clicked()
{
    double MinEndOfRun = (ui->NewMinEndOfRunParam->value());
    //SMC.SetNegativeLimit(MinEndOfRun);
    SetStageNegativeLimit(MinEndOfRun, StageType);
    QString MinEndOfRunString =QString::number(MinEndOfRun);
    ui->TerminalOut->append("MinEndOfRun: " + MinEndOfRunString + "mm");
    ui->CurrentMinEndOfRun->setText(MinEndOfRunString);
}

void ManualStageControl::on_SetMaxEndOfRun_clicked()
{
    double MaxEndOfRun = (ui->NewMaxEndOfRunParam->value());
    //SMC.SetPositiveLimit(MaxEndOfRun);
    SetStagePositiveLimit(MaxEndOfRun, StageType);
    QString MaxEndOfRunString = QString::number(MaxEndOfRun);
    ui->TerminalOut->append("MaxEndOfRun: " + MaxEndOfRunString + "mm");
    ui->CurrentMaxEndOfRun->setText(MaxEndOfRunString);
}

void ManualStageControl::on_SetVelocity_clicked()
{
    double StageVelocity = (ui->NewVelocityParam->value());
    //SMC.SetVelocity(StageVelocity);
    SetStageVelocity(StageVelocity, StageType);
    QString VelocityString = QString::number(StageVelocity);
    ui->TerminalOut->append("Set Velocity: " + VelocityString  + "mm/s");
    ui->CurrentVelocity->setText(VelocityString);
}

void ManualStageControl::on_SetAcceleration_clicked()
{
    double StageAcceleration = (ui->NewAccelParam->value());
    //SMC.SetAcceleration(StageAcceleration);
    SetStageAcceleration(StageAcceleration, StageType);
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
    //QString CurrentPosition = SMC.GetPosition();
    //CurrentPosition = CurrentPosition.remove(0,3);
    QString CurrentPosition = StageGetPosition(StageType);
    ui->TerminalOut->append("Stage is at: " + CurrentPosition);
    ui->CurrentPositionIndicator->setText(CurrentPosition);
}

void ManualStageControl::on_StopMotion_clicked()
{
    //SMC.StopMotion();
    StageStop(StageType);
    ui->TerminalOut->append("Stopping Motion");
}


/*************************Expanded Stage Lib***********************/
int ManualStageControl::StageInit(const char* COMPort, Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.SMC100CInit(COMPort);
        returnVal = 1;
    }
    else if (StageType == STAGE_GCODE){
        if(StageSerial.openDevice(COMPort,115200) == 1){ //If serial connection was succesful
            returnVal = 1; //return 1 for succesful connection
            Sleep(10); //Delay to avoid serial congestion

            //Set stage programming to mm
            QString UnitCommand = "\rG21\r\n";
            int UnitReturn = StageSerial.writeString(UnitCommand.toLatin1().data());
            if (UnitReturn < 0) //if command failed
                returnVal = -1; //return -1 for failed command
            Sleep(10); //Delay to avoid serial congestion

            //Set stage to use incremental movements
            QString IncrementCommand = "G91\r\n";
            int IncrementReturn = StageSerial.writeString(IncrementCommand.toLatin1().data());
            if (IncrementReturn < 0) //if command failed
                returnVal = -1; //return -1 for failed command
            Sleep(10); //Delay to avoid serial congestion

            //Enable steppers
            QString StepperCommand = "M17\r\n";
            int StepperReturn = StageSerial.writeString(StepperCommand.toLatin1().data());
            if (StepperReturn < 0) //if command failed
                returnVal = -1; //return -1 for failed command
            Sleep(500);
            for (uint8_t i = 0; i < 25; i++){
                static char receivedString[] = "ThisIsMyTest";
                char finalChar = '\n';
                uint maxNbBytes = 100;//make sure to validate this
                int ReadStatus = StageSerial.readString(receivedString, finalChar, maxNbBytes, 10);
                ui->TerminalOut->append(receivedString);
                Sleep(10);
            }
        }
        else{ //Serial connection failed
            returnVal = -1;
            printf("GCode stage connection failed");
        }
    }
    return returnVal;
}
int ManualStageControl::StageClose(Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.SMC100CClose();
    }
    else if (StageType == STAGE_GCODE){
        StageSerial.closeDevice();
    }
    return returnVal;
}
int ManualStageControl::StageHome(Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        returnVal = SMC.Home();
    }
    else if (StageType == STAGE_GCODE){
        //Is there a use for a stage home command?
        returnVal = 1;
    }
    return returnVal;
}
int ManualStageControl::StageStop(Stage_t StageType)
{
    int returnVal = 0; //default 0 is an error
    if(StageType == STAGE_SMC){
        SMC.StopMotion();
    }
    else if (StageType == STAGE_GCODE){
        QString STOP = "M0\r";
        int STOPreturn = StageSerial.writeString(STOP.toLatin1().data());
        if (STOPreturn)
            returnVal = -1;
    }
    return returnVal;
}
int ManualStageControl::SetStageVelocity(float VelocityToSet, Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.SetVelocity(VelocityToSet);
    }
    else if (StageType == STAGE_GCODE){
        FeedRate = VelocityToSet * 60;
    }
    return returnVal;
}
int ManualStageControl::SetStageAcceleration(float AccelerationToSet, Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.SetAcceleration(AccelerationToSet);
    }
    else if (StageType == STAGE_GCODE){
        /*QString AccelCommand = "M201 Z" + QString::number(AccelerationToSet) + "\r\n";
        int AccelReturn = StageSerial.writeString(AccelCommand.toLatin1().data());
        if (AccelReturn < 0)
            returnVal = -1;*/
    }
    return returnVal;
}
int ManualStageControl::SetStagePositiveLimit(float PositiveLimit, Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.SetPositiveLimit(PositiveLimit);
    }
    else if (StageType == STAGE_GCODE){

    }
    return returnVal;
}
int ManualStageControl::SetStageNegativeLimit(float NegativeLimit, Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.SetNegativeLimit(NegativeLimit);
    }
    else if (StageType == STAGE_GCODE){

    }
    return returnVal;
}
int ManualStageControl::StageAbsoluteMove(float AbsoluteMovePosition, Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.AbsoluteMove(AbsoluteMovePosition);
    }
    else if (StageType == STAGE_GCODE){
        //Set stage to do absolute move
        QString AbsMoveCommand = "G92\r\nG1 Z" + QString::number(AbsoluteMovePosition) + " F" + QString::number(FeedRate) + "\r\n";
        int AbsMoveReturn = StageSerial.writeString(AbsMoveCommand.toLatin1().data());
        if (AbsMoveReturn < 0) //if command failed
            returnVal = -1; //return -1 for failed command
    }
    return returnVal;
}
int ManualStageControl::StageRelativeMove(float RelativeMoveDistance, Stage_t StageType)
{
    int returnVal = 0;
    if(StageType == STAGE_SMC){
        SMC.RelativeMove(RelativeMoveDistance);
    }
    else if (StageType == STAGE_GCODE){
        QString RelMoveCommand = "G91\r\nG1 Z" + QString::number(-RelativeMoveDistance) + " F" + QString::number(FeedRate) + "\r\n";
        int RelMoveReturn = StageSerial.writeString(RelMoveCommand.toLatin1().data());
        if (RelMoveReturn < 0)
            returnVal = 1;
    }
    return returnVal;
}

char* ManualStageControl::StageGetPosition(Stage_t StageType)
{
    if(StageType == STAGE_SMC){
        return SMC.GetPosition();
    }
    else if (StageType == STAGE_GCODE){

        QString GetPositionCommand = "M114 R\r\n";
        StageSerial.flushReceiver();
        Sleep(5);
        StageSerial.writeString(GetPositionCommand.toLatin1().data());
        static char receivedString[] = "ThisIsMyTest";
        char finalChar = '\n';
        uint maxNbBytes = 100;//make sure to validate this
        Sleep(10);
        StageSerial.readString(receivedString, finalChar, maxNbBytes, 10);
        printf(receivedString);
        return receivedString;

    }
    return "NA";
}

void ManualStageControl::ClearRead()
{
    static char receivedString[] = "ThisIsMyTest";
    char finalChar;
}

void ManualStageControl::on_SetPositionValue_clicked()
{
    double PositionVal = ui->SetPositionParam->value();
    QString SetPositionCommand = "G92 Z" + QString::number(PositionVal) + "\r\n";
    StageSerial.writeString(SetPositionCommand.toLatin1().data());
    ui->TerminalOut->append("Set current position to: " + QString::number(PositionVal));
}

void ManualStageControl::on_SendCustomCommand_clicked()
{
    QString Command = ui->CustomCommandLine->text() + "\r";
    const char* CommandToSend = Command.toLatin1().data();
    StageSerial.writeString(CommandToSend);
    ui->TerminalOut->append("Custom Command: " + ui->CustomCommandLine->text());
}

void ManualStageControl::on_EnableEndStopCheckbox_clicked()
{
    if(ui->EnableEndStopCheckbox->isChecked())
    {
        ui->DisableEndstopCheckbox->setChecked(false);
        EndStopState = ON;
        QString Command = "M121\r\n";
        int returnVal = StageSerial.writeString(Command.toLatin1().data());
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
        int returnVal = StageSerial.writeString(Command.toLatin1().data());
        if (returnVal >= 0){
            ui->TerminalOut->append("Endstops disabled");
        }
        else{
            ui->TerminalOut->append("Failed to send command");
        }
    }
}
