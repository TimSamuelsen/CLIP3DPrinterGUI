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
        CommandBufferUpdate();
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

/******************************************Active Controls******************************************/
void manualpumpcontrol::on_StartInfusion_clicked()
{
    StartInfusion();
    ui->TerminalOut->append("Starting Infusion");
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

    ui->CurrentInfuseRate->setText(T_Time);
}

void manualpumpcontrol::on_SetTargetTime_clicked()
{
    TargetTime = ui->NewTargetTime->value();
    QString TargetTimeString= "Set Target Time to: " + QString::number(TargetTime);
    ui->TerminalOut->append(TargetTimeString);

    SetTargetTime(TargetTime);
    SerialRead();
}

void manualpumpcontrol::on_GetTargetVolume_clicked()
{
    QString T_Vol = GetTargetVolume();

    ui->CurrentInfuseRate->setText(T_Vol);
}

void manualpumpcontrol::on_SetTargetVolume_clicked()
{
    TargetVolume = ui->NewTargetVolume->value();
    QString TargetVolumeString= "Set Target Volume to: " + QString::number(TargetVolume);
    ui->TerminalOut->append(TargetVolumeString);

    SetTargetVolume(TargetVolume);
    SerialRead();
}

void manualpumpcontrol::on_GetInfuseRate_clicked()
{
    QString I_Rate = GetInfuseRate();

    ui->CurrentInfuseRate->setText(I_Rate);
}

void manualpumpcontrol::on_SetInfuseRate_clicked()
{
    InfusionRate = ui->NewInfuseRateParam->value();
    QString InfuseRateString= "Set Infusion Rate to: " + QString::number(InfusionRate);
    ui->TerminalOut->append(InfuseRateString);

    SetInfuseRate(InfusionRate);
    SerialRead();
}

void manualpumpcontrol::on_GetWithdrawRate_clicked()
{
    QString W_Rate = GetWithdrawRate();

    ui->CurrentInfuseRate->setText(W_Rate);
}

void manualpumpcontrol::on_SetWithdrawRate_clicked()
{
    WithdrawRate = ui->NewWithdrawRateParam->value();
    QString WithdrawRateString= "Set Withdraw Rate to: " + QString::number(WithdrawRate);
    ui->TerminalOut->append(WithdrawRateString);

    SetWithdrawRate(WithdrawRate);
    SerialRead();
}

/******************************************Active Commands******************************************/
int manualpumpcontrol::StartInfusion()
{
    QString Command = "irun.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    SerialRead();
    CommandBufferUpdate();
    return returnval;
}

int manualpumpcontrol::StartTargetInfusion() //Is this needed? Replace with withdraw?
{
    QString Command;
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    CommandBufferUpdate();
    return returnval;
}

int manualpumpcontrol::Stop()
{
    QString Command = "stop.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    SerialRead();
    return returnval;
}
/******************************************Set Commands******************************************/
int manualpumpcontrol::SetTargetTime(double T_Time)
{
    QString Command = "ttime " + QString::number(T_Time) + " s.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::SetTargetVolume(double T_Vol)
{
    QString Command = "tvol " + QString::number(T_Vol) + " ul.\r\n";
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
    QString Command = "irate " + QString::number(I_Rate) + " ul/s.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}

int manualpumpcontrol::SetWithdrawRate(double W_Rate)
{
    QString Command = "wrate. " + QString::number(W_Rate) + " ul/s.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PSerial.writeString(CommandToSend);
    return returnval;
}
/******************************************Get Commands******************************************/
QString manualpumpcontrol::GetTargetTime()
{
    QString TargetTime;

    QString Command = "ttime.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadTargetTime = SerialRead();
    if (strlen(ReadTargetTime) > 1)
    {
        TargetTime = ReadTargetTime;
    }
    return TargetTime;
}

QString manualpumpcontrol::GetTargetVolume()
{
    QString TargetVolume;

    QString Command = "tvolume.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadTargetVolume = SerialRead();
    if (strlen(ReadTargetVolume) > 1)
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
    if (strlen(ReadSyringeVolume) > 1)
    {
        SyringeVolume = ReadSyringeVolume;
    }

    return SyringeVolume;
}

QString manualpumpcontrol::GetInfuseRate()
{
    QString InfuseRate;

    QString Command = "irate";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadInfuseRate = SerialRead();
    if (strlen(ReadInfuseRate) > 0)
    {
        InfuseRate = ReadInfuseRate;
    }

    return InfuseRate;
}

QString manualpumpcontrol::GetWithdrawRate()
{
    QString WithdrawRate;

    QString Command = "wrate.\r\n";
    const char* CommandToSend = Command.toLatin1().data();
    PSerial.writeString(CommandToSend);

    char* ReadWithdrawRate = SerialRead();
    if (strlen(ReadWithdrawRate) > 1)
    {
        WithdrawRate = ReadWithdrawRate;
    }

    return WithdrawRate;
}

/******************************************Helper Functions******************************************/
char* manualpumpcontrol::SerialRead()
{
    char* receivedString = "non";
    char finalChar;
    unsigned int maxNbBytes = 20;
    int ReadStatus;
    ReadStatus = PSerial.readString(receivedString,finalChar,maxNbBytes,50);

    if (ReadStatus > 0)
    {
        return receivedString;
    }
    else if(ReadStatus == 0)
    {
        char* errString = "Timeout Reached";
        return  receivedString;
    }
    else if(ReadStatus == -1)
    {
        char* errString = "A";
        return  errString;
    }
    else if(ReadStatus == -2)
    {
        char* errString = "B";
        return  errString;
    }
    else if(ReadStatus == -3)
    {
        char* errString = "Max N bytes reached";
        return  receivedString;
    }

   return receivedString;
}

void manualpumpcontrol::CommandBufferUpdate()
{
    Sleep(20);
    QString BufferCommand3 = ".\r\n";
    const char* CommandToSend3 = BufferCommand3.toLatin1().data();
    PSerial.writeString(CommandToSend3);
    Sleep(20);
    QString BufferCommand1 = "cmd.\r\n";
    const char* CommandToSend1 = BufferCommand1.toLatin1().data();
    PSerial.writeString(CommandToSend1);
    Sleep(20);
    QString BufferCommand2 = "ver.\r\n";
    const char* CommandToSend2 = BufferCommand2.toLatin1().data();
    PSerial.writeString(CommandToSend2);
}

