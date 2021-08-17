#include "printcontrol.h"
#include "stagecommands.h"
#include "pumpcommands.h"
#include "dlp9000.h"
#include <QTimer>

DLP9000& pc_DLP = DLP9000::Instance();
StageCommands& pc_Stage = StageCommands::Instance();
PumpCommands& pc_Pump = PumpCommands::Instance();

printcontrol::printcontrol()
{

}

/*!
 * \brief printcontrol::InitializeSystem
 * Initializes system to be ready for print, moves stage to starting position, uploads patterns to light engine,
 * calculates and sets print end for continuous prints and exposure type.
 * \param ImageList - List of images to be uploaded
 * \param m_PrintSettings - Print settings from Main Window, passed to initStagePosition, PatternUpload, and CalcPrintEnd
 * \param pPrintControls - Print Controls pointer from Main Window, passed to PatternUpload and CalcPrintEnd, sets PrintEnd and ExposureType
 * \param m_PrintScript - Print script from Main Window, passed to PatternUpload and GetExposureType
 */
void printcontrol::InitializeSystem(QStringList ImageList, PrintSettings m_PrintSettings, PrintControls *pPrintControls, PrintScripts m_PrintScript)
{
    pc_DLP.PatternDisplay(OFF);
    pc_Stage.initStagePosition(m_PrintSettings);
    pc_DLP.PatternUpload(ImageList, *pPrintControls, m_PrintSettings, m_PrintScript);

    pPrintControls->PrintEnd = CalcPrintEnd(pPrintControls->nSlice, m_PrintSettings);
    pPrintControls->ExposureType = GetExposureType(m_PrintScript.PrintScript, m_PrintSettings.PumpingMode);
}

/*!
 * \brief printcontrol::AbortPrint
 * Aborts the print, stop projection, stops stage movement, stops pump, ends print process by setting layerCount to 0xFFFFFF
 * \param StageType - Select stage type between STAGE_SMC or STAGE_GCODE
 * \param pPrintControl - Pointer to PrintControls from MainWindow, sets layerCount
 */
void printcontrol::AbortPrint(Stage_t StageType, PrintControls *pPrintControl)
{
    pc_DLP.PatternDisplay(OFF);
    pc_Stage.StageStop(StageType);
    pc_Pump.Stop();
    pPrintControl->layerCount = 0xFFFFFF;
}

/*!
 * \brief printcontrol::StartPrint
 * Starts print, sets LEDIntensity, validates stage parameters, starts light engine pattern sequence
 * if in continuous injection mode starts infusion/injection
 * \param m_PrintSettings - Print settings from Main Window, passed to SetLEDIntensity and initStageStart, uses ProjectionMode
 * \param m_PrintScript - Print script from Main Window, passed to SetLEDIntensity using PrintScript and LEDScriptList
 * \param ContinuousInjection - bool to indicate whether system is in continuous injection mode
 */
void printcontrol::StartPrint(PrintSettings m_PrintSettings, PrintScripts m_PrintScript, bool ContinuousInjection)
{
    pc_DLP.SetLEDIntensity(m_PrintSettings.UVIntensity, m_PrintScript.PrintScript, m_PrintScript.LEDScriptList);
    pc_Stage.initStageStart(m_PrintSettings);
    emit GetPositionSignal();
    pc_DLP.startPatSequence();

    if (ContinuousInjection == ON){
        pc_Pump.StartInfusion();
    }

    if (m_PrintSettings.ProjectionMode == POTF){
        pc_DLP.clearElements();
        emit ControlPrintSignal("Entering POTF print process");
    }
    else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        emit ControlPrintSignal("Entering Video Patttern print process");
    }
}

/*!
 * \brief printcontrol::ReuploadHandler
 * Handles the reupload, stops stage if in continuous print mode
 * \param ImageList - From MainWindow, passed to PatternUpload
 * \param m_PrintControls - From MainWindow, passed to PatternUpload
 * \param m_PrintSettings - From MainWindow, passed to PatternUpload, StageType passed to StageStop method
 * \param m_PrintScript - From MainWindow, passed to PatternUpload
 * \return - Returns the number of patterns uploaded
 */
int printcontrol::ReuploadHandler(QStringList ImageList, PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    if(m_PrintSettings.MotionMode == CONTINUOUS){
        pc_Stage.StageStop(m_PrintSettings.StageType);
    }
    return pc_DLP.PatternUpload(ImageList, m_PrintControls, m_PrintSettings, m_PrintScript);
}

