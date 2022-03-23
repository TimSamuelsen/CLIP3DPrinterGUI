#include "stagecommands.h"
#include <QTimer>

static float FeedRate = 60;
//static bool StagePrep1 = false;
//static bool StagePrep2 = false;
PrintSettings s_PrintSettings;
serialib StageSerial;

/*!
 * \brief StageCommands::StageInit
 * Initializes the stage serial port connection.
 * \param COMPort - Select which COM port to connect to.
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful connection, -1 if failed connection
 */
int StageCommands::StageInit(const char* COMPort, Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    returnVal = SMC.SMC100CInit(COMPort);
  } else if (StageType == STAGE_GCODE) {
    if(StageSerial.openDevice(COMPort, 115200) == 1) { //If serial connection was successful
      returnVal = 1; //return 1 for successful connection
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
      StageSerial.flushReceiver();
      // TODO: can this be replaced by flushing the receiver??
      /*for (uint8_t i = 0; i < 25; i++){
          static char receivedString[] = "ThisIsMyTest";
          char finalChar = '\n';
          uint maxNbBytes = 100;//make sure to validate this
          int ReadStatus = StageSerial.readString(receivedString, finalChar, maxNbBytes, 10);
          Sleep(10);
      }*/
    } else { //Serial connection failed
      returnVal = -1;
      printf("GCode stage connection failed");
    }
  }
  if (returnVal == 1) {
    isConnected = true;
    emit StageConnect();
  }
  return returnVal;
}

/*!
 * \brief StageCommands::StageClose
 * Closes stage serial port connection.
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::StageClose(Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    SMC.SMC100CClose();
  } else if (StageType == STAGE_GCODE) {
    StageSerial.closeDevice();
  }
  return returnVal;
}

/*!
 * \brief StageCommands::StageHome
 * Homes the stage to provide a reference point, only used for STAGE_SMC
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::StageHome(Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    returnVal = SMC.Home();
  } else if (StageType == STAGE_GCODE) {
    //TODO: is there a use for a stage home command for Gcode stage?
    returnVal = 1;
  }
  return returnVal;
}

/*!
 * \brief StageCommands::StageStop
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::StageStop(Stage_t StageType) {
  int returnVal = 1; //default 1 is success
  if(StageType == STAGE_SMC) {
    SMC.StopMotion();
  } else if (StageType == STAGE_GCODE) {
    QString STOP = "M0\r";
    returnVal = StageSerial.writeString(STOP.toLatin1().data());
  }
  return returnVal;
}

/*!
 * \brief StageCommands::SetStageVelocity
 * \param VelocityToSet - Velocity to be set in unit of mm/s
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::SetStageVelocity(float VelocityToSet, Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    SMC.SetVelocity(VelocityToSet);
  } else if (StageType == STAGE_GCODE) {
    /*For the Gcode stage the velocity is set along with the move command
      so we need to set the static FeedRate variable*/
    FeedRate = VelocityToSet * 60;
  }
  return returnVal;
}

/*!
 * \brief StageCommands::SetStageAcceleration
 * \param AccelerationToSet - Acceleration to be in units of mm/s^2
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::SetStageAcceleration(float AccelerationToSet, Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    SMC.SetAcceleration(AccelerationToSet);
  } else if (StageType == STAGE_GCODE) {
    /*QString AccelCommand = "M201 Z" + QString::number(AccelerationToSet) + "\r\n";
    int AccelReturn = StageSerial.writeString(AccelCommand.toLatin1().data());
    if (AccelReturn < 0)
        returnVal = -1;*/ // TODO: Check if accel works
  }
  return returnVal;
}

/*!
 * \brief StageCommands::SetStagePositiveLimit
 * \param PositiveLimit - Sets the positive stage endstop for SMC stage (mm)
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::SetStagePositiveLimit(float PositiveLimit, Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    SMC.SetPositiveLimit(PositiveLimit);
  } else if (StageType == STAGE_GCODE) {
    // not applicable
  }
  return returnVal;
}

/*!
 * \brief StageCommands::SetStageNegativeLimit
 * \param NegativeLimit - Sets the negative stage endstop for SMC stage (mm)
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::SetStageNegativeLimit(float NegativeLimit, Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    SMC.SetNegativeLimit(NegativeLimit);
  } else if (StageType == STAGE_GCODE) {
    // not applicable
  }
  return returnVal;
}

int StageCommands::SetStageJerkTime(float JerkTime, Stage_t StageType) {
  int returnVal = 0;
  if (StageType == STAGE_SMC) {
    SMC.SetJerkTime(JerkTime);
  }
  return returnVal;
}

/*!
 * \brief StageCommands::StageAbsoluteMove
 * \param AbsoluteMovePosition
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::StageAbsoluteMove(float AbsoluteMovePosition, Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    SMC.AbsoluteMove(AbsoluteMovePosition);
  } else if (StageType == STAGE_GCODE) {
    //Set stage to do absolute move
    QString AbsMoveCommand = "G92\r\nG1 Z" + QString::number(AbsoluteMovePosition) + " F" + QString::number(FeedRate) + "\r\n";
    returnVal = StageSerial.writeString(AbsMoveCommand.toLatin1().data());
  }
  return returnVal;
}

/*!
 * \brief StageCommands::StageRelativeMove
 * Moves the stage relative to it's current position.
 * \param RelativeMoveDistance - Distance to move in units of (mm)
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns 1 if successful, -1 if failed
 */
