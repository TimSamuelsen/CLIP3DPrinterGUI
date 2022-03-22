#include "printcontrol.h"
#include "stagecommands.h"
#include "pumpcommands.h"
#include "dlp9000.h"
#include <QTimer>
#include <QTime>

DLP9000& pc_DLP = DLP9000::Instance();
StageCommands& pc_Stage = StageCommands::Instance();
PumpCommands& pc_Pump = PumpCommands::Instance();

// Constructor
printcontrol::printcontrol() {

}

// Pointers stored for future use
void printcontrol::getControlPointers(PrintSettings* pPrintSettings, PrintControls* pPrintControls
                                      , PrintScripts* pPrintScript) {
  pcPrintSettings = pPrintSettings;
  pcPrintControls = pPrintControls;
  pcPrintScript = pPrintScript;
}

/*!
 * \brief printcontrol::InitializeSystem
 * Initializes system to be ready for print, moves stage to starting position, uploads patterns to
 * light engine, calculates and sets print end for continuous prints and exposure type.
 * \param ImageList  List of images to be uploaded
 * \param m_PrintSettings  Print settings from Main Window, passed to initStagePosition,
 *                         PatternUpload, and CalcPrintEnd
 * \param pPrintControls  Print Controls pointer from Main Window, passed to PatternUpload and
 *                        CalcPrintEnd, sets PrintEnd and ExposureType
 * \param m_PrintScript  Print script from Main Window, passed to PatternUpload and GetExposureType
 */
void printcontrol::InitializeSystem(QStringList ImageList, PrintSettings m_PrintSettings,
                                    PrintControls* pPrintControls, PrintScripts m_PrintScript,
                                    InjectionSettings m_InjectionSettings) {
  pc_Stage.initStagePosition(m_PrintSettings);    // Move stage to starting position
  pc_Pump.initPumpParams(m_InjectionSettings);
  pc_DLP.SetLEDIntensity(m_PrintSettings.InitialIntensity);
  if(m_PrintSettings.ProjectionMode != VIDEO) {
    pc_DLP.PatternDisplay(OFF);
    pc_DLP.PatternUpload(ImageList, *pPrintControls, m_PrintSettings, m_PrintScript);
  }
  pPrintControls->PrintEnd = CalcPrintEnd(pPrintControls->nSlice, m_PrintSettings);
  pPrintControls->ExposureType = GetExposureType(m_PrintScript.PrintScript, m_PrintSettings.PumpingMode);
}

/*!
 * \brief printcontrol::AbortPrint
 * Aborts the print, stops projection, stops stage movement, stops pump, ends print process
 * by setting layerCount to 0xFFFFFF
 * \param StageType  Select stage type between STAGE_SMC or STAGE_GCODE
 * \param pPrintControl  Pointer to PrintControls from MainWindow, sets layerCount
 */
void printcontrol::AbortPrint(Stage_t StageType, PrintControls* pPrintControl) {
  pc_DLP.PatternDisplay(OFF);                 // Turn projection off
  pc_Stage.StageStop(StageType);              // Stop stage movement
  pc_Stage.StageClose(StageType);             // Close stage connection
  pc_Pump.Stop();                             // Stop injection
  pPrintControl->layerCount = 0xFFFFFF;       // End print process loop
}

/*!
 * \brief printcontrol::StartPrint
 * Starts print, sets LEDIntensity, validates stage parameters, starts light engine pattern sequence
 * if in continuous injection mode starts infusion/injection
 * \param m_PrintSettings   Print settings from Main Window, passed to SetLEDIntensity and
 *                          initStageStart, uses ProjectionMode
 * \param m_PrintScript     Print script from Main Window, passed to SetLEDIntensity
 *                          using PrintScript and LEDScriptList
 * \param ContinuousInjection   bool to indicate whether system is in continuous injection mode
 */
