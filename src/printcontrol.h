#ifndef PRINTCONTROL_H
#define PRINTCONTROL_H

#include "PrintElements.h"
#include <QObject>

class printcontrol: public QObject
{
    Q_OBJECT
public:
    printcontrol();
    void InitializeSystem(QStringList ImageList, PrintSettings p_PrintSettings, PrintControls *pPrintControls, PrintScripts m_PrintScript);    
    void AbortPrint(PrintSettings m_PrintSettings, PrintControls *pPrintControl);
    void StartPrint(PrintSettings m_PrintSettings, PrintScripts m_PrintScript, InjectionSettings m_InjectionSettings);
    void PrintProcessHandler(PrintControls *pPrintControls, PrintSettings m_PrintSettings);
    int ReuploadHandler(QStringList ImageList, PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript);
    bool VPFrameUpdate(PrintControls *pPrintControls, PrintSettings m_PrintSettings);
    void DarkTimeHandler(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript, InjectionSettings m_InjectionSettings);
    void StagePumpingHandler(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript);
signals:
    void ControlPrintSignal(QString StringToPrint);
    void ControlError(QString ErrorString);
    void GetPositionSignal();
    void UpdatePlotSignal();
private:
    double CalcPrintEnd(PrintControls m_PrintControls, PrintSettings m_PrintSettings);
    ExposureType_t GetExposureType(PrintSettings m_PrintSettings, PrintScripts m_PrintScript);
private slots:
    void StageMove(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript);
    void PrintInfuse(InjectionSettings m_InjectionSettings);
};



#endif // PRINTCONTROL_H
