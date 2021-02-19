#include <stdio.h>
#include <stdlib.h>
#include <QFileDialog>
#include <QFileInfo>
#include <QProgressDialog>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QDateTime>
#include <QTextStream>
#include <QInputDialog>
#include <QDesktopServices>
#include <time.h>

#include "BMPParser.h"
#include "API.h"
#include "string.h"
#include "usb.h"
//#include "version.h"
#include "firmware.h"
#include "splash.h"
#include "flashloader.h"
//#include "PtnImage.h"
#include "batchfile.h"
//#include "FirmwareW.h"
#include <string>
#include <windows.h>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <Qt>
#include <QElapsedTimer>

#include "serialib.h"
#include "SMC100C.h"
#include "dlp9000.h"
#include "manualstagecontrol.h"

#define NormalTime
#define QuickTime

//Module level variables
static SMC100C SMC;
static DLP9000 DLP;
static bool PrintFlag = 0;
//Settings are static so they can be accessed from outside anywhere in this module
static double SliceThickness;
static double StageVelocity;
static double StageAcceleration;
static double MaxEndOfRun;
static double MinEndOfRun;
static double ExposureTime;
static double DarkTime;
static int UVIntensity;


static bool ExposureFlag = false;
static bool DarkTimeFlag = false;
static uint32_t nSlice = 10;
static uint32_t layerCount = 0;





MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)

{
    ui = (new Ui::MainWindow);
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_ManualStage_clicked()
{
    ManualStageUI = new ManualStageControl();
    ManualStageUI->show();
    SMC.Home();
    ui->SliceThicknessParam->setValue(50);
    ui->StageVelocityParam->setValue(5);

}

void MainWindow::on_SetSliceThickness_clicked()
{
    SliceThickness = (ui->SliceThicknessParam->value());
    QString ThicknessString = "Set Slice Thickness to: ";
    ThicknessString += QString::number(SliceThickness);
    ThicknessString += " μm";
    ui->ProgramPrints->append(ThicknessString);
}

void MainWindow::on_SetStageVelocity_clicked()
{
    StageVelocity = (ui->StageVelocityParam->value());
    SMC.SetVelocity(StageVelocity);
    QString VelocityString = "Set Stage Velocity to: ";
    VelocityString += QString::number(StageVelocity);
    VelocityString += " mm/s";
    ui->ProgramPrints->append(VelocityString);
}

void MainWindow::on_SetStageAcceleration_clicked()
{
    StageAcceleration = (ui->StageAccelParam->value());
    SMC.SetAcceleration(StageAcceleration);
    QString AccelerationString = "Set Stage Acceleration to: ";
    AccelerationString += QString::number(StageAcceleration);
    //AccelerationString += "mm/s";
    ui->ProgramPrints->append(AccelerationString + "mm/s");
}

void MainWindow::on_SetMaxEndOfRun_clicked()
{
    MaxEndOfRun = (ui->MaxEndOfRun->value());
    SMC.SetPositiveLimit(MaxEndOfRun);
    QString MaxEndOfRunString = "Set Max End Of Run to: ";
    MaxEndOfRunString += QString::number(MaxEndOfRun);
    MaxEndOfRunString += " mm";
    ui->ProgramPrints->append(MaxEndOfRunString);
}

void MainWindow::on_SetMinEndOfRun_clicked()
{
    MinEndOfRun = (ui->MinEndOfRunParam->value());
    SMC.SetNegativeLimit(MinEndOfRun);
    QString MinEndOfRunString = "Set Min End Of Run to: ";
    MinEndOfRunString += QString::number(MinEndOfRun);
    MinEndOfRunString += " mm";
    ui->ProgramPrints->append(MinEndOfRunString);
}

void MainWindow::on_SetDarkTime_clicked()
{
    DarkTime = (ui->DarkTimeParam->value());
    QString DarkTimeString = "Set Dark Time to: ";
    DarkTimeString += QString::number(DarkTime);
    DarkTimeString += " μs";
    ui->ProgramPrints->append(DarkTimeString);
}

void MainWindow::on_SetExposureTime_clicked()
{
    ExposureTime = (ui->ExposureTimeParam->value());
    QString ExposureTimeString = "Set Exposure Time to: ";
    ExposureTimeString += QString::number(ExposureTime);
    ExposureTimeString += " ms";
    ui->ProgramPrints->append(ExposureTimeString);
}

void MainWindow::on_SetUVIntensity_clicked()
{
    UVIntensity = (ui->UVIntensityParam->value());
    QString UVIntensityString = "Set UV Intensity to: ";
    UVIntensityString += QString::number(UVIntensity);
    ui->ProgramPrints->append(UVIntensityString);
}

void MainWindow::on_LiveValueList1_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV1: " + arg1);
}