void printcontrol::StartPrint(PrintSettings m_PrintSettings, PrintScripts m_PrintScript,
                              bool ContinuousInjection) {
  pc_Stage.initStageStart(m_PrintSettings);       // Validate correct stage parameters are set
  emit GetPositionSignal();                       // Sanity check

  if (m_PrintSettings.ProjectionMode != VIDEO) {  // No pattern sequences in video mode
    pc_DLP.startPatSequence();
  }

  if (ContinuousInjection == ON) {
    pc_Pump.StartInfusion();
  }

  if (m_PrintSettings.ProjectionMode == POTF) {
    pc_DLP.clearElements();
    emit ControlPrintSignal("Entering POTF print process");
  } else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN) {
    emit ControlPrintSignal("Entering Video Patttern print process");
  }
}

/*!
 * \brief printcontrol::ReuploadHandler
 * Handles the reupload, stops stage if in continuous print mode
 * \param ImageList  From MainWindow, passed to PatternUpload
 * \param m_PrintControls  From MainWindow, passed to PatternUpload, CheckReupload
 * \param m_PrintSettings  From MainWindow, passed to PatternUpload, StageType passed to StageStop,
 *                         CheckReupload
 * \param m_PrintScript    From MainWindow, passed to PatternUpload, CheckReupload
 * \return  Returns the number of patterns uploaded
 */
int printcontrol::ReuploadHandler(QStringList ImageList, PrintControls m_PrintControls, PrintSettings m_PrintSettings,
                                  PrintScripts m_PrintScript, bool ContinuousInjection) {
  int UploadedImages = 0;
  bool VPReupload = CheckReupload(m_PrintSettings, m_PrintControls, m_PrintScript);
  if (VPReupload == false) {
    UploadedImages = m_PrintSettings.ResyncVP;
    pc_DLP.startPatSequence();
  } else {
    emit ControlPrintSignal("Entering Reupload: " + QTime::currentTime().toString("hh.mm.ss.zzz"));

    // If in continuous motion mode, pause stage movement during the reupload
    if(m_PrintSettings.MotionMode == CONTINUOUS) {
      pc_Stage.StageStop(m_PrintSettings.StageType);
      emit ControlPrintSignal("Pausing Continuous Stage Movement");
    }

    // If in continuous injection mode, pause injection during the reupload
    if(ContinuousInjection) {
      pc_Pump.Stop();
      emit ControlPrintSignal("Pausing Continuous Injection");
    }

    UploadedImages = pc_DLP.PatternUpload(ImageList, m_PrintControls, m_PrintSettings, m_PrintScript);
    emit ControlPrintSignal(QString::number(UploadedImages) + " images uploaded");

    // Restarting continous injection after reupload
    if(ContinuousInjection) {
      pc_Pump.SetTargetVolume(0);
      pc_Pump.StartInfusion();
      emit ControlPrintSignal("Resuming Continuous Injection");
    }
    Sleep(625);                     // Necessary delay for DLP to process reupload
    pc_DLP.startPatSequence();      // Restart projection after reupload

    if (m_PrintScript.VP8) {        // TODO: validate this
      UploadedImages = m_PrintSettings.ResyncVP;
    }
  }
  return UploadedImages;
}

/*!
 * \brief printcontrol::PrintProcessHandler
 * Ensures that initial exposure only happens once, also handles layerCount and remainingImages.
 * \param pPrintControls From MainWindow, uses InitialExposureFlag, sets inMotion, layerCount, and remainingImages
 * \param InitialExposure Initial exposure duration in units of seconds
 */