/*!
 * \brief printcontrol::PrintProcessHandler
 * Ensures that initial exposure only happens once, also handles layerCount and remainingImages.
 * \param pPrintControls - From MainWindow, uses InitialExposureFlag, sets inMotion, layerCount, and remainingImages
 * \param InitialExposure - Initial exposure duration in units of seconds
 */
void printcontrol::PrintProcessHandler(PrintControls *pPrintControls, uint InitialExposure)
{
    if(pPrintControls->InitialExposureFlag == true){
        pPrintControls->InitialExposureFlag = false;
        pPrintControls->inMotion = false;
        emit ControlPrintSignal("Exposing Initial Layer " + QString::number(InitialExposure) + "s");
        emit UpdatePlotSignal();
    }
    else{
        pPrintControls->layerCount++;
        pPrintControls->remainingImages--;
    }
}

/*!
 * \brief printcontrol::VPFrameUpdate
 * Handles the frame update for Video Pattern mode.
 * \param pPrintControls - Pointer from MainWindow, uses BitLayer, sets FrameCount,
 * BitLayer, ReSyncCount, ReSyncFlag
 * \param BitMode - Bit depth of the images for this print
 * \return - Returns false if no frame update is needed, true if frame update is needed
 */
bool printcontrol::VPFrameUpdate(PrintControls *pPrintControls, int BitMode)
{
    bool returnVal = false;
    if (pPrintControls->BitLayer > 24){
        pPrintControls->FrameCount++;
        emit ControlPrintSignal("New Frame: " + QString::number(pPrintControls->FrameCount));
        pPrintControls->BitLayer = 1;
        pPrintControls->ReSyncCount++;

        //If 120 frames have been reached, prepare for resync
        if(pPrintControls->ReSyncCount > (120 - 24)/(24/BitMode)){
            pPrintControls->ReSyncFlag = ON;
            pPrintControls->ReSyncCount = 0;
        }
        returnVal = true;
    }
    return returnVal;
}

/*!
 * \brief printcontrol::DarkTimeHandler
 * Sets the appropriate dark time, handles injection delays for iCLIP and moves stage
 * \param m_PrintControls - Print controls from MainWindow, passed to StageMove
 * \param m_PrintSettings - Print settings from MainWindow, passed to StageMove, using PrinterType
 * \param m_PrintScript - Print script from MainWindow, passed to StageMove
 * \param m_InjectionSettings - Injection settings from MainWindow, using InjectionDelayFlag, ContinuousInjection, InjectionDelayParam
 */
