#ifndef PRINTCONTROL_H
#define PRINTCONTROL_H

#include "PrintElements.h"
#include "manualstagecontrol.h"
#include "manualpumpcontrol.h"

class printcontrol
{
public:
    printcontrol();
    ManualStageControl Stage;
    manualpumpcontrol Pump;
    void InitializeSystem(QStringList ImageList, PrintSettings p_PrintSettings, PrintControls *pPrintControls, PrintScripts m_PrintScript);

private:
    double CalcPrintEnd(PrintControls m_PrintControls, PrintSettings m_PrintSettings);
};



#endif // PRINTCONTROL_H
