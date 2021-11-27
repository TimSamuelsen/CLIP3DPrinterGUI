#ifndef PRINTSETTINGS_H
#define PRINTSETTINGS_H

#include <QWidget>
#include <QSettings>
#include "PrintElements.h"

namespace Ui {
class printsettings;
}

class printsettings : public QWidget
{
    Q_OBJECT

public:
    explicit printsettings(QWidget *parent = nullptr);
    ~printsettings();
    void initSettingsPointers(PrintSettings *pPrintSettings, PrintControls *pPrintControls,
                           PrintScripts *pPrintScript, InjectionSettings *pInjectionSettings);
    void savePrintSettings();
    void loadPrintSettings();
    void initPrintSettings();

    void EnableParameter(Parameter_t Parameter, bool State);
    int FileListCount();
    QString FileListItem(int itemNum);


Q_SIGNALS:
    void SettingsPrint(QString);
    void updateScript();

private slots:
    void on_resinSelect_editTextChanged(const QString &arg1);

    void on_StartingPositionParam_valueChanged(double arg1);

    void on_BitDepthParam_valueChanged(int arg1);

    void on_MaxImageUpload_valueChanged(int arg1);

    void on_ResyncRateList_currentIndexChanged(const QString &arg1);

    void on_LayerThicknessParam_valueChanged(double arg1);

    void on_InitialExposureParameter_valueChanged(double arg1);

    void on_InitialDelayParam_valueChanged(int arg1);

    void on_UVIntensityParam_valueChanged(int arg1);

    void on_InitialExposureIntensityParam_valueChanged(int arg1);

    void on_ExposureTimeParam_valueChanged(double arg1);

    void on_DarkTimeParam_valueChanged(double arg1);

    void on_ContinuousMotionButton_clicked();

    void on_SteppedMotionButton_clicked();

    void on_pumpingCheckBox_clicked();

    void on_pumpingParameter_valueChanged(double arg1);

    void on_StageVelocityParam_valueChanged(double arg1);

    void on_MaxEndOfRunParam_valueChanged(double arg1);

    void on_StageAccelParam_valueChanged(double arg1);

    void on_MinEndOfRunParam_valueChanged(double arg1);

    void on_ContinuousInjection_clicked();

    void on_SteppedContInjection_clicked();

    void on_BaseInfusionParam_valueChanged(double arg1);

    void on_PreMovementCheckbox_clicked();

    void on_PostMovementCheckbox_clicked();

    void on_InjectionDelayParam_valueChanged(double arg1);

    void on_InjectionRateParam_valueChanged(double arg1);

    void on_VolPerLayerParam_valueChanged(double arg1);

    void on_InitialVolumeParam_valueChanged(double arg1);

    void on_UsePrintScript_clicked();

    void on_SelectPrintScript_clicked();

    void on_ClearPrintScript_clicked();

    void on_SelectFile_clicked();

    void on_ClearImageFiles_clicked();

    void on_PostExposureDelayParam_valueChanged(double arg1);

    void on_JerkTimeParam_valueChanged(double arg1);

private:
    Ui::printsettings *ui;
    PrintSettings *psPrintSettings;
    PrintControls *psPrintControls;
    PrintScripts *psPrintScript;
    InjectionSettings *psInjectionSettings;

    void initImageFiles(QStringList file_names);
    void initPrintScript(QString file_name);
};

#endif // PRINTSETTINGS_H