void printcontrol::DarkTimeHandler(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript, InjectionSettings m_InjectionSettings)
{
    if(m_PrintSettings.PrinterType == ICLIP){
        if (m_InjectionSettings.InjectionDelayFlag == PRE){
            PrintInfuse(m_InjectionSettings.ContinuousInjection);
            QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(StageMove(m_PrintControls, m_PrintSettings, m_PrintScript)));
            emit ControlPrintSignal("Pre-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
        }
        else if (m_InjectionSettings.InjectionDelayFlag == POST){
            StageMove(m_PrintControls, m_PrintSettings, m_PrintScript);
            QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(m_InjectionSettings));
            emit ControlPrintSignal("Post-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
        }
        else{
            PrintInfuse(m_InjectionSettings.ContinuousInjection);
            StageMove(m_PrintControls, m_PrintSettings, m_PrintScript);
            emit ControlPrintSignal("No injection delay");
        }
        emit ControlPrintSignal("Injecting " + QString::number(m_InjectionSettings.InfusionVolume) + "ul at " + QString::number(m_InjectionSettings.InfusionRate) + "ul/s");
    }
    else{
        StageMove(m_PrintControls, m_PrintSettings, m_PrintScript);
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
void printcontrol::StagePumpingHandler(uint layerCount, PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    if(m_PrintScript.PrintScript == ON){
        if (layerCount < m_PrintScript.PumpHeightScriptList.size()){
            pc_Stage.StageRelativeMove(-m_PrintScript.PumpHeightScriptList.at(layerCount).toDouble(), m_PrintSettings.StageType);
            emit ControlPrintSignal("Pumping " + QString::number(m_PrintScript.PumpHeightScriptList.at(layerCount).toDouble()*1000) +" um");
        }
    }
    else{
        pc_Stage.StageRelativeMove(-m_PrintSettings.PumpingParameter, m_PrintSettings.StageType);
        emit ControlPrintSignal("Pumping " + QString::number(m_PrintSettings.PumpingParameter*1000) +" um");
    }
}
/***************************************Helpers*********************************************/

/*!
 * \brief printcontrol::CalcPrintEnd
 * Calculates the end of the print for a continuous motion print,
 * \param nSlice - Number of slices in the print
 * \param m_PrintSettings - Print settings from MainWindow, using MotionMode,
 *        PrinterType, StartingPosition and LayerThickness
 * \return
 */
double printcontrol::CalcPrintEnd(uint nSlice, PrintSettings m_PrintSettings)
{
    double PrintEnd = 0;
    if(m_PrintSettings.MotionMode == CONTINUOUS){
        if(m_PrintSettings.PrinterType == CLIP30UM){
            PrintEnd = m_PrintSettings.StartingPosition - (nSlice*m_PrintSettings.LayerThickness);
        }
        else if (m_PrintSettings.PrinterType == ICLIP){
            PrintEnd = nSlice*m_PrintSettings.LayerThickness;
        }
        //PrintToTerminal("Print end for continuous motion print set to: " + QString::number(m_PrintControls.PrintEnd));
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
ExposureType_t printcontrol::GetExposureType(int PrintScript, int PumpingMode)
{
    ExposureType_t ExposureType;
    if (PrintScript == 1){
        if (PumpingMode == 1){
            ExposureType = EXPOSURE_PS_PUMP;
        }
        else{
            ExposureType = EXPOSURE_PS;
        }
    }
    else{
        if (PumpingMode == 1){
            ExposureType = EXPOSURE_PUMP;
        }
        else{
            ExposureType = EXPOSURE_NORM;
        }
    }
    return ExposureType;
}

//Move layer count out fix docu later
void printcontrol::StageMove(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    //If printscript is active, filter out layerCount = 0 and layerCount is greater than script length
    if (m_PrintScript.PrintScript == ON){
        if (m_PrintControls.layerCount > 0){
            if (m_PrintControls.layerCount < m_PrintScript.LayerThicknessScriptList.size()){
                double LayerThickness = m_PrintScript.LayerThicknessScriptList.at(m_PrintControls.layerCount).toDouble()/1000; //grab layer thickness from script list
                emit ControlPrintSignal("Moving Stage: " + QString::number(m_PrintScript.LayerThicknessScriptList.at(m_PrintControls.layerCount).toDouble()) + " um");
                pc_Stage.StageRelativeMove(-LayerThickness, m_PrintSettings.StageType); //Move stage 1 layer thickness

                //If pumping mode is active, grab pump height from script and move stage Pump height - layer thickness
                if (m_PrintSettings.PumpingMode == ON){
                    double PumpParam = m_PrintScript.PumpHeightScriptList.at(m_PrintControls.layerCount).toDouble();
                    pc_Stage.StageRelativeMove(PumpParam - LayerThickness, m_PrintSettings.StageType);
                }
            }
        }
    }
    else{
        if (m_PrintSettings.PumpingMode == ON){
            pc_Stage.StageRelativeMove(m_PrintSettings.PumpingParameter - m_PrintSettings.LayerThickness, m_PrintSettings.StageType);
            emit ControlPrintSignal("Pumping active, moving stage: " + QString::number(m_PrintSettings.PumpingParameter - m_PrintSettings.LayerThickness) + " um");
        }
        else{
            pc_Stage.StageRelativeMove(-m_PrintSettings.LayerThickness, m_PrintSettings.StageType);
            emit ControlPrintSignal("Moving stage: " + QString::number(m_PrintSettings.LayerThickness) + " um");
        }
    }
}

/*!
 * \brief printcontrol::PrintInfuse
 * Handles infusion/injection if continous injection is disabled.
 * clears volume and starts new infusion
 * \param ContinuousInjection - boolean, true if ContinuousInjection is active, false otherwise
 */
void printcontrol::PrintInfuse(bool ContinuousInjection)
{
    if (ContinuousInjection == OFF){
        pc_Pump.ClearVolume();
        Sleep(10);
        pc_Pump.StartInfusion();
        emit ControlPrintSignal("Injecting resin");
    }
}