int StageCommands::StageRelativeMove(float RelativeMoveDistance, Stage_t StageType) {
  int returnVal = 0;
  if(StageType == STAGE_SMC) {
    SMC.RelativeMove(RelativeMoveDistance);
  } else if (StageType == STAGE_GCODE) {
    QString RelMoveCommand = "G91\r\nG1 Z" + QString::number(-RelativeMoveDistance) + " F" + QString::number(FeedRate) + "\r\n";
    returnVal = StageSerial.writeString(RelMoveCommand.toLatin1().data());
  }
  return returnVal;
}

/*!
 * \brief StageCommands::StageGetPosition
 * Gets current stage position.
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \return - Returns QString containing the current stage position
 */
QString StageCommands::StageGetPosition(Stage_t StageType) {
  if(StageType == STAGE_SMC) {
    char* ReadPosition = SMC.GetPosition();
    if (strnlen(ReadPosition, 50) > 1) {
      QString CurrentPosition = QString::fromUtf8(ReadPosition);
      CurrentPosition.remove(0, 3); //Removes address and command
      CurrentPosition.chop(2);
      return CurrentPosition;
    }
  } else if (StageType == STAGE_GCODE) { //Test if
    QString GetPositionCommand = "M114 R\r\n";
    StageSerial.flushReceiver(); //Flush receiver to get rid of readout we don't need
    StageSerial.writeString(GetPositionCommand.toLatin1().data());
    static char receivedString[] = "ThisIsMyPositionTest";
    char finalChar = '\n';
    uint maxNbBytes = 100;//make sure to validate this
    StageSerial.readString(receivedString, finalChar, maxNbBytes, 10);
    printf(receivedString);
    QString CurrentPosition = QString::fromUtf8(receivedString);
    QString returnVal = CurrentPosition.mid(CurrentPosition.indexOf("Z"),
                                            CurrentPosition.indexOf("E"));
    returnVal.remove(0, 2);
    returnVal.chop(3);

    return returnVal;

  }
  return "NA";
}

int StageCommands::SendCustom(Stage_t StageType, QString Command) {
  if (StageType == STAGE_SMC) {
    //char* command = qPrintable(Command.toStdString());
    //char* ReadPosition = SMC.GetCustom(Command.toLocal8bit.constData());
  } else if (StageType == STAGE_GCODE) {
    return StageSerial.writeString(Command.toLatin1().data());
  }
  return 0;
}
/****************************Helper Functions****************************/
/*!
 * \brief StageCommands::initStagePosition
 * Saves the print settings to a module variable
 * and calls initStageSlot
 * \param si_PrintSettings - Print settings passed down from the Main Window
 */
void StageCommands::initStagePosition(PrintSettings si_PrintSettings) {
  s_PrintSettings = si_PrintSettings;
  initStageSlot();
  SMC.serial.flushReceiver();
}

static int MovementAttempts = 0;
/*!
 * \brief StageCommands::initStageSlot
 * Starts process of lowering stage down to it's starting position,
 * tells stage to move to 3.2mm above the starting position then checks
 * the stage position once per second and once it has reached the position
 * 3.2mm above the starting position calls the fineMovement function
 */
void StageCommands::initStageSlot() {
  if (s_PrintSettings.StageType == STAGE_SMC) {
    double CurrentPosition = StageGetPosition(STAGE_SMC).toDouble();
    emit StageGetPositionSignal(QString::number(CurrentPosition));
    if (CurrentPosition < (s_PrintSettings.StartingPosition - 3.2)) {
      SetStageVelocity(3, s_PrintSettings.StageType);
      Sleep(20);
      StageAbsoluteMove(s_PrintSettings.StartingPosition - 3, s_PrintSettings.StageType);
      emit StagePrintSignal("Performing Rough Stage Move to: " + QString::number(s_PrintSettings.StartingPosition - 3));
      repeatRoughSlot();
    } else {
      fineMovement();
    }
  } else {
    emit StagePrintSignal("Auto stage initialization disabled for iCLIP, please move stage to endstop with manual controls");
  }
}

/*!
 * \brief StageCommands::fineMovement
 * Sets the stage velocity to 0.3 mm/s and lowers the stage
 * the final 3.2 mm down to the starting position, this is
 * done to reduce the bubble created when the stage enters
 * the resin
 */
