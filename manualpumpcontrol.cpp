#include "manualpumpcontrol.h"
#include "ui_manualpumpcontrol.h"

#include "serialib.h"
#include "SMC100C.h"

SMC100C SMC;

static bool ConnectionFlag;

static double InfusionRate;

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

void manualpumpcontrol::on_ConnectButton_clicked()
{

    if (SMC.serial.openDevice("COM3",57600) == 1)
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



 /*
void manualPumpControl::setInfusionRate(double InfusionRate)
{

}
*/


void manualpumpcontrol::on_GetInfuseRate_clicked()
{
    InfusionRate = ui->NewInfuseRateParam->value();
    QString ExpDarkRatioString = "Set Infusion Rate to: " + QString::number(InfusionRate);
    ui->TerminalOut->append(ExpDarkRatioString);

    QString Command = "irate.";
    const char* CommandToSend = Command.toLatin1().data();
    SMC.serial.writeString(CommandToSend);
    QString ReadVal = SerialRead();

    ui->NewInfuseRateParam->setValue(ReadVal.toDouble());
    ui->TerminalOut->append("Current Infusion Rate: " + ReadVal);


}

void manualpumpcontrol::on_SetInfuseRate_clicked()
{
    InfusionRate = ui->NewInfuseRateParam->value();
    QString ExpDarkRatioString = "Set Infusion Rate to: " + QString::number(InfusionRate);
    ui->TerminalOut->append(ExpDarkRatioString);

    QString Command = "irate " + QString::number(InfusionRate) + "ul/s.";
    const char* CommandToSend = Command.toLatin1().data();
    SMC.serial.writeString(CommandToSend);
}

char* manualpumpcontrol::SerialRead()
{
    char* receivedString;
    char finalChar;
    unsigned int maxNbBytes = 13;
    int ReadStatus;
    ReadStatus = SMC.serial.readString(receivedString,finalChar,maxNbBytes,250);
/*
    char ReadChar;
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
        char* errString = "Error Setting Timeout";
        return  errString;
    }
    else if(ReadStatus == -2)
    {
        char* errString = "Error while reading byte";
        return  receivedString;
    }
    else if(ReadStatus == -3)
    {
        char* errString = "Max N bytes reached";
        return  receivedString;
    }
    */
   return receivedString;
}