void MainWindow::on_LiveValueList2_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV2: " + arg1);
}

void MainWindow::on_LiveValueList3_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV3: " + arg1);
}

void MainWindow::on_LiveValueList4_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV4: " + arg1);
}

void MainWindow::on_LiveValueList5_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV5: " + arg1);
}

void MainWindow::on_LiveValueList6_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV6: " + arg1);
}

void MainWindow::on_ResinSelect_activated(const QString &arg1)
{
    ui->ProgramPrints->append(arg1 + " Selected");
}


void MainWindow::on_SelectFile_clicked()
{
    QStringList file_name = QFileDialog::getOpenFileNames(this,"Open Object Image Files","C://","*.bmp *.png *.tiff *.tif");
    for (uint16_t i = 0; i < file_name.size(); i++)
    {
       ui->FileList->addItem(file_name.at(i));
    }
}

void MainWindow::on_ClearImageFiles_clicked()
{
    ui->FileList->clear();
}

void MainWindow::on_InitializeAndSynchronize_clicked()
{
    if (ui->FileList->count() > 0)
    {
        QListWidgetItem * item;
        QStringList imageList;
        for(int i = 0; i < ui->FileList->count(); i++)
        {
            item = ui->FileList->item(i);
            imageList << item->text();
        }
        QDir dir = QFileInfo(QFile(imageList.at(0))).absoluteDir();
        ui->ProgramPrints->append(dir.absolutePath());
        LCR_SetMode(PTN_MODE_OTF);
        DLP.AddPatterns(imageList,ExposureTime,DarkTime,UVIntensity);
        DLP.updateLUT();
        unsigned int NumLutEntries;
        BOOL Repeat;
        unsigned int NumPatsForTrigOut2;
        unsigned int NumSplash;
        LCR_GetPatternConfig(&NumLutEntries,&Repeat,&NumPatsForTrigOut2, &NumSplash);
        ui->ProgramPrints->append(QString::number(NumLutEntries));
        ui->ProgramPrints->append(QString::number(NumSplash));
        if (Repeat)
        {
            ui->ProgramPrints->append("Repeat is set to true");
        }
        else
        {
            ui->ProgramPrints->append("Repeat is set to true");
        }
    }
}


void MainWindow::on_StageConnectButton_clicked()
{
    QString COMSelect = ui->COMPortSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (SMC.SMC100CInit(COM) == true)
    {
        ui->ProgramPrints->append("Stage Connected");
        ui->StageConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->StageConnectionIndicator->setText("Connected");
    }
    else
    {
        ui->ProgramPrints->append("Stage Connection Failed");
        ui->StageConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->StageConnectionIndicator->setText("Disconnected");
    }
}

void MainWindow::on_LightEngineConnectButton_clicked()
{
    if (DLP.InitProjector())
    {
        ui->ProgramPrints->append("Light Engine Connected");
        ui->LightEngineIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->LightEngineIndicator->setText("Connected");
    }
    else
    {
        ui->ProgramPrints->append("Light Engine Connection Failed");
        ui->LightEngineIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->LightEngineIndicator->setText("Disconnected");
    }
}


void MainWindow::on_StartPrint_clicked()
{
    //If settings are validated successfully and Initialization has been completed
    if (ValidateSettings() == true)
    {
        //Set PrintFlag to true
        PrintFlag = true;
        ui->ProgramPrints->append("Entering Printing Procedure");
        //Set LED currents to 0 red, 0 green, set blue to chosen UVIntensity
        LCR_SetLedCurrents(0, 0, UVIntensity);
        PrintProcess();
        DLP.startPatSequence();
    }
}

void MainWindow::PrintProcess(void)
{
    if (layerCount <= nSlice)
    {
        QTimer::singleShot(ExposureTime, this, SLOT(ExposureTimeSlot()));
        ExposureFlag = true;
        layerCount++;

        //QListWidget item = ui->FileList->item(layerCount);
        //QString filename = item <<item->text();

        ui->ProgramPrints->append("Exposing");
        return;
    }
    else
    {
        return;
    }
}

