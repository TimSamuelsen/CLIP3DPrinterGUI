#include "pumpcommands.h"
#include "PrintElements.h"
// There probably exists a cleaner implementation for this using
// a command libary instead of individual functions, but the
// functions keep it simple for future users
serialib PumpSerial;
/******************************************Active Commands******************************************/
bool PumpCommands::PumpInitConnection(const char* COMport) {
  bool returnVal = false;
  PumpSerial.closeDevice();   // Close any previous pump connections
  if (PumpSerial.openDevice(COMport, 9600) == 1) {
    returnVal = true;
    isConnected = true;
    emit PumpConnect();
  }
  return returnVal;
}

/*!
 * \brief PumpCommands::StartInfusion
 * Starts pump infusion/injection
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::StartInfusion() {
  QString command = "0irun\r";
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::StartWithdraw
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::StartWithdraw() {
  QString command = "0wrun\r";
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::Stop
 * Stops all pump activity.
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::Stop() {
  QString command = "0stop\r";
  return PumpSerial.writeString(command.toLatin1().data());
}
/******************************************Set Commands******************************************/
/*!
 * \brief PumpCommands::SetTargetTime
 * Sets the target time for a withdraw or infusion/injection,
 * in units of seconds
 * \param T_Time
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetTargetTime(double T_Time) {
  QString command = "0ttime " + QString::number(T_Time) + " s\r";
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::SetTargetVolume
 * Sets the target volume to infuse/inject or withdraw
 * \param T_Vol - Target volume to be set in units of ul
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetTargetVolume(double T_Vol) {
  QString command = "0tvol " + QString::number(T_Vol) + " ul\r";
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::SetSyringeVolume
 * Sets the volume for the syringe in use, not currently in use.
 * \param S_Vol - Syringe volume to be set
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetSyringeVolume(double S_Vol) {
  QString command;    // TODO: make this functional
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::SetInfuseRate
 * Sets the infusion/injection rate.
 * \param I_Rate - Infusion/injection rate to be set in units of ul/s
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetInfuseRate(double I_Rate) {
  QString command = "0irate " + QString::number(I_Rate) + " ul/s\r";
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::SetWithdrawRate
 * Sets the withdraw rate.
 * \param W_Rate - Withdraw rate in units of ul/s
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::SetWithdrawRate(double W_Rate) {
  QString command = "0wrate " + QString::number(W_Rate) + " ul/s\r";
  return PumpSerial.writeString(command.toLatin1().data());
}
/******************************************Get Commands******************************************/
/*!
 * \brief PumpCommands::GetTargetTime
 * \return - Returns target time set on pump in units of seconds
 */
QString PumpCommands::GetTargetTime() {
  QString command = "0ttime\r";
  return getCommand(command);
}

/*!
 * \brief PumpCommands::GetTargetVolume
 * \return - Returns the target volume set on the pump,
 *           generally in units of ul/s
 */
QString PumpCommands::GetTargetVolume() {
  QString command = "0tvolume\r";
  return getCommand(command);
}

/*!
 * \brief PumpCommands::GetSyringeVolume
 * \return - Returns the syringe volume set on the pump,
 *           generally in units of ul
 */
QString PumpCommands::GetSyringeVolume() {
  QString command = "";   // TODO: make this work
  return getCommand(command);
}

/*!
 * \brief PumpCommands::GetInfuseRate
 * \return - Returns the infusion/injection rate in units of ul/s
 */
QString PumpCommands::GetInfuseRate() {
  QString command = "0irate\r";
  return getCommand(command);
}

/*!
 * \brief PumpCommands::GetWithdrawRate
 * \return - Returns the withdraw rate in units of ul/s
 */
QString PumpCommands::GetWithdrawRate() {
  QString command = "0wrate\r";
  return getCommand(command);
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
int PumpCommands::ClearTime() {
  QString command = "0ctime\r";
  return PumpSerial.writeString(command.toLatin1().data());
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
int PumpCommands::ClearVolume() {
  QString command = "0cvolume\r";
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::CustomCommand
 * Sends a custom command to the pump, allows user to use
 * commands from the syringe pump manual
 * \param NewCommand
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::CustomCommand(QString NewCommand) {
  QString command = "0" + NewCommand + "\r";
  return PumpSerial.writeString(command.toLatin1().data());
}

/*!
 * \brief PumpCommands::IndefiniteRun
 * Sets the pump to run indefinitely regardless of target
 * \return - Returns 1 if successful, -1 if failed
 */
int PumpCommands::IndefiniteRun() {
  QString command;    //TODO: make this work?
  return PumpSerial.writeString(command.toLatin1().data());
}
/******************************************Helper Functions******************************************/
/*
 * \brief PumpCommands::SerialRead
 * \return
 */
char* PumpCommands::SerialRead() {
  static char receivedString[] = "ThisIsMyTest";
  char finalChar = '\r';
  unsigned int maxNbBytes = 30;
  int ReadStatus;
  ReadStatus = PumpSerial.readString(receivedString, finalChar, maxNbBytes, 10);

  if (ReadStatus > 0) {
    return receivedString;
  } else if(ReadStatus == 0) {
    //Error timeout reached
    return "A";
  } else if(ReadStatus == -1) {
    //Error setting timeout
    return  "B";
  } else if(ReadStatus == -2) {
    //Error while reading byte
    return "C";
  } else if(ReadStatus == -3) {
    //Max N bytes reached, return receivedstring
    return  receivedString;
  }
  return receivedString;
}

void PumpCommands::initPumpParams(InjectionSettings m_InjectionSettings) {
  if (m_InjectionSettings.ContinuousInjection || m_InjectionSettings.SteppedContinuousInjection) {
    SetInfuseRate(m_InjectionSettings.BaseInjectionRate);
    Sleep(10);
    SetTargetTime(0); // Settings target time to 0 results in a indefinite injection
  } else {
    SetInfuseRate(m_InjectionSettings.InfusionRate);
    Sleep(10);
    SetTargetVolume(m_InjectionSettings.InitialVolume);
  }
}

QString PumpCommands::getCommand(QString command) {
  QString returnString = "NA";
  // If sending command was succesful
  if (PumpSerial.writeString(command.toLatin1().data())) {
    char* read = SerialRead();
    // Check to make sure read string isn't empty
    if (strnlen(read, 50) > 1) {
      returnString = read;
    }
  }
  return returnString;
}