void printcontrol::PrintProcessHandler(PrintControls* pPrintControls, PrintSettings m_PrintSettings,
                                       InjectionSettings m_InjectionSettings) {
  if(pPrintControls->InitialExposureFlag == true) {
    pPrintControls->InitialExposureFlag = false;
    pPrintControls->inMotion = false;
    emit ControlPrintSignal("Exposing Initial Layer " + QString::number(m_PrintSettings.InitialExposure) + "s");
    emit GetPositionSignal();
    pc_Pump.SetTargetVolume(m_InjectionSettings.InfusionVolume);
  } else {
    if(m_InjectionSettings.SteppedContinuousInjection) {
      pc_Pump.SetInfuseRate(m_InjectionSettings.BaseInjectionRate);
      emit ControlPrintSignal("Injection rate set to: " +
                              QString::number(m_InjectionSettings.BaseInjectionRate));
    }
    pPrintControls->layerCount++;
    pPrintControls->remainingImages--;
    pPrintControls->BitLayer += m_PrintSettings.BitMode;
    emit ControlPrintSignal("Layer " + QString::number(pPrintControls->layerCount));
    emit GetPositionSignal();
  }
}

/*!
 * \brief printcontrol::VPFrameUpdate
 * Handles the frame update for Video Pattern mode.
 * \param pPrintControls  Pointer from MainWindow, uses BitLayer, sets FrameCount,
 * BitLayer, ReSyncCount, ReSyncFlag
 * \param BitMode  Bit depth of the images for this print
 * \return  Returns false if no frame update is needed, true if frame update is needed
 */
bool printcontrol::VPFrameUpdate(PrintControls* pPrintControls, int BitMode, int ReSyncRate) {
  bool returnVal = false;
  if (pPrintControls->BitLayer > 24) {    // If exceeded max bit layers for 1 image
    pPrintControls->FrameCount++;
    emit ControlPrintSignal("New Frame: " + QString::number(pPrintControls->FrameCount));
    pPrintControls->BitLayer = 1;       // reset BitLayer counter to 1; TODO: Validate this
    pPrintControls->ReSyncCount++;

    // If n resync count frames have been reached, prepare for resync
    if(pPrintControls->ReSyncCount > (ReSyncRate - 24) / (24 / BitMode)) {
      pPrintControls->ReSyncFlag = ON;
      pPrintControls->ReSyncCount = 0;
    }
    returnVal = true;
  }
  return returnVal;
}

/*!
 * \brief printcontrol::DarkTimeHandler
 * Sets the appropriate dark time, handles pre and post injection delays for iCLIP and moves stage
 * \param m_PrintControls  Print controls from MainWindow, passed to StageMove
 * \param m_PrintSettings  Print settings from MainWindow, passed to StageMove, using PrinterType
 * \param m_PrintScript  Print script from MainWindow, passed to StageMove
 * \param m_InjectionSettings  Injection settings from MainWindow, using InjectionDelayFlag,
 *                              ContinuousInjection, InjectionDelayParam
 */
void printcontrol::DarkTimeHandler(PrintSettings m_PrintSettings, PrintScripts m_PrintScript,
                                   InjectionSettings m_InjectionSettings) {
  if(m_PrintSettings.PrinterType == ICLIP) {
    // For pre injection delay, injection is handled first and stage movement second
    if (m_InjectionSettings.InjectionDelayFlag == PRE) {
      PrintInfuse(m_InjectionSettings);
      QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(StageMove()));
      emit ControlPrintSignal("Pre-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
    }
    // For post injection delay, stage movement is handled first and injection second
    else if (m_InjectionSettings.InjectionDelayFlag == POST) {
      StageMove();
      QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(PrintInfuse(m_InjectionSettings)));
      emit ControlPrintSignal("Post-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
    }
    // If no injection delay is set, injection and movement are handled at the same time
    else {
      PrintInfuse(m_InjectionSettings);
      StageMove();
      emit ControlPrintSignal("No injection delay");
    }

    if (m_InjectionSettings.ContinuousInjection == OFF) {
      emit ControlPrintSignal("Injecting " + QString::number(m_InjectionSettings.InfusionVolume)
                              + "ul at " + QString::number(m_InjectionSettings.InfusionRate) + "ul/s");
    }
  }
  // If not in iCLIP mode, stage movement is performed directly
  else {
    if (m_PrintSettings.PostExposureDelay == 0) {
      StageMove();
    } else {    // QTimer is used to create a delay before the stage movement
      QTimer::singleShot(m_PrintSettings.PostExposureDelay, Qt::PreciseTimer, this
                         , SLOT(StageMove()));
      emit ControlPrintSignal("Post-Exposure delay: " + QString::number(m_PrintSettings.PostExposureDelay) + " ms");
    }
    emit GetPositionSignal();
  }
}

