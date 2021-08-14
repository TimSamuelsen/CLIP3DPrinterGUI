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
signals:
    void ControlPrintSignal(QString StringToPrint);
    void ControlError(QString ErrorString);
    void GetPositionSignal();
private:
    double CalcPrintEnd(PrintControls m_PrintControls, PrintSettings m_PrintSettings);
};



#endif // PRINTCONTROL_H
