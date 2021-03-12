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
#include <QDateTime>
#include <QSettings>
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
#include "manualprojcontrol.h"

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
static uint32_t nSlice = 0;
static uint32_t layerCount = 0;

static QDateTime CurrentDateTime;
static QString LogFileDestination;
static QString ImageFileDirectory = "C://";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)

{
    ui = (new Ui::MainWindow);
    ui->setupUi(this);
    usbPollTimer = new QTimer(this);
    usbPollTimer->setInterval(2000);
    connect(usbPollTimer, SIGNAL(timeout()), this, SLOT(timerTimeout()));
    usbPollTimer->start();
    CurrentDateTime = QDateTime::currentDateTime();
    loadSettings();
    initSettings();
}

void MainWindow::timerTimeout(void)
{

}

MainWindow::~MainWindow()
{
    USB_Close();
    USB_Exit();
    saveText();
    saveSettings();

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
    SliceThickness = (ui->SliceThicknessParam->value()/1000);
    QString ThicknessString = "Set Slice Thickness to: ";
    ThicknessString += QString::number(SliceThickness*1000);
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
    DarkTime = (ui->DarkTimeParam->value()*1000);
    QString DarkTimeString = "Set Dark Time to: ";
    DarkTimeString += QString::number(DarkTime/1000);
    DarkTimeString += " μs";
    ui->ProgramPrints->append(DarkTimeString);
}

void MainWindow::on_SetExposureTime_clicked()
{
    ExposureTime = (ui->ExposureTimeParam->value()*1000);
    QString ExposureTimeString = "Set Exposure Time to: ";
    ExposureTimeString += QString::number(ExposureTime/1000);
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
    for (uint16_t i = 0; i < file_name.count(); i++)
    {
       ui->FileList->addItem(file_name.at(i));
    }
}

void MainWindow::on_LogFileBrowse_clicked()
{
    LogFileDestination = QFileDialog::getExistingDirectory(this, "Open Log File Destination");
    ui->LogFileLocation->setText(LogFileDestination);
}

void MainWindow::on_ClearImageFiles_clicked()
{
    ui->FileList->clear();
}

void MainWindow::on_InitializeAndSynchronize_clicked()
{
    LCR_PatternDisplay(0);
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
        //LCR_SetMode(PTN_MODE_OTF);
        DLP.AddPatterns(imageList,ExposureTime,DarkTime,UVIntensity);
        DLP.updateLUT();
        unsigned int NumLutEntries;
        BOOL Repeat;
        unsigned int NumPatsForTrigOut2;
        unsigned int NumSplash;
        LCR_GetPatternConfig(&NumLutEntries,&Repeat,&NumPatsForTrigOut2, &NumSplash);
        ui->ProgramPrints->append("Number of LUT Entries: " + QString::number(NumLutEntries));
        ui->ProgramPrints->append("Number of Splash Images: " + QString::number(NumSplash));
        if (Repeat)
        {
            ui->ProgramPrints->append("Repeat is set to true");
        }
        else
        {
            ui->ProgramPrints->append("Repeat is set to false");
        }
    }
}


void MainWindow::on_StageConnectButton_clicked()
{
    QString COMSelect = ui->COMPortSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (SMC.SMC100CInit(COM) == true && SMC.Home() == true)
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
        uint Code;
        if (LCR_ReadErrorCode(&Code) >= 0)
        {
            ui->ProgramPrints->append("Last Error Code: " + QString::number(Code));
        }
        else
        {
            ui->ProgramPrints->append("Failed to get last error code");
        }
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
        nSlice = ui->FileList->count();
        ui->ProgramPrints->append("Entering Printing Procedure");
        //Set LED currents to 0 red, 0 green, set blue to chosen UVIntensity
        LCR_SetLedCurrents(0, 0, UVIntensity);
        //LCR_PatternDisplay(0);
        DLP.startPatSequence();
        PrintProcess();
    }
}