/*!
 * \brief printcontrol::StagePumpingHandler
 * Handles stage pumping by instructing the stage to move the pumping distance
 * , if using print script grabs value from print script
 * \param layerCount - The current layer count for the print
 * \param m_PrintSettings - Print settings from MainWindow, using PumpingParameter and StageType
 * \param m_PrintScript - Print script from MainWindow, using PrintScript and PumpHeightScriptList
 */
void printcontrol::StagePumpingHandler(int layerCount, PrintSettings m_PrintSettings, PrintScripts m_PrintScript) {
  // If in printscript mode grab pump height from PumpHeightScriptList
  if(m_PrintScript.PrintScript == ON) {
    // This if statement is a guard on segmentation faults
    if (layerCount < m_PrintScript.PumpHeightScriptList.size()) {
      double PumpHeight = m_PrintScript.PumpHeightScriptList.at(layerCount).toDouble();
      pc_Stage.StageRelativeMove(-PumpHeight, m_PrintSettings.StageType);
      emit ControlPrintSignal("Pumping " + QString::number(PumpHeight) + " um");
    }
  }
  // If not in print script mode use static pumping parameter
  else {
    pc_Stage.StageRelativeMove(-m_PrintSettings.PumpingParameter, m_PrintSettings.StageType);
    emit ControlPrintSignal("Pumping " + QString::number(m_PrintSettings.PumpingParameter * 1000) + " um");
  }
}
/***************************************Helpers*********************************************/
// Helper function to determine whether a reupload is needed
bool printcontrol::CheckReupload(PrintSettings m_PrintSettings, PrintControls m_PrintControls
                                 , PrintScripts m_PrintScript) {
  bool returnVal = true;
  if (m_PrintSettings.ProjectionMode == VIDEOPATTERN) {
    if (m_PrintScript.PrintScript == OFF) {
      if (m_PrintControls.layerCount != 0) {
        returnVal = false;
      }
    } else {
      // check not at end or beginning of print to avoid sig segv when accessing scripts
      if ((int)m_PrintControls.layerCount + m_PrintSettings.ResyncVP < m_PrintScript.ExposureScriptList.size()
          && m_PrintControls.layerCount > 1) {
        returnVal = false; // Default to false if all match
        for (uint i = m_PrintControls.layerCount; i < m_PrintControls.layerCount + m_PrintSettings.ResyncVP; i++) {
          double Last = m_PrintScript.ExposureScriptList.at(i - m_PrintSettings.ResyncVP).toDouble();
          double Current = m_PrintScript.ExposureScriptList.at(i).toDouble();
          if (Last != Current) {
            returnVal = true;
            break;
          }
        }
      }
    }
  }
  return returnVal;
}

/*!
 * \brief printcontrol::CalcPrintEnd
 * Calculates the end of the print for a continuous motion print,
 * \param nSlice - Number of slices in the print
 * \param m_PrintSettings - Print settings from MainWindow, using MotionMode,
 *        PrinterType, StartingPosition and LayerThickness
 * \return  Returns the calculated print ending position
 */
double printcontrol::CalcPrintEnd(uint nSlice, PrintSettings m_PrintSettings) {
  double PrintEnd = 0;
  if(m_PrintSettings.MotionMode == CONTINUOUS) {
    if(m_PrintSettings.PrinterType == CLIP30UM) {
      PrintEnd = m_PrintSettings.StartingPosition - (nSlice * m_PrintSettings.LayerThickness);
    } else if (m_PrintSettings.PrinterType == ICLIP) {
      PrintEnd = nSlice * m_PrintSettings.LayerThickness;
    }
    emit ControlPrintSignal("Print end for continuous motion print set to: " +
                            QString::number(PrintEnd));
  }
  return PrintEnd;
}

