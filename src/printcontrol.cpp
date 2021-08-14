#include "printcontrol.h"
#include "stagecommands.h"
#include "pumpcommands.h"
#include "dlp9000.h"
#include <QTimer>

DLP9000& pc_DLP = DLP9000::Instance();
StageCommands& pc_Stage = StageCommands::Instance();
PumpCommands& pc_Pump = PumpCommands::Instance();

static QStringList ImageList;

printcontrol::printcontrol()
{

}

void printcontrol::InitializeSystem(QStringList ImageList, PrintSettings m_PrintSettings, PrintControls *pPrintControls, PrintScripts m_PrintScript)
{
    pc_DLP.PatternDisplay(OFF);
    pc_Stage.initStagePosition(m_PrintSettings);
    pc_DLP.PatternUpload(ImageList, *pPrintControls, m_PrintSettings, m_PrintScript);

    pPrintControls->PrintEnd = CalcPrintEnd(*pPrintControls, m_PrintSettings);
    pPrintControls->ExposureType = GetExposureType(m_PrintSettings, m_PrintScript);
}

void printcontrol::AbortPrint(PrintSettings m_PrintSettings, PrintControls *pPrintControl)
{
    pc_DLP.PatternDisplay(OFF);
    pc_Stage.StageStop(m_PrintSettings.StageType);
    pc_Pump.Stop();
    pPrintControl->layerCount = 0xFFFFFF;

}

void printcontrol::StartPrint(PrintSettings m_PrintSettings, PrintScripts m_PrintScript, InjectionSettings m_InjectionSettings)
{
    pc_DLP.SetLEDIntensity(m_PrintSettings, m_PrintScript);
    pc_Stage.initStageStart(m_PrintSettings);
    emit GetPositionSignal();
    pc_DLP.startPatSequence();

    if (m_InjectionSettings.ContinuousInjection == ON){
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

int printcontrol::ReuploadHandler(QStringList ImageList, PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    if(m_PrintSettings.MotionMode == CONTINUOUS){
        pc_Stage.StageStop(m_PrintSettings.StageType);
    }
    return pc_DLP.PatternUpload(ImageList, m_PrintControls, m_PrintSettings, m_PrintScript);
}

void printcontrol::PrintProcessHandler(PrintControls *pPrintControls, PrintSettings m_PrintSettings)
{
    if(pPrintControls->InitialExposureFlag == true){
        pPrintControls->InitialExposureFlag = false;
        pPrintControls->inMotion = false;
        emit ControlPrintSignal("Exposing Initial Layer " + QString::number(m_PrintSettings.InitialExposure) + "s");
        emit UpdatePlotSignal();
    }
    else{
        pPrintControls->layerCount++;
        pPrintControls->remainingImages--;
    }
}

/***************************************Helpers*********************************************/
double printcontrol::CalcPrintEnd(PrintControls m_PrintControls, PrintSettings m_PrintSettings)
{
    double PrintEnd = 0;
    if(m_PrintSettings.MotionMode == CONTINUOUS){
        if(m_PrintSettings.PrinterType == CLIP30UM){
            PrintEnd = m_PrintSettings.StartingPosition - (m_PrintControls.nSlice*m_PrintSettings.LayerThickness);
        }
        else if (m_PrintSettings.PrinterType == ICLIP){
            PrintEnd = m_PrintControls.nSlice*m_PrintSettings.LayerThickness;
        }
        //PrintToTerminal("Print end for continuous motion print set to: " + QString::number(m_PrintControls.PrintEnd));
    }
    return PrintEnd;
}

ExposureType_t printcontrol::GetExposureType(PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    ExposureType_t ExposureType;
    if (m_PrintScript.PrintScript == 1){
        if (m_PrintSettings.PumpingMode == 1){
            ExposureType = EXPOSURE_PS_PUMP;
        }
        else{
            ExposureType = EXPOSURE_PS;
        }
    }
    else{
        if (m_PrintSettings.PumpingMode == 1){
            ExposureType = EXPOSURE_PUMP;
        }
        else{
            ExposureType = EXPOSURE_NORM;
        }
    }
    return ExposureType;
}
