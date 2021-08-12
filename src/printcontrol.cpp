#include "printcontrol.h"
#include "dlp9000.h"

DLP9000& pc_DLP = DLP9000::Instance();

printcontrol::printcontrol()
{

}

void printcontrol::InitializeSystem(QStringList ImageList, PrintSettings m_PrintSettings, PrintControls *pPrintControls, PrintScripts m_PrintScript)
{
    pc_DLP.PatternDisplay(OFF);
    Stage.initStagePosition(m_PrintSettings);
    pc_DLP.PatternUpload(ImageList, *pPrintControls, m_PrintSettings, m_PrintScript);
    pPrintControls->PrintEnd = CalcPrintEnd(*pPrintControls, m_PrintSettings);
}




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
