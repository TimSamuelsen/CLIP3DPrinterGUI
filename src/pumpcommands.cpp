#include "pumpcommands.h"

/******************************************Active Commands******************************************/
/*!
 * \brief PumpCommands::StartInfusion
 * Starts pump infusion/injection
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::StartInfusion()
{
    QString Command = "0irun\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::StartWithdraw
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::StartWithdraw()
{
    QString Command = "0wrun\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::Stop
 * Stops all pump activity.
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::Stop()
{
    QString Command = "0stop\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}
/******************************************Set Commands******************************************/
/*!
 * \brief PumpCommands::SetTargetTime
 * Sets the target time for a withdraw or infusion/injection,
 * in units of seconds
 * \param T_Time
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetTargetTime(double T_Time)
{
    QString Command = "0ttime " + QString::number(T_Time) + " s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::SetTargetVolume
 * Sets the target volume to infuse/inject or withdraw
 * \param T_Vol - Target volume to be set in units of ul
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetTargetVolume(double T_Vol)
{
    QString Command = "0tvol " + QString::number(T_Vol) + " ul\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::SetSyringeVolume
 * Sets the volume for the syringe in use, not currently in use.
 * \param S_Vol - Syringe volume to be set
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetSyringeVolume(double S_Vol)
{
    QString Command;
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::SetInfuseRate
 * Sets the infusion/injection rate.
 * \param I_Rate - Infusion/injection rate to be set in units of ul/s
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetInfuseRate(double I_Rate)
{
    QString Command = "0irate " + QString::number(I_Rate) + " ul/s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::SetWithdrawRate
 * Sets the withdraw rate.
 * \param W_Rate - Withdraw rate in units of ul/s
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetWithdrawRate(double W_Rate)
{
    QString Command = "0wrate " + QString::number(W_Rate) + " ul/s\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}
/******************************************Get Commands******************************************/
/*!
 * \brief PumpCommands::GetTargetTime
 * \return - Returns target time set on pump in units of seconds
 */
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

/*!
 * \brief PumpCommands::GetTargetVolume
 * \return - Returns the target volume set on the pump,
 *           generally in units of ul/s
 */
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

/*!
 * \brief PumpCommands::GetSyringeVolume
 * \return - Returns the syringe volume set on the pump,
 *           generally in units of ul
 */
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

/*!
 * \brief PumpCommands::GetInfuseRate
 * \return - Returns the infusion/injection rate in units of ul/s
 */
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

/*!
 * \brief PumpCommands::GetWithdrawRate
 * \return - Returns the withdraw rate in units of ul/s
 */
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
/*!
 * \brief PumpCommands::ClearTime
 * Clear the set target time on the pump, this is
 * needed because the pump will store previous actions.
 * i.e. If the target time is set to 5 seconds and you start
 * injection it will inject for 5 seconds, will store that it has
 * hit it's target and so will not inject again until the target
 * has been moved or cleared.
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::ClearTime()
{
    QString Command = "0ctime\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::ClearVolume
 * Clear the set target volume on the pump, this is
 * needed because the pump will store previous actions.
 * i.e. If the target volume is set to 5 ml and you start
 * injection it will inject 5 ml, will store that it has
 * hit it's target and so will not inject again until the target
 * has been moved or cleared.
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::ClearVolume()
{
    QString Command = "0cvolume\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::CustomCommand
 * Sends a custom command to the pump, allows user to use
 * commands from the syringe pump manual
 * \param NewCommand
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::CustomCommand(QString NewCommand)
{
    QString Command = "0" + NewCommand + "\r";
    const char* CommandToSend = Command.toLatin1().data();
    int returnval = PumpSerial.writeString(CommandToSend);
    return returnval;
}

/*!
 * \brief PumpCommands::IndefiniteRun
 * Sets the pump to run indefinitely regardless of target
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::IndefiniteRun()
{
    QString Command;
    const char* CommandToSend = Command.toLatin1().data();
    int returnVal = PumpSerial.writeString(CommandToSend);
    return returnVal;
}
/******************************************Helper Functions******************************************/
/*
 * \brief PumpCommands::SerialRead
 * \return
 */
char* PumpCommands::SerialRead()
{
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
