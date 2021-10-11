#include "printsettings.h"
#include "ui_printsettings.h"

printsettings::printsettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::printsettings)
{
    ui->setupUi(this);
}

printsettings::~printsettings()
{
    delete ui;
}

void printsettings::initPrintSettings(PrintSettings *pPrintSettings, PrintControls *pPrintControls, PrintScripts *pPrintScript)
{
    psPrintSettings = pPrintSettings;
    psPrintControls = pPrintControls;
    psPrintScript = pPrintScript;
}

/*******************************General Print Settings*********************************************/
void printsettings::on_resinSelect_editTextChanged(const QString &arg1)
{
    ui->testTerm->append(arg1);
}

void printsettings::on_BitDepthParam_valueChanged(int arg1)
{

}

void printsettings::on_MaxImageUpload_valueChanged(int arg1)
{

}

void printsettings::on_StartingPositionParam_valueChanged(double arg1)
{
    ui->testTerm->append(QString::number(arg1));
    emit SettingsPrint(QString::number(arg1));
}

void printsettings::on_LayerThicknessParam_valueChanged(double arg1)
{

}

void printsettings::on_ResyncRateList_currentIndexChanged(const QString &arg1)
{
    ui->testTerm->append(arg1);
}


/*********************************Light Engine Control*********************************************/
void printsettings::on_InitialExposureParameter_valueChanged(double arg1)
{

}
void printsettings::on_InitialDelayParam_valueChanged(int arg1)
{

}

void printsettings::on_UVIntensityParam_valueChanged(int arg1)
{

}

void printsettings::on_InitialExposureIntensityParam_valueChanged(int arg1)
{

}

void printsettings::on_ExposureTimeParam_valueChanged(double arg1)
{

}

void printsettings::on_DarkTimeParam_valueChanged(double arg1)
{

}

/***********************************Stage Control**************************************************/
void printsettings::on_ContinuousMotionButton_clicked()
{

}

void printsettings::on_SteppedMotionButton_clicked()
{

}


void printsettings::on_pumpingCheckBox_clicked()
{

}

void printsettings::on_pumpingParameter_valueChanged(double arg1)
{

}


void printsettings::on_StageVelocityParam_valueChanged(double arg1)
{

}

void printsettings::on_StageAccelParam_valueChanged(double arg1)
{

}

void printsettings::on_MaxEndOfRunParam_valueChanged(double arg1)
{

}

void printsettings::on_MinEndOfRunParam_valueChanged(double arg1)
{

}

/***********************************Injection Control**********************************************/
void printsettings::on_ContinuousInjection_clicked()
{

}

void printsettings::on_SteppedContInjection_clicked()
{

}

void printsettings::on_BaseInfusionParam_valueChanged(double arg1)
{

}

void printsettings::on_PreMovementCheckbox_clicked()
{

}

void printsettings::on_PostMovementCheckbox_clicked()
{

}

void printsettings::on_InjectionDelayParam_valueChanged(double arg1)
{

}

void printsettings::on_InjectionRateParam_valueChanged(double arg1)
{

}

void printsettings::on_VolPerLayerParam_valueChanged(double arg1)
{

}

void printsettings::on_InitialVolumeParam_valueChanged(double arg1)
{

}

/***********************************Dynamic Print Mode*********************************************/
void printsettings::on_UsePrintScript_clicked()
{

}

void printsettings::on_SelectPrintScript_clicked()
{

}

void printsettings::on_ClearPrintScript_clicked()
{

}

/**********************************Image File Selection********************************************/
void printsettings::on_SelectFile_clicked()
{

}

void printsettings::on_ClearImageFiles_clicked()
{

}

#if 0

#endif
