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

bool printcontrol::VPFrameUpdate(PrintControls *pPrintControls, PrintSettings m_PrintSettings)
{
    bool returnVal = false;
    if (pPrintControls->BitLayer > 24){
        pPrintControls->FrameCount++;
        emit ControlPrintSignal("New Frame: " + QString::number(pPrintControls->FrameCount));
        pPrintControls->BitLayer = 1;
        pPrintControls->ReSyncCount++;

        //If 120 frames have been reached, prepare for resync
        if(pPrintControls->ReSyncCount > (120 - 24)/(24/m_PrintSettings.BitMode)){
            pPrintControls->ReSyncFlag = ON;
            pPrintControls->ReSyncCount = 0;
        }
        returnVal = true;
    }
    return returnVal;
}

void printcontrol::DarkTimeHandler(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript, InjectionSettings m_InjectionSettings)
{
    if(m_PrintSettings.PrinterType == ICLIP){
        if (m_InjectionSettings.InjectionDelayFlag == PRE){
            PrintInfuse(m_InjectionSettings);
            QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(StageMove(m_PrintControls, m_PrintSettings, m_PrintScript)));
            emit ControlPrintSignal("Pre-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
        }
        else if (m_InjectionSettings.InjectionDelayFlag == POST){
            StageMove(m_PrintControls, m_PrintSettings, m_PrintScript);
            QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(m_InjectionSettings));
            emit ControlPrintSignal("Post-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
        }
        else{
            PrintInfuse(m_InjectionSettings);
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

void printcontrol::StagePumpingHandler(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    if(m_PrintScript.PrintScript == ON){
        if (m_PrintControls.layerCount < m_PrintScript.PumpHeightScriptList.size()){
            pc_Stage.StageRelativeMove(-m_PrintScript.PumpHeightScriptList.at(m_PrintControls.layerCount).toDouble(), m_PrintSettings.StageType);
            emit ControlPrintSignal("Pumping " + QString::number(m_PrintScript.PumpHeightScriptList.at(m_PrintControls.layerCount).toDouble()*1000) +" um");
        }
    }
    else{
        pc_Stage.StageRelativeMove(-m_PrintSettings.PumpingParameter, m_PrintSettings.StageType);
        emit ControlPrintSignal("Pumping " + QString::number(m_PrintSettings.PumpingParameter*1000) +" um");
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

void printcontrol::StageMove(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    //If printscript is active, filter out layerCount = 0 and layerCount is greater than script length
    if (m_PrintScript.PrintScript == ON){
        if (m_PrintControls.layerCount > 0){
            if (m_PrintControls.layerCount < m_PrintScript.LayerThicknessScriptList.size() && m_PrintControls.layerCount < m_PrintScript.StageAccelerationScriptList.size()){
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

void printcontrol::PrintInfuse(InjectionSettings m_InjectionSettings)
{
    if (m_InjectionSettings.ContinuousInjection == OFF){
        pc_Pump.ClearVolume();
        Sleep(10);
        pc_Pump.StartInfusion();
        emit ControlPrintSignal("Injecting resin");
    }
}