/*!
 * \brief printcontrol::GetExposureType
 * Gets the exposure type depending on if print script and/or pumping mode is active
 * \param PrintScript - 1 if print script is active, 0 otherwise
 * \param PumpingMode - 1 if pumping mode is active, 0 otherwise
 * \return EXPOSURE_NORM for pumping mode and print script disabled
 * \return EXPOSURE_PS for print script active, pumping mode disabled
 * \return EXPOSURE_PUMP for pumping mode disabled, print script active
 * \return EXPOSURE_PS_PUMP for pumping mode and print script active
 */
ExposureType_t printcontrol::GetExposureType(int PrintScript, int PumpingMode) {
  ExposureType_t ExposureType;
  if (PrintScript == ON) {
    if (PumpingMode == ON) {
      ExposureType = EXPOSURE_PS_PUMP;
    } else {
      ExposureType = EXPOSURE_PS;
    }
  } else {
    if (PumpingMode == ON) {
      ExposureType = EXPOSURE_PUMP;
    } else {
      ExposureType = EXPOSURE_NORM;
    }
  }
  return ExposureType;
}

//Move layer count out fix docu later
//Be very careful when accessing pointers...
void printcontrol::StageMove() {
  //If printscript is active, filter out layerCount = 0 and layerCount is greater than script length
  if (pcPrintScript->PrintScript == ON && pcPrintControls->layerCount) {
    uint lenScript = pcPrintScript->LayerThicknessScriptList.size();
    if (pcPrintControls->layerCount > 0 && pcPrintControls->layerCount < lenScript) {
      double LayerThickness = pcPrintScript->LayerThicknessScriptList.at(pcPrintControls->layerCount).toDouble() / 1000; //grab layer thickness from script list
      emit ControlPrintSignal("Moving Stage: " + QString::number(LayerThickness) + " um");
      pc_Stage.StageRelativeMove(-LayerThickness, pcPrintSettings->StageType); //Move stage 1 layer thickness

      // If pumping mode is active, grab pump height from script and move stage (Pump height - layer thickness)
      if (pcPrintSettings->PumpingMode == ON) {
        double PumpParam = pcPrintScript->PumpHeightScriptList.at(pcPrintControls->layerCount).toDouble();
        pc_Stage.StageRelativeMove(PumpParam - LayerThickness, pcPrintSettings->StageType);
      }
    }
  } else {
    if (pcPrintSettings->PumpingMode == ON) {
      double pumpDistance = pcPrintSettings->PumpingParameter - pcPrintSettings->LayerThickness;
      pc_Stage.StageRelativeMove(pumpDistance, pcPrintSettings->StageType);
      emit ControlPrintSignal("Pumping active, moving stage: " + QString::number(1000 * (pumpDistance)) + " um");
    } else {
      pc_Stage.StageRelativeMove(-pcPrintSettings->LayerThickness, pcPrintSettings->StageType);
      emit ControlPrintSignal("Moving Stage: " + QString::number(pcPrintSettings->LayerThickness * 1000) + " um");
    }
  }
}

/*!
 * \brief printcontrol::PrintInfuse
 * Handles infusion/injection if continous injection is disabled.
 * clears volume and starts new infusion
 * \param ContinuousInjection - boolean, true if ContinuousInjection is active, false otherwise
 */
void printcontrol::PrintInfuse(InjectionSettings m_InjectionSettings) {
  if (m_InjectionSettings.ContinuousInjection == OFF) {
    pc_Pump.ClearVolume();
    Sleep(10);              // delay because pump firmware is bad
    pc_Pump.StartInfusion();
    emit ControlPrintSignal("Injecting resin");
  } else if (m_InjectionSettings.SteppedContinuousInjection == ON) {
    pc_Pump.SetInfuseRate(m_InjectionSettings.InfusionRate);
    emit ControlPrintSignal("Injection rate set to: " +
                            QString::number(m_InjectionSettings.InfusionRate));

  }
}
