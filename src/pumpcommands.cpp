#include "pumpcommands.h"

/******************************************Active Commands******************************************/
int PumpCommands::StartInfusion()
{
    QString Command = "0irun\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::StartWithdraw()
{
    QString Command = "0wrun\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::Stop()
{
    QString Command = "0stop\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}
/******************************************Set Commands******************************************/
int PumpCommands::SetTargetTime(double T_Time)
{
    QString Command = "0ttime " + QString::number(T_Time) + " s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::SetTargetVolume(double T_Vol)
{
    QString Command = "0tvol " + QString::number(T_Vol) + " ul\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::SetSyringeVolume(double S_Vol)
{
    QString Command;
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::SetInfuseRate(double I_Rate)
{
    QString Command = "0irate " + QString::number(I_Rate) + " ul/s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::SetWithdrawRate(double W_Rate)
{
    QString Command = "0wrate " + QString::number(W_Rate) + " ul/s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}
/******************************************Get Commands******************************************/
QString PumpCommands::GetTargetTime()
{
    QString TargetTime;

    QString Command = "0ttime\r";
    const char* CommandToSend = Command.toLatin1().data();
    PumpSerial.writeString(CommandToSend);

    char* ReadTargetTime = SerialRead();
    if (strnlen(ReadTargetTime,50) > 1)
    {
        TargetTime = ReadTargetTime;
    }
    return TargetTime;
}

QString PumpCommands::GetTargetVolume()
{
    QString TargetVolume;

    QString Command = "0tvolume\r";
    const char* CommandToSend = Command.toLatin1().data();
    PumpSerial.writeString(CommandToSend);

    char* ReadTargetVolume = SerialRead();
    if (strnlen(ReadTargetVolume,50) > 1)
    {
        TargetVolume = ReadTargetVolume;
    }

    return TargetVolume;
}

QString PumpCommands::GetSyringeVolume()
{
    QString SyringeVolume;

    QString Command = "";
    const char* CommandToSend = Command.toLatin1().data();
    PumpSerial.writeString(CommandToSend);

    char* ReadSyringeVolume= SerialRead();
    if (strnlen(ReadSyringeVolume,50) > 1)
    {
        SyringeVolume = ReadSyringeVolume;
    }

    return SyringeVolume;
}

QString PumpCommands::GetInfuseRate()
{
    QString InfuseRate;

    QString Command = "0irate\r";
    const char* CommandToSend = Command.toLatin1().data();
    PumpSerial.writeString(CommandToSend);

    char* ReadInfuseRate = SerialRead();
    if (strnlen(ReadInfuseRate, 50) > 0)
    {
        InfuseRate = ReadInfuseRate;
    }

    return InfuseRate;
}

QString PumpCommands::GetWithdrawRate()
{
    QString WithdrawRate;

    QString Command = "0wrate\r";
    const char* CommandToSend = Command.toLatin1().data();
    PumpSerial.writeString(CommandToSend);

    char* ReadWithdrawRate = SerialRead();
    if (strnlen(ReadWithdrawRate, 50) > 1)
    {
        WithdrawRate = ReadWithdrawRate;
    }

    return WithdrawRate;
}
/******************************************Other Commands******************************************/
int PumpCommands::ClearTime()
{
    QString Command = "0ctime\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::ClearVolume()
{
    QString Command = "0cvolume\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::CustomCommand(QString NewCommand)
{
    QString Command = "0" + NewCommand + "\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

int PumpCommands::IndefiniteRun()
{
    QString Command;
    const char* CommandToSend = Command.toLatin1().data();
    int returnVal = PumpSerial.writeString(CommandToSend);
    return returnVal;
}
/******************************************Helper Functions******************************************/
char* PumpCommands::SerialRead()
{
    //char* receivedString = "non";
    //char finalChar;
    //unsigned int maxNbBytes = 20;
    //int ReadStatus;
    //ReadStatus = PumpSerial.readString(receivedString,finalChar,maxNbBytes,50);

    static char receivedString[] = "ThisIsMyTest";
    char finalChar = '\r';
    unsigned int maxNbBytes = 30;
    int ReadStatus;
    ReadStatus = PumpSerial.readString(receivedString,finalChar,maxNbBytes,10);
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