void MainWindow::PreviewImageLoad()
{
    QListWidgetItem* item = ui->FileList->item(layerCount);
    //QString filename = item << item->text();
}

/*
 *
 *
 *
 *
 * bool ImageViewer::loadFile(const QString &fileName)
{
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    }
 * */





void MainWindow::ExposureTimeSlot(void)
{
    QTimer::singleShot(DarkTime, this, SLOT(DarkTimeSlot()));
    SMC.RelativeMove(SliceThickness/1000);
    ui->ProgramPrints->append("Dark Time");
}

void MainWindow::DarkTimeSlot(void)
{
    PrintProcess();
    //QTimer::singleShot(10, this, SLOT(PrintProcess()));
}

void MainWindow::CheckDLPStatus(void)
{
    //If GUI is in exposure state
    if (ExposureFlag == true && DarkTimeFlag == false)
    {
        //Query DLP status
        if (true) //If DLP has changed to dark time
        {

        }
        else //If DLP has not changed states
        {
            //Wait 20 ms and repeat this call
            QTimer::singleShot(20, this, SLOT(CheckDLPStatus()));
        }
    }
    else if (ExposureFlag == false && DarkTimeFlag == true)
    {
        //Query DLP status
    }
    else
    {
        showError("Flag error");
        return;
    }
}




//Validate all settings
bool MainWindow::ValidateSettings(void)
{
    //Validate Slicethickness
    if (SliceThickness <= 0)
    {
        showError("Invalid Slice Thickness");
        ui->ProgramPrints->append("Invalid Slice Thickness");
        return false;
    }
    //Validate StageVelocity
    else if (StageVelocity <= 0)
    {
        showError("Invalid Stage Velocity");
        ui->ProgramPrints->append("Invalid Stage Velocity");
        return false;
    }
    //Validate StageAcceleration
    else if (StageAcceleration <= 0)
    {
        showError("Invalid Stage Acceleration");
        ui->ProgramPrints->append("Invalid Stage Acceleration");
        return false;
    }
    //Validate MaxEndOfRun
    else if (MaxEndOfRun <= 0)
    {
        showError("Invalid Max End Of Run");
        ui->ProgramPrints->append("Invalid Max End Of Run");
        return false;
    }
    //Validate StageMinEndOfRun
    else if (MinEndOfRun <= -6)
    {
        showError("Invalid Min End Of Run");
        ui->ProgramPrints->append("Invalid Min End Of Run");
        return false;
    }
    //Validate DarkTime
    else if (DarkTime <= 0)
    {
        showError("Invalid Dark Time");
        ui->ProgramPrints->append("Invalid Dark Time");
        return false;
    }
    //Validate ExposureTime
    else if (ExposureTime <= 0)
    {
        showError("Invalid Exposure Time");
        ui->ProgramPrints->append("Invalid Exposure Time");
        return false;
    }
    //Validate UVIntensity
    else if (UVIntensity < 1 || UVIntensity > 255)
    {
        showError("Invalid UV Intensity");
        ui->ProgramPrints->append("Invalid UVIntensity");
        return false;
    }
    ui->ProgramPrints->append("All Settings Valid");
    return true;
}

/**
 * @brief MainWindow::showError
 * helper function to show the appropriate API Error message
 * @param errMsg - I - error messgae to be shown
 */
void MainWindow::showError(QString errMsg)
{
    QMessageBox errMsgBox;
    char errStr[128];

    if(LCR_ReadErrorString(errStr)<0)
        errMsgBox.setText("Error: " + errMsg);
    else
        errMsgBox.setText(errMsg + "\nError: " + QString::fromLocal8Bit(errStr));

    errMsgBox.exec();
}

/*************************************************************
 * ********************OUTSIDE CODE***************************
 * ***********************************************************/

/************************DLP9000 CODE**************************/


bool MainWindow::nanosleep(double ns)
{
    /* Declarations */
    HANDLE timer;   /* Timer handle */
    LARGE_INTEGER li;   /* Time defintion */
    /* Create timer */
    if(!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
       return FALSE;
     /* Set timer properties */
     li.QuadPart = -ns;
     if(!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)){
        CloseHandle(timer);
        return FALSE;
      }
     /* Start & wait for timer */
     WaitForSingleObject(timer, INFINITE);
     /* Clean resources */
      CloseHandle(timer);
     /* Slept without problems */
     return TRUE;
}

/*
void CallError(QString Error)
{
    MainWindow Main;
    Main.showError(Error);
}
*/