void StageCommands::fineMovement() {
  double CurrentPosition = StageGetPosition(STAGE_SMC).toDouble();
  emit StageGetPositionSignal(QString::number(CurrentPosition));
  MovementAttempts = 0;
  //If stage has not reached it's final starting position
  if (CurrentPosition > s_PrintSettings.StartingPosition - 0.05 && CurrentPosition < s_PrintSettings.StartingPosition + 0.05) {
    StageAbsoluteMove(s_PrintSettings.StartingPosition, s_PrintSettings.StageType);
    verifyStageParams(s_PrintSettings);
  } else {
    SetStageVelocity(0.3, s_PrintSettings.StageType);
    Sleep(20);
    StageAbsoluteMove(s_PrintSettings.StartingPosition, s_PrintSettings.StageType);
    emit StagePrintSignal("Fine Stage Movement to: " + QString::number(s_PrintSettings.StartingPosition));
    repeatFineSlot();
  }
}

void StageCommands::repeatRoughSlot() {
  double CurrentPosition = StageGetPosition(STAGE_SMC).toDouble();
  emit StageGetPositionSignal(QString::number(CurrentPosition));
  if (CurrentPosition > s_PrintSettings.StartingPosition - 3.2 && CurrentPosition < s_PrintSettings.StartingPosition + 3.2) {
    fineMovement();
  } else {
    if (MovementAttempts < 30) {
      QTimer::singleShot(1000, this, SLOT(repeatRoughSlot()));
    } else {
      emit StagePrintSignal("Could not move stage to starting position");
    }
  }

}

void StageCommands::repeatFineSlot() {
  double CurrentPosition = StageGetPosition(STAGE_SMC).toDouble();
  emit StageGetPositionSignal(QString::number(CurrentPosition));
  if (CurrentPosition > s_PrintSettings.StartingPosition - 0.05 && CurrentPosition < s_PrintSettings.StartingPosition + 0.05) {
    StageAbsoluteMove(s_PrintSettings.StartingPosition, s_PrintSettings.StageType);
    verifyStageParams(s_PrintSettings);
  } else {
    if (MovementAttempts < 30) {
      QTimer::singleShot(1000, this, SLOT(repeatFineSlot()));
    } else {
      emit StagePrintSignal("Could not move stage to starting position");
    }
  }
}

/*!
 * \brief StageCommands::verifyStageParams
 * After the stage has reached it's final position
 * and is ready to start the final stage printing parameters
 * are sent to the stage.
 * \param s_PrintSettings - Print settings passed down from MainWindow,
 * uses StageType, StageVelocity, StageAcceleration, MinEndOfRun, and StageVelocity
 */
void StageCommands::verifyStageParams(PrintSettings s_PrintSettings) {
  emit StagePrintSignal("Verifying Stage Parameters");
  SetStageJerkTime(s_PrintSettings.JerkTime, s_PrintSettings.StageType);
  Sleep(20);
  SetStageAcceleration(s_PrintSettings.StageAcceleration, s_PrintSettings.StageType);
  Sleep(20);
  SetStageNegativeLimit(s_PrintSettings.MinEndOfRun, s_PrintSettings.StageType);
  Sleep(20);
  SetStagePositiveLimit(s_PrintSettings.MaxEndOfRun, s_PrintSettings.StageType);
  Sleep(20);
  SetStageVelocity(s_PrintSettings.StageVelocity, s_PrintSettings.StageType);
  Sleep(20);
  double StagePosition  = StageGetPosition(STAGE_SMC).toDouble();
  emit StageGetPositionSignal(QString::number(StagePosition));
}

/*!
 * \brief StageCommands::initStageStart
 * Used to confirm that the stage velocity is set correctly,
 * if in Continuous motion mode calculates the continuous stage
 * velocity and sets it.
 * \param si_PrintSettings Print settings passed down from MainWindow,
 * uses StageType, MotionMode, StageVelocity, and ExposureTime
 */
void StageCommands::initStageStart(PrintSettings si_PrintSettings) {
  if(si_PrintSettings.MotionMode == STEPPED) {
    SetStageVelocity(si_PrintSettings.StageVelocity, si_PrintSettings.StageType);
  } else if (si_PrintSettings.MotionMode == CONTINUOUS) {
    double ContStageVelocity = (si_PrintSettings.StageVelocity) / (si_PrintSettings.ExposureTime / (1e6)); //Multiply Exposure time by 1e6 to convert from us to s to get to proper units
    emit StagePrintSignal("Continuous Stage Velocity set to " + QString::number(si_PrintSettings.StageVelocity) + "/" + QString::number(si_PrintSettings.ExposureTime) + " = " + QString::number(ContStageVelocity) + " mm/s");
    SetStageVelocity(ContStageVelocity, si_PrintSettings.StageType);
  }
  Sleep(10);
}
/*
void StageCommands::StageAbort(PrintSettings si_PrintSettings)
{
    StageStop(si_PrintSettings.StageType);
}
*/
void StageCommands::resetStageInit() {
  MovementAttempts = 0;
}
