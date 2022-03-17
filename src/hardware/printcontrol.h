#ifndef PRINTCONTROL_H
#define PRINTCONTROL_H

#include "PrintElements.h"
#include <QObject>

/*!
 * \brief The printcontrol class handles hardware operation during the print.
 */
class printcontrol: public QObject {
  Q_OBJECT

 public:
  printcontrol();
  void getControlPointers(PrintSettings* pPrintSettings, PrintControls* pPrintControls, PrintScripts* pPrintScript);
  void InitializeSystem(QStringList ImageList, PrintSettings p_PrintSettings, PrintControls* pPrintControls, PrintScripts m_PrintScript, InjectionSettings m_InjectionSettings);
  void AbortPrint(Stage_t StageType, PrintControls* pPrintControl);
  void StartPrint(PrintSettings m_PrintSettings, PrintScripts m_PrintScript, bool ContinuousInjection);
  int ReuploadHandler(QStringList ImageList, PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript, bool ContinuousInjection);
  void PrintProcessHandler(PrintControls* pPrintControls, PrintSettings m_PrinSettings, InjectionSettings m_InjectionSettings);
  bool VPFrameUpdate(PrintControls* pPrintControls, int BitMode, int ReSyncRate);
  void DarkTimeHandler(PrintSettings m_PrintSettings, PrintScripts m_PrintScript, InjectionSettings m_InjectionSettings);
  void StagePumpingHandler(int layerCount, PrintSettings m_PrintSettings, PrintScripts m_PrintScript);

 signals:
  /*!
   * \brief ControlPrintSignal Print control terminal printing signal, connected to TerminalPrint function in Main Window.
   * \param StringToPrint String to be printed
   */
  void ControlPrintSignal(QString StringToPrint);
  /*!
   * \brief ControlError Print control signal, connected to showError function in Main Window.
   * \param ErrorString Error string to be shown
   */
  void ControlError(QString ErrorString);
  /*!
   * \brief GetPositionSignal
   */
  void GetPositionSignal();
  /*!
   * \brief UpdatePlotSignal
   */
  void UpdatePlotSignal();
  void StageMoveSignal(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript);
 private:
  PrintSettings* pcPrintSettings;
  PrintControls* pcPrintControls;
  PrintScripts* pcPrintScript;

  double CalcPrintEnd(uint nSlice, PrintSettings m_PrintSettings);
  ExposureType_t GetExposureType(int PrintScript, int PumpingMode);
 private slots:
  bool CheckReupload(PrintSettings m_PrintSettings, PrintControls m_PrintControls, PrintScripts m_PrintScript);
  void StageMove();
  void PrintInfuse(InjectionSettings m_InjectionSettings);
};



#endif // PRINTCONTROL_H