void MainWindow::PrintProcess(void)
{
    if (layerCount <= nSlice)
    {
        QTimer::singleShot(ExposureTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
        ExposureFlag = true;

        //QListWidgetItem item = ui->FileList->item(layerCount);
        QString filename =ui->FileList->item(layerCount)->text();
        QPixmap img(filename);
        QPixmap img2 = img.scaled(560,350, Qt::KeepAspectRatio);
        ui->PrintImage->setPixmap(img2);

        ui->ProgramPrints->append("Exposing: " + QString::number(ExposureTime/1000) + " ms");
        ui->ProgramPrints->append("Image File: " + filename);
        layerCount++;
        return;
    }
    else
    {
        USB_Close();
        ui->ProgramPrints->append("Print Complete");
        return;
    }
}

void MainWindow::ExposureTimeSlot(void)
{
    QTimer::singleShot(DarkTime/1000, Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
    SMC.RelativeMove(SliceThickness);
    ui->ProgramPrints->append("Dark Time: " + QString::number(DarkTime/1000) + " ms");
    ui->ProgramPrints->append("Moving Stage: " + QString::number(SliceThickness) + " um");
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

void MainWindow::saveText()
{
     QString Log = ui->ProgramPrints->toPlainText();
     QString LogDate = CurrentDateTime.toString("yyyy-MM-dd");
     QString LogTime = CurrentDateTime.toString("hh.mm.ss");
     QString FileDirectory;
     QString LogTitle = LogFileDestination + "/CLIPGUITEST_" + LogDate + "_" + LogTime + ".txt";
     ui->ProgramPrints->append(LogTitle);
     QFile file(LogTitle);
     if (file.open(QIODevice::WriteOnly | QFile::Text))
     {
         QTextStream out(&file);
         out << Log;
         file.flush();
         file.close();
     }

}

void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("ExposureTime", ExposureTime);
    settings.setValue("DarkTime", DarkTime);
    settings.setValue("UVIntensity", UVIntensity);
    settings.setValue("SliceThickness", SliceThickness);
    settings.setValue("StageVelocity", StageVelocity);
    settings.setValue("StageAcceleration", StageAcceleration);
    settings.setValue("MaxEndOfRun", MaxEndOfRun);
    settings.setValue("MinEndOfRun", MinEndOfRun);

    settings.setValue("LogFileDestination", LogFileDestination);
}

void MainWindow::loadSettings()
{
    QSettings settings;
    ExposureTime = settings.value("ExposureTime", 1000).toDouble();
    DarkTime = settings.value("DarkTime", 1000).toDouble();
    UVIntensity = settings.value("UVIntensity", 12).toDouble();
    SliceThickness = settings.value("SliceThickness", 200).toDouble();
    StageVelocity = settings.value("StageVelocity", 10).toDouble();
    StageAcceleration = settings.value("StageAcceleration", 5).toDouble();
    MaxEndOfRun = settings.value("MaxEndOfRun", 60).toDouble();
    MinEndOfRun = settings.value("MinEndOfRun", 0).toDouble();

    LogFileDestination = settings.value("LogFileDestination", "C://").toString();
}

void MainWindow::initSettings()
{
    ui->ExposureTimeParam->setValue(ExposureTime/1000);
    ui->DarkTimeParam->setValue(DarkTime/1000);
    ui->UVIntensityParam->setValue(UVIntensity);
    ui->SliceThicknessParam->setValue(SliceThickness*1000);
    ui->StageVelocityParam->setValue(StageVelocity);
    ui->StageAccelParam->setValue(StageAcceleration);
    ui->MaxEndOfRun->setValue(MaxEndOfRun);
    ui->MinEndOfRunParam->setValue(MinEndOfRun);

    ui->LogFileLocation->setText(LogFileDestination);
}
/*************************************************************
 * ********************OUTSIDE CODE***************************
 * ***********************************************************/

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





void MainWindow::on_ManualLightEngine_clicked()
{
    //ManualProjUI = new manualLEcontrol();
    //ManualProjUI->show();
    uint Code = 100;

    if (LCR_ReadErrorCode(&Code) >= 0)
    {
        ui->ProgramPrints->append("Last Error Code: " + QString::number(Code));
    }
    else
    {
        ui->ProgramPrints->append("Failed to get Error Code");
    }
    ui->ProgramPrints->append("Last Error Code: " + QString::number(Code));
    unsigned char HWStatus, SysStatus, MainStatus;
    if (LCR_GetStatus(&HWStatus, &SysStatus, &MainStatus) == 0)
    {
        if(SysStatus & BIT0)
                   ui->ProgramPrints->append("Internal Memory Test Passed");
        else
                    ui->ProgramPrints->append("Internal Memory Test Failed");

        if(HWStatus & BIT0)
                    ui->ProgramPrints->append("Internal Initialization Succesful");
        else
                    ui->ProgramPrints->append("Internal Initialization Failed");
        if(HWStatus & BIT1)
                    ui->ProgramPrints->append("Incompatible Controller");
        else
                    ui->ProgramPrints->append("Controller is Compatible");

        if(HWStatus & BIT4)
                    ui->ProgramPrints->append("Slave Controller Ready and Present");
        else
                    ui->ProgramPrints->append("No Slave Controller Present");

        if(HWStatus & BIT2)
                    ui->ProgramPrints->append("DMD Reset Controller Error");
        else
                    ui->ProgramPrints->append("No DMD Reset Controller Error");

        if(HWStatus & BIT3)
                    ui->ProgramPrints->append("Forced Swap Error");
        else
                    ui->ProgramPrints->append("No Forced Swap Error");

        if(HWStatus & BIT6)
                    ui->ProgramPrints->append("Sequencer has detected an error condition causing an Abort");
        else
                    ui->ProgramPrints->append("No Sequencer Errors have Occured");

        if(HWStatus & BIT7)
                    ui->ProgramPrints->append("Sequencer has detected an error");
        else
                    ui->ProgramPrints->append("No Sequencer Error has Occurred");

        if(MainStatus & BIT0)
                    ui->ProgramPrints->append("DMD Micromirrors Parked");
        else
                    ui->ProgramPrints->append("DMD Micromirrors Not Parked");

        if(MainStatus & BIT1)
                    ui->ProgramPrints->append("Sequencer is Running Normally");
        else
                    ui->ProgramPrints->append("Sequencer is Stopped");

        if(MainStatus & BIT2)
                    ui->ProgramPrints->append("Video is Frozen (Display Single Frame)");
        else
                    ui->ProgramPrints->append("Video is Running (Normal Frame Change");

        if(MainStatus & BIT3)
                    ui->ProgramPrints->append("External Video Source Locked");
        else
                    ui->ProgramPrints->append("External Video Source Not Locked");
    }
    //else if(LCR_GetBLStatus(&BLStatus) == 0)
    //{
        //This means the device is in boot mode
    //}
    else
    {
        ui->ProgramPrints->append("Failed to get Status");
    }

}



void MainWindow::on_pushButton_clicked()
{
    saveText();
}
