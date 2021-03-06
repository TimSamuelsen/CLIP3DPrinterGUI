#include "manualpumpcontrol.h"
#include "ui_manualpumpcontrol.h"

#include "serialib.h"

#include "SMC100C.h"

SMC100C SMC;
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
    if (PSerial.openDevice(COM,9600) == 1)
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
    ClearVolume();
    ui->TerminalOut->append("Clearing Infused and Withdrawn Volume");
}

void manualpumpcontrol::on_ClearTime_clicked()
{
    ClearTime();
    ui->TerminalOut->append("Clearing Infused and Withdrawn Time");
}

void manualpumpcontrol::on_SendCustom_clicked()
{
   QString WrittenCommand = ui->CustomCommandLine->text();
   CustomCommand(WrittenCommand);
   ui->TerminalOut->append("Sending Custom Command: " + WrittenCommand);
}

/******************************************Active Controls******************************************/
void manualpumpcontrol::on_StartInfusion_clicked()
{
    StartInfusion();
    ui->TerminalOut->append("Starting Infusion");
}

void manualpumpcontrol::on_StartWithdraw_clicked()
{
    StartWithdraw();
    ui->TerminalOut->append("Starting Withdraw");
}

void manualpumpcontrol::on_StopInfusion_clicked()
{
    Stop();
    ui->TerminalOut->append("Stopping Pump");
}

/******************************************Configuration Controls******************************************/
void manualpumpcontrol::on_GetTargetTime_clicked()
{
    QString T_Time = GetTargetTime();
    ui->TerminalOut->append(T_Time);
    ui->CurrentTargetTime->setText(T_Time);
}

void manualpumpcontrol::on_SetTargetTime_clicked()
{
    TargetTime = ui->NewTargetTime->value();
    QString TargetTimeString= "Set Target Time to: " + QString::number(TargetTime);
    ui->TerminalOut->append(TargetTimeString);

    SetTargetTime(TargetTime);
    //SerialRead();
}

void manualpumpcontrol::on_GetTargetVolume_clicked()
{
    QString T_Vol = GetTargetVolume();
    ui->TerminalOut->append(T_Vol);
    ui->CurrentTargetVolume->setText(T_Vol);
}

void manualpumpcontrol::on_SetTargetVolume_clicked()
{
    TargetVolume = ui->NewTargetVolume->value();
    QString TargetVolumeString= "Set Target Volume to: " + QString::number(TargetVolume);
    ui->TerminalOut->append(TargetVolumeString);

    SetTargetVolume(TargetVolume);
    //SerialRead();
}

void manualpumpcontrol::on_GetInfuseRate_clicked()
{
    QString I_Rate = GetInfuseRate();
    ui->TerminalOut->append(I_Rate);
    ui->CurrentInfuseRate->setText(I_Rate);
}

void manualpumpcontrol::on_SetInfuseRate_clicked()
{
    InfusionRate = ui->NewInfuseRateParam->value();
    QString InfuseRateString= "Set Infusion Rate to: " + QString::number(InfusionRate);
    ui->TerminalOut->append(InfuseRateString);

    SetInfuseRate(InfusionRate);
    //SerialRead();
}

void manualpumpcontrol::on_GetWithdrawRate_clicked()
{
    QString W_Rate = GetWithdrawRate();
    ui->TerminalOut->append(W_Rate);
    ui->CurrentWithdrawRate->setText(W_Rate);
}

void manualpumpcontrol::on_SetWithdrawRate_clicked()
{
    WithdrawRate = ui->NewWithdrawRateParam->value();
    QString WithdrawRateString= "Set Withdraw Rate to: " + QString::number(WithdrawRate);
    ui->TerminalOut->append(WithdrawRateString);

    SetWithdrawRate(WithdrawRate);
    //SerialRead();
}

/******************************************Active Commands******************************************/
int manualpumpcontrol::StartInfusion()
{
    QString Command = "0irun\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::StartWithdraw()
{
    QString Command = "0wrun\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}


int manualpumpcontrol::Stop()
{
    QString Command = "0stop\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}
/******************************************Set Commands******************************************/
int manualpumpcontrol::SetTargetTime(double T_Time)
{
    QString Command = "0ttime " + QString::number(T_Time) + " s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::SetTargetVolume(double T_Vol)
{
    QString Command = "0tvol " + QString::number(T_Vol) + " ul\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::SetSyringeVolume(double S_Vol)
{
    QString Command;
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::SetInfuseRate(double I_Rate)
{
    QString Command = "0irate " + QString::number(I_Rate) + " ul/s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::SetWithdrawRate(double W_Rate)
{
    QString Command = "0wrate " + QString::number(W_Rate) + " ul/s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}
/******************************************Get Commands******************************************/
QString manualpumpcontrol::GetTargetTime()
{
    QString TargetTime;

    QString Command = "0ttime\r";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadTargetTime = SerialRead();
    if (strnlen(ReadTargetTime,50) > 1)
    {
        TargetTime = ReadTargetTime;
    }
    return TargetTime;
}

QString manualpumpcontrol::GetTargetVolume()
{
    QString TargetVolume;

    QString Command = "0tvolume\r";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadTargetVolume = SerialRead();
    if (strnlen(ReadTargetVolume,50) > 1)
    {
        TargetVolume = ReadTargetVolume;
    }

    return TargetVolume;
}

QString manualpumpcontrol::GetSyringeVolume()
{
    QString SyringeVolume;

    QString Command = "";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadSyringeVolume= SerialRead();
    if (strnlen(ReadSyringeVolume,50) > 1)
    {
        SyringeVolume = ReadSyringeVolume;
    }

    return SyringeVolume;
}

QString manualpumpcontrol::GetInfuseRate()
{
    QString InfuseRate;

    QString Command = "0irate\r";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadInfuseRate = SerialRead();
    if (strnlen(ReadInfuseRate, 50) > 0)
    {
        InfuseRate = ReadInfuseRate;
    }

    return InfuseRate;
}

QString manualpumpcontrol::GetWithdrawRate()
{
    QString WithdrawRate;

    QString Command = "0wrate\r";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadWithdrawRate = SerialRead();
    if (strnlen(ReadWithdrawRate, 50) > 1)
    {
        WithdrawRate = ReadWithdrawRate;
    }

    return WithdrawRate;
}
/******************************************Other Commands******************************************/
int manualpumpcontrol::ClearTime()
{
    QString Command = "0ctime\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::ClearVolume()
{
    QString Command = "0cvolume\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::CustomCommand(QString NewCommand)
{
    QString Command = "0" + NewCommand + "\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::IndefiniteRun()
{
    QString Command;
    const char* CommandToSend = Command.toLatin1().data();
    int returnVal = PSerial.writeString(CommandToSend);
    return returnVal;
}
/******************************************Helper Functions******************************************/
char* manualpumpcontrol::SerialRead()
{
    //char* receivedString = "non";
    //char finalChar;
    //unsigned int maxNbBytes = 20;
    //int ReadStatus;
    //ReadStatus = PSerial.readString(receivedString,finalChar,maxNbBytes,50);

    static char receivedString[] = "ThisIsMyTest";
    char finalChar = '\r';
    unsigned int maxNbBytes = 30;
    int ReadStatus;
    ReadStatus = PSerial.readString(receivedString,finalChar,maxNbBytes,10);
    //printf("at serialread: %s, status: %d\r\n", receivedString, ReadStatus);

    //char* outputString = '\0';
    if (ReadStatus > 0)
    {
        return receivedString;
    }
    else if(ReadStatus == 0)
    {
        //Error timeout reached
        return "A";
        //return  receivedString;
    }
    else if(ReadStatus == -1)
    {
        //Error setting timeout
        return  "B";
    }
    else if(ReadStatus == -2)
    {
        //Error while reading byte
        return "C";
    }
    else if(ReadStatus == -3)
    {
        //Max N bytes reached, return receivedstring
        return  receivedString;
    }
   return receivedString;
}
