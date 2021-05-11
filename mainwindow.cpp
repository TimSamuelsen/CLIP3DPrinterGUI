#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
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
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QSvgWidget>
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
#include "manualpumpcontrol.h"
#include <opencv2/opencv.hpp>

#define NormalTime
#define QuickTime

static DLP9000 DLP; //DLP object for calling functions from dlp9000.cpp
//Module level variables
static bool InitialExposureFlag = true; //Flag to indicate print process to do intial expore first, set false after intial exposure never set true again
//Settings are static so they can be accessed from outside anywhere in this module
//Module level variables for print parameters
static double SliceThickness;
static double StageVelocity;
static double StageAcceleration;
static double StartingPosition;
static double MaxEndOfRun;
static double MinEndOfRun;
static double ExposureTime;
static double DarkTime;
static uint InitialExposure;
static int UVIntensity;
static double PrintSpeed;
static double PrintHeight;
static int MaxImageUpload = 20;

static int remainingImages; //Keeps count of remaining images to be printed

static bool AutoModeFlag = false; //true when automode is on, false otherwise
static uint32_t nSlice = 0; //number of slices to be printed
static uint32_t layerCount = 0; //current layer
static double TotalPrintTimeS = 0; //For calculating print times
static double RemainingPrintTimeS; //For calculating remaining print times

static bool loadSettingsFlag = false; //For ensuring the settings are only loaded once, (May not be needed)
static QDateTime CurrentDateTime; //Current time
static QString LogFileDestination; //for storing log file destination in settings
static QString ImageFileDirectory; //For storing image file directory in settings
static QTime PrintStartTime; //Get start time for log
static double GetPosition; //Holds
static bool StagePrep1 = false; //For lowering stage
static bool StagePrep2 = false; //For lowering stage
static QStringList PrintScriptList; //Printscript is taken from a .csv file and stored in this variable
static int PrintScript = 0; //For selecting type of printscript, currently: 0 = no printscript, 1 = exposure time print script
static int ProjectionMode = 0; //For selecting projection mode, 0 = POTF mode, 1 = Video Pattern HDMI
static bool VideoLocked;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)

{
    ui = (new Ui::MainWindow);
    ui->setupUi(this);
    CurrentDateTime = QDateTime::currentDateTime();
    loadSettings();
    initSettings();
    initPlot();
}

void MainWindow::timerTimeout(void)
{
    //emit(on_GetPosition_clicked());

}

MainWindow::~MainWindow()
{
    saveSettings();
    //SMC.SMC100CClose();

    delete ui;
}


//Opens manual stage control window
void MainWindow::on_ManualStage_clicked()
{
    ManualStageUI = new ManualStageControl();
    ManualStageUI->show();
    SMC.Home(); //Home command is issued to make sure the stage is referenced upon startup
    SMC.SMC100CClose(); //Close main window connection to allow manual stage control to take over, ideally this would not be needed and it would stay on the same connection.
    ui->ProgramPrints->append("Manual Stage Control Entered");
    ui->StageConnectionIndicator->setStyleSheet("background:rgb(0, 255, 255); border: 1px solid black;");
    ui->StageConnectionIndicator->setText("Manual Control");
}

//Gets position, saves it in module level variable GetPosition, updates slider and print to terminal window
void MainWindow::on_GetPosition_clicked()
{
    for (int i = 0; i<5; i++) //To be removed...
    {
        QString delay = "This delay is needed for the serial read";
    }
    char* ReadPosition = SMC.GetPosition(); //Get position and store it in char*, could store directly in QString but if result is null it can cause issues
    if (strlen(ReadPosition) > 1) //In essence means that if there has not been an error -> update variable
    {
        printf("at mainwindow %s\r\n",ReadPosition);
        QString CurrentPosition = QString::fromUtf8(ReadPosition); //convert char* to QString
        CurrentPosition.remove(0,3); //Removes address and command code
        CurrentPosition.chop(2); //To be removed...
        ui->CurrentPositionIndicator->setText(CurrentPosition);
        ui->ProgramPrints->append("Stage is currently at: " + CurrentPosition + " mm");
        ui->CurrentStagePos->setSliderPosition(CurrentPosition.toDouble());
        GetPosition = CurrentPosition.toDouble();
    }
}

//Opens Manual Pump Control Window, closes mainwindow pump connection
void MainWindow::on_pushButton_clicked()
{
    ManualPumpUI = new manualpumpcontrol();
    ManualPumpUI->show();
    ui->ProgramPrints->append("Manual Pump Control Entered");
    Pump.PSerial.closeDevice(); //Closes mainwindow pump connection
}

//Opens Image Processing Window
void MainWindow::on_ImageProcess_clicked()
{
    ImageProcessUI = new imageprocessing();
    ImageProcessUI->show();
    ui->ProgramPrints->append("Opening Image Processing");
}

/*********************************************Mode Selection*********************************************/
void MainWindow::on_POTFcheckbox_clicked()
{
    if (ui->POTFcheckbox->isChecked())
    {
        ui->VP_HDMIcheckbox->setChecked(false);
        ProjectionMode = 0;
        DLP.setIT6535Mode(0); //not sure if needed
        LCR_SetMode(PTN_MODE_OTF);
    }
}


void MainWindow::on_VP_HDMIcheckbox_clicked()
{
    if (ui->VP_HDMIcheckbox->isChecked())
    {
        ui->POTFcheckbox->setChecked(false);
        ProjectionMode = 1;
        DLP.setIT6535Mode(1);
        Check4VideoLock();
        LCR_SetMode(PTN_MODE_VIDEO);
    }
}

void MainWindow::Check4VideoLock()
{
    QMessageBox VlockPopup;
    static uint8_t repeatCount = 0;
    static bool repeat;
    unsigned char HWStatus, SysStatus, MainStatus;
    if (repeatCount == 0)
        ui->ProgramPrints->append("Attempting to Lock to Video Source");
    if (LCR_GetStatus(&HWStatus, &SysStatus, &MainStatus) == 0)
    {
        if(MainStatus & BIT3){
            ui->ProgramPrints->append("External Video Source Locked");
            VideoLocked = true;
            repeat = false;
            VlockPopup.close();
            return;
        }
        else{
            ui->ProgramPrints->append("External Video Source Not Locked, Please Wait");
            VideoLocked = false;
            if(repeatCount < 15) //Repeats 15 times (15 seconds) before telling user that an error has occurred
            {
                QTimer::singleShot(1000, this, SLOT(Check4VideoLock()));
                repeatCount++;
            }
            else
            {
                ui->ProgramPrints->append("External Video Source Lock Failed");
                showError("External Video Source Lock Failed");
                ui->VP_HDMIcheckbox->setChecked(false);
                ui->POTFcheckbox->setChecked(true);
                emit(on_POTFcheckbox_clicked());
            }
        }
    }
}
/*********************************************Print Parameters*********************************************/
//Saves resin selected to log for reference
void MainWindow::on_ResinSelect_activated(const QString &arg1)
{
    ui->ProgramPrints->append(arg1 + " Selected");
}

//Sets whether auto mode is set
void MainWindow::on_AutoCheckBox_stateChanged(int arg1)
{
    if (arg1 == 2) //If automode is checked
    {
        int SliceCount = ui->FileList->count(); //# of images = # of slices
        if (SliceCount > 0)  //If there are images selected
        {
            ui->SetExposureTime->setEnabled(false);
            ui->ExposureTimeParam->setEnabled(false);

            ui->SetSliceThickness->setEnabled(false);
            ui->SliceThicknessParam->setEnabled(false);

            ui->PrintSpeedParam->setEnabled(true);
            ui->setPrintSpeed->setEnabled(true);

            ui->PrintHeightParam->setEnabled(true);
            ui->SetPrintHeight->setEnabled(true);

            AutoModeFlag = true;
            AutoMode();
        }
        else //With no images selected auto mode does not worked and the program reverts to normal operation
        {
            ui->AutoCheckBox->setChecked(false);
            ui->ProgramPrints->append("No Object Image Files Detected, Please Select Image Files First");
        }
    }
    else //Automode is not checked, revert to normal operation
    {
        ui->SetExposureTime->setEnabled(true);
        ui->ExposureTimeParam->setEnabled(true);

        ui->SetSliceThickness->setEnabled(true);
        ui->SliceThicknessParam->setEnabled(true);

        ui->PrintSpeedParam->setEnabled(false);
        ui->setPrintSpeed->setEnabled(false);

        ui->PrintHeightParam->setEnabled(false);
        ui->SetPrintHeight->setEnabled(false);

        ui->AutoCheckBox->setChecked(false);

        AutoModeFlag = false;
    }
}

//Needed to avoid the auto mode checkbox becoming checked
void MainWindow::on_AutoCheckBox_clicked()
{
    int SliceCount = ui->FileList->count();
    if (SliceCount < 1)
    {
        ui->AutoCheckBox->setChecked(false);
    }
}

//Sets max image upload, used to avoid large error buildup or large stop times from large image upload times
void MainWindow::on_SetMaxImageUpload_clicked()
{
    int MaxImageVal = ui->MaxImageUpload->value(); //Grab value from UI
    if (MaxImageVal < (InitialExposure + 1)) //Verifies that the max image upload is greater than initial exposure time
    {
        MaxImageVal = InitialExposure + 1;
    }
    else if (MaxImageVal > 398) //Verifies that max image upload is not greater than the storage limit on light engine
    {
        MaxImageVal = 398;
    }
    else //If MaxImageVal is is an acceptable range
    {
        MaxImageUpload = MaxImageVal; //This line might not be needed
    }
    MaxImageUpload = MaxImageVal;
    QString MaxImageUploadString = "Set Max Image Upload to: " + QString::number(MaxImageUpload);
    ui->ProgramPrints->append(MaxImageUploadString);
}

//Sets PrintSpeed variable from value inputted by user, also triggers automode
void MainWindow::on_setPrintSpeed_clicked()
{
    PrintSpeed = (ui->PrintSpeedParam->value());
    QString PrintSpeedString = "Set Print Speed to: " + QString::number(PrintSpeed) + " um/s";
    ui->ProgramPrints->append(PrintSpeedString);
    AutoMode();
}

//Sets PrintHeight variable from value inputted by user, also triggers automode
void MainWindow::on_SetPrintHeight_clicked()
{
    PrintHeight = (ui->PrintHeightParam->value());
    QString PrintHeightString = "Set Print Speed to: " + QString::number(PrintHeight) + "" ;
    ui->ProgramPrints->append(PrintHeightString);
    AutoMode();
}

//Sets InitialExposure variable from the value inputted by the user
void MainWindow::on_SetIntialAdhesionTimeButton_clicked()
{
    InitialExposure = (ui->InitialAdhesionParameter->value());
    QString InitialExposureString = "Set Initial Exposure to: " + QString::number(InitialExposure) + " s";
    ui->ProgramPrints->append(InitialExposureString);
}

//Sets Starting Position variable from the value inputted by the user (also doubles as debug tool for GetPosition(), will be removed)
void MainWindow::on_SetStartingPosButton_clicked()
{
    StartingPosition = (ui->StartingPositionParam->value());
    QString StartingPositionString = "Set Starting Position to: " + QString::number(StartingPosition) + " mm";
    ui->ProgramPrints->append(StartingPositionString);
    QString CurrentPosition = SMC.GetPosition();
    CurrentPosition = CurrentPosition.remove(0,3);
    ui->ProgramPrints->append("Stage is currently at: " + CurrentPosition + "mm");
    ui->CurrentPositionIndicator->setText(CurrentPosition);
}

//Sets Slicethickness variable from the value inputted by the user
void MainWindow::on_SetSliceThickness_clicked()
{
    SliceThickness = (ui->SliceThicknessParam->value()/1000);
    QString ThicknessString = "Set Slice Thickness to: " + QString::number(SliceThickness*1000) + " μm";
    ui->ProgramPrints->append(ThicknessString);
}

//Sets StageVelocity variable from the value inputted by the user, also directly sends command to stage to set velocity
void MainWindow::on_SetStageVelocity_clicked()
{
    StageVelocity = (ui->StageVelocityParam->value());
    SMC.SetVelocity(StageVelocity); //Sends command to stage
    QString VelocityString = "Set Stage Velocity to: " + QString::number(StageVelocity) +" mm/s";
    ui->ProgramPrints->append(VelocityString);
}

//Sets StageAcceleration variable from the value inputted by the user, also directly sends command to stage to set acceleration
void MainWindow::on_SetStageAcceleration_clicked()
{
    StageAcceleration = (ui->StageAccelParam->value());
    SMC.SetAcceleration(StageAcceleration); //Sends command to stage
    QString AccelerationString = "Set Stage Acceleration to: " + QString::number(StageAcceleration) + " mm/s";
    ui->ProgramPrints->append(AccelerationString);
}

//Sets MaxEndOfRun variable from the value inputted by the user, also directly sends command to stage to set max end of run
void MainWindow::on_SetMaxEndOfRun_clicked()
{
    MaxEndOfRun = (ui->MaxEndOfRun->value());
    SMC.SetPositiveLimit(MaxEndOfRun); //Sends command to stage
    QString MaxEndOfRunString = "Set Max End Of Run to: " + QString::number(MaxEndOfRun) + " mm";
    ui->ProgramPrints->append(MaxEndOfRunString);
}

//Sets MinEndOfRun variable from the value inputted by the user, also directly sends command to stage to set min end of run
void MainWindow::on_SetMinEndOfRun_clicked()
{
    MinEndOfRun = (ui->MinEndOfRunParam->value());
    SMC.SetNegativeLimit(MinEndOfRun); //Sends command to stage
    QString MinEndOfRunString = "Set Min End Of Run to: " + QString::number(MinEndOfRun) + " mm";
    ui->ProgramPrints->append(MinEndOfRunString);
}

//Sets DarkTime variable from the value inputted by the user
void MainWindow::on_SetDarkTime_clicked()
{
    DarkTime = (ui->DarkTimeParam->value()*1000);
    QString DarkTimeString = "Set Dark Time to: " + QString::number(DarkTime/1000) + " ms";
    ui->ProgramPrints->append(DarkTimeString);
}

//Sets ExposureTime variable from the value inputted by the user
void MainWindow::on_SetExposureTime_clicked()
{
    ExposureTime = (ui->ExposureTimeParam->value()*1000);
    QString ExposureTimeString = "Set Exposure Time to: " + QString::number(ExposureTime/1000) + " ms";
    ui->ProgramPrints->append(ExposureTimeString);
}

//Sets UVIntensity from the value inputted by the user
void MainWindow::on_SetUVIntensity_clicked()
{
    UVIntensity = (ui->UVIntensityParam->value());
    QString UVIntensityString = "Set UV Intensity to: " + QString::number(UVIntensity);
    ui->ProgramPrints->append(UVIntensityString);
}

//Currently Not In Use
void MainWindow::on_LiveValueList1_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV1: " + arg1);
}

//Currently Not In Use
void MainWindow::on_LiveValueList2_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV2: " + arg1);
}

//Currently Not In Use
void MainWindow::on_LiveValueList3_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV3: " + arg1);
}

//Currently Not In Use
void MainWindow::on_LiveValueList4_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV4: " + arg1);
}

//Currently Not In Use
void MainWindow::on_LiveValueList5_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV5: " + arg1);
}

//Currently Not In Use
void MainWindow::on_LiveValueList6_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV6: " + arg1);
}

/*********************************************File Handling*********************************************/
//Select all object image files to project
void MainWindow::on_SelectFile_clicked()
{
    //Open files from last directory chosen (stored in settings), limited to bitmapped image file formats
    QStringList file_name = QFileDialog::getOpenFileNames(this,"Open Object Image Files",ImageFileDirectory,"*.bmp *.png *.tiff *.tif *.svg");
    if (file_name.count() > 0) //If images were selected
    {
        QDir ImageDirectory = QFileInfo(file_name.at(0)).absoluteDir();
        ImageFileDirectory = ImageDirectory.filePath(file_name.at(0));
        for (uint16_t i = 0; i < file_name.count(); i++)
        {
            ui->FileList->addItem(file_name.at(i)); //Add each file name to FileList, will be displayed for user
        }
    }
    int SliceCount = ui->FileList->count();
    ui->ProgramPrints->append(QString::number(SliceCount) + " Images Currently Selected");
}

//Select directory to store log files
void MainWindow::on_LogFileBrowse_clicked()
{
    LogFileDestination = QFileDialog::getExistingDirectory(this, "Open Log File Destination");
    ui->LogFileLocation->setText(LogFileDestination);
}

//Clear image files selected
void MainWindow::on_ClearImageFiles_clicked()
{
    ui->FileList->clear();
    //initConfirmationScreen();
}

//Used to select whether to use set print variables or print script
void MainWindow::on_UsePrintScript_clicked()
{
    if (ui->UsePrintScript->checkState()) //If printscript is checked
    {
        PrintScript = 1;
        ui->SelectPrintScript->setEnabled(true);
        ui->ClearPrintScript->setEnabled(true);
        ui->PrintScriptFile->setEnabled(true);
    }
    else //printscript is not checked
    {
        PrintScript = 0;
        ui->SelectPrintScript->setEnabled(false);
        ui->ClearPrintScript->setEnabled(false);
        ui->PrintScriptFile->setEnabled(false);
    }
}

//
void MainWindow::on_SelectPrintScript_clicked()
{
    QString file_name = QFileDialog::getOpenFileName(this, "Open Print Script", "C://", "*.txt *.csv");


    ui->PrintScriptFile->setText(file_name);
    QFile file(file_name);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << file.errorString();
    }
    QStringList wordList;
    while (!file.atEnd())
    {
            QByteArray line = file.readLine();
            PrintScriptList.append(line.split(',').at(1));
    }
     //For testing
    //for (uint i= 0; i < PrintScriptList.size(); i++)
    //{
    //    ui->ProgramPrints->append(PrintScriptList.at(i));
    //    //PrintScriptList.removeAt(0);
    //}
    ui->ProgramPrints->append("Print List has: " + QString::number(PrintScriptList.size()) + " entries");

}


static int FPStestImage = 0;
static bool SVG = false;
static QTime FPSStartTime;


void MainWindow::on_ClearPrintScript_clicked()
{
    ui->PrintScriptFile->clear();
    if (PrintScript == 0)
    {
        FPSStartTime = QTime::currentTime();
        //SVG = true;
        testFPS();
        /*for (int i = 0; i < ui->FileList->count() ; i++)
        {
            QString filename =ui->FileList->item(i)->text();
            QPixmap img(filename);
            QPixmap img2 = img.scaled(890,490, Qt::KeepAspectRatio);
            ui->PrintImage->setPixmap(img);
            ui->ProgramPrints->append(QString::number(i));
            Sleep(20);
        }*/
    }
}

void MainWindow::testFPS()
{
    if(SVG)
    {
        if (FPStestImage < ui->FileList->count())
        {
            QString filename =ui->FileList->item(FPStestImage)->text();
            QSvgRenderer *renderer = new QSvgRenderer(filename);
            QGraphicsSvgItem *testImage = new QGraphicsSvgItem(filename);
            QImage image(900, 500, QImage::Format_Mono);
            QPainter painter(&image);
            renderer->render(&painter);
            QPixmap pix = QPixmap::fromImage(image);
            ui->PrintImage->setPixmap(pix);
            //ui->ProgramPrints->append(QString::number(FPStestImage));
            FPStestImage++;
            //ui->svgImage->
            //ui->svgImage->render(image);
            QTimer::singleShot(1, Qt::PreciseTimer, this, SLOT(testFPS()));
        }
        else
        {
            QTime FPSupdateTime = QTime::currentTime();
            double TimeElapsed = FPSStartTime.msecsTo(FPSupdateTime);
            ui->ProgramPrints->append("Elapsed Time: " + QString::number(TimeElapsed));
        }
    }
    else
    {
        if (FPStestImage < ui->FileList->count())
        {
            QString filename =ui->FileList->item(FPStestImage)->text();
            QPixmap img(filename);
            //QPixmap img2 = img.scaled(900,500, Qt::KeepAspectRatio);
            ui->PrintImage->setPixmap(img);
            ui->ProgramPrints->append(QString::number(FPStestImage));
            FPStestImage++;
            //testFPS();
            QTimer::singleShot(1, Qt::PreciseTimer, this, SLOT(testFPS()));
        }
        else
        {
            QTime FPSupdateTime = QTime::currentTime();
            double TimeElapsed = FPSStartTime.msecsTo(FPSupdateTime);
            ui->ProgramPrints->append("Elapsed Time: " + QString::number(TimeElapsed));
        }
    }
}


/*******************************************Peripheral Connections*********************************************/
void MainWindow::on_LightEngineConnectButton_clicked()
{
    if (DLP.InitProjector())
    {
        ui->ProgramPrints->append("Light Engine Connected");
        ui->LightEngineIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->LightEngineIndicator->setText("Connected");
        uint Code;
        //Their GUI does not call reader Errorcode
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

void MainWindow::on_StageConnectButton_clicked()
{
    SMC.SMC100CClose();
    QString COMSelect = ui->COMPortSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (SMC.SMC100CInit(COM) == true && SMC.Home() == true)
    {
        ui->ProgramPrints->append("Stage Connected");
        ui->StageConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->StageConnectionIndicator->setText("Connected");
        Sleep(10);
        emit(on_GetPosition_clicked());
    }
    else
    {
        ui->ProgramPrints->append("Stage Connection Failed");
        ui->StageConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->StageConnectionIndicator->setText("Disconnected");
    }
}

void MainWindow::on_PumpConnectButton_clicked()
{
    Pump.PSerial.closeDevice();
    QString COMSelect = ui->COMPortSelectPump->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (Pump.PSerial.openDevice(COM,9600) == 1)
    {
        ui->ProgramPrints->append("Pump Connected");
        ui->PumpConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->PumpConnectionIndicator->setText("Connected");
    }
    else
    {
        ui->ProgramPrints->append("Pump Connection Failed");
        ui->PumpConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->PumpConnectionIndicator->setText("Disconnected");
    }
}


/***************************************Print Functionality*********************************************/
void MainWindow::on_InitializeAndSynchronize_clicked()
{
    if (initConfirmationScreen())
    {
        LCR_PatternDisplay(0);

        initStageSlot();

        if (ui->FileList->count() > 0)
        {
            //Upload images for initial exposure
            QListWidgetItem * firstItem = ui->FileList->item(0);
            QStringList firstImage;
            for (uint i = 0; i < InitialExposure; i++)
            {
                firstImage << firstItem->text();
            }
            DLP.AddPatterns(firstImage, 1000*1000, 0, UVIntensity, 0, 0, PrintScriptList); //Printscript is set to 0 for initial exposure time, 0 for initial image
            nSlice = ui->FileList->count();
            ui->ProgramPrints->append(QString::number(nSlice) + " layers to print");
            QListWidgetItem * item;
            QStringList imageList;
            for(int i = 1; (i < (ui->FileList->count())); i++)
            {
                    item = ui->FileList->item(i);
                    imageList << item->text();
                    if ((i + InitialExposure) > MaxImageUpload)
                    {
                        break;
                    }
            }
            if (PrintScript == 1)
            {
                ui->ProgramPrints->append("Using Print Script");
            }
            DLP.AddPatterns(imageList,ExposureTime,DarkTime,UVIntensity, PrintScript, 0, PrintScriptList); //Set initial image to 0
            DLP.updateLUT();
            DLP.clearElements();
            remainingImages = MaxImageUpload - InitialExposure;

            QDir dir = QFileInfo(QFile(imageList.at(0))).absoluteDir();
            ui->ProgramPrints->append(dir.absolutePath());
            emit(on_GetPosition_clicked());
            initPlot();
            updatePlot();
            ui->StartPrint->setEnabled(true);
        }
    }
}

void MainWindow::on_AbortPrint_clicked()
{
    LCR_PatternDisplay(0);
    SMC.StopMotion();
    layerCount = 0xFFFF; //Set layer count high to stop print process
    ui->ProgramPrints->append("PRINT ABORTED");
}

void MainWindow::on_StartPrint_clicked()
{
    if (ValidateSettings() == true) //If settings are validated successfully and Initialization has been completed
    {
        SMC.SetVelocity(StageVelocity);
        Sleep(20);
        emit(on_GetPosition_clicked());
        Sleep(20);
        emit(on_GetPosition_clicked());
        //usbPollTimer->stop();
        nSlice = ui->FileList->count();
        ui->ProgramPrints->append("Entering Printing Procedure");
        //Set LED currents to 0 red, 0 green, set blue to chosen UVIntensity
        LCR_SetLedCurrents(0, 0, UVIntensity);
        //LCR_PatternDisplay(0);
        PrintStartTime = QTime::currentTime();
        DLP.startPatSequence();
        PrintProcess();
    }
}

void MainWindow::PrintProcess(void)
{
    if (layerCount +1 <= nSlice)
    {
        if (remainingImages <= 0)
        {
            LCR_PatternDisplay(0);
            QListWidgetItem * item;
            QStringList imageList;
            uint count = 0;
            int MaxImageReupload = MaxImageUpload;
            if (MaxImageUpload > 390)
            {
                MaxImageReupload -= 5;
            }
            for(int i = (layerCount); (i < ui->FileList->count()); i++)
            {
                    item = ui->FileList->item(i);
                    imageList << item->text();
                    count++;
                    if (i > layerCount + MaxImageReupload)
                    {
                        break;
                    }
            }
            if (PrintScript == 1)
            {
                ui->ProgramPrints->append("Using Print Script");
            }
            DLP.AddPatterns(imageList,ExposureTime,DarkTime,UVIntensity, PrintScript, layerCount, PrintScriptList); //Should it be layerCount + 1??
            DLP.updateLUT();
            DLP.clearElements();
            remainingImages = count - 1;
            layerCount++;
            Sleep(500);
            DLP.startPatSequence();
            QTimer::singleShot(ExposureTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
            updatePlot();
            ui->ProgramPrints->append("Reupload succesful, current layer: " + QString::number(layerCount));
            ui->ProgramPrints->append(QString::number(remainingImages + 1) + " images uploaded");
        }
        else if (InitialExposureFlag == true)
        {
            updatePlot();
            QTimer::singleShot((InitialExposure*1000), Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
            InitialExposureFlag = false;
            ui->ProgramPrints->append("Exposing Initial Layer " + QString::number(InitialExposure) + "s");
        }
        else
        {
            if (PrintScript == 1)
            {
                QTimer::singleShot(PrintScriptList.at(layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(ExposureTimeSlot())); //value from printscript will be in ms
                ui->ProgramPrints->append("Exposing: " + QString::number(PrintScriptList.at(layerCount).toDouble()) + " ms");
            }
            else
            {
                QTimer::singleShot(ExposureTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot())); //Value from exposuretime will be in us so /1000
                ui->ProgramPrints->append("Exposing: " + QString::number(ExposureTime/1000) + " ms");
            }
            ui->ProgramPrints->append("Layer: " + QString::number(layerCount));
            QString filename =ui->FileList->item(layerCount)->text();
            QPixmap img(filename);
            QPixmap img2 = img.scaled(890,490, Qt::KeepAspectRatio);
            ui->PrintImage->setPixmap(img2);
            ui->ProgramPrints->append("Exposing: " + QString::number(ExposureTime/1000) + " ms");
            ui->ProgramPrints->append("Image File: " + filename);
            layerCount++;
            remainingImages--;
            updatePlot();
            double OldPosition = GetPosition;
            emit(on_GetPosition_clicked());
            ui->ProgramPrints->append("Stage moved: " + QString::number(OldPosition - GetPosition));
        }
        return;
    }
    else
    {
        //USB_Close();
        ui->ProgramPrints->append("Print Complete");
        saveText();
        saveSettings();

        SMC.StopMotion();

        Sleep(50);
        SMC.SetVelocity(2);
        Sleep(50);
        emit(on_GetPosition_clicked());
        Sleep(50);
        if (MinEndOfRun > 0)
        {
            SMC.AbsoluteMove(MinEndOfRun);
        }
        else
        {
           SMC.AbsoluteMove(0);
        }

        return;
    }
}

void MainWindow::ExposureTimeSlot(void)
{
    QTimer::singleShot(DarkTime/1000, Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
    SMC.RelativeMove(-SliceThickness);
    ui->ProgramPrints->append("Dark Time: " + QString::number(DarkTime/1000) + " ms");
    ui->ProgramPrints->append("Moving Stage: " + QString::number(SliceThickness*1000) + " um");
}

void MainWindow::DarkTimeSlot(void)
{
    PrintProcess();
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
    else if (StageVelocity <= 0 || StageVelocity > 10)
    {
        showError("Invalid Stage Velocity");
        ui->ProgramPrints->append("Invalid Stage Velocity");
        return false;
    }
    //Validate StageAcceleration
    else if (StageAcceleration <= 0 || StageAcceleration > 10)
    {
        showError("Invalid Stage Acceleration");
        ui->ProgramPrints->append("Invalid Stage Acceleration");
        return false;
    }
    //Validate MaxEndOfRun
    else if (MaxEndOfRun < -5 || MaxEndOfRun > 65 || MinEndOfRun >= MaxEndOfRun)
    {
        showError("Invalid Max End Of Run");
        ui->ProgramPrints->append("Invalid Max End Of Run");
        return false;
    }
    //Validate StageMinEndOfRun
    else if (MinEndOfRun < -5 || MinEndOfRun > 65 || MinEndOfRun >= MaxEndOfRun)
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


/*******************************************Helper Functions********************************************/
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

void MainWindow::validateStartingPosition()
{
    QString CurrentPosition = SMC.GetPosition();
    CurrentPosition = CurrentPosition.remove(0,3);
    if (CurrentPosition.toInt() > (StartingPosition - 0.1) && CurrentPosition.toInt() < (StartingPosition +0.1))
    {
        ui->ProgramPrints->append("Stage has reached Starting Position");
        ui->StartPrint->setEnabled(true);
    }
    else
    {
        QTimer::singleShot(1000, this, SLOT(validateStartingPosition));
    }
}

void MainWindow::initStageSlot(void)
{
    emit(on_GetPosition_clicked());

    if (GetPosition < (StartingPosition-3.1))
    {
        if (StagePrep1 == false)
        {
            SMC.SetVelocity(3);
            Sleep(20);
            SMC.AbsoluteMove((StartingPosition-3));
            Sleep(20);
            QString MoveTime = SMC.GetMotionTime();
            MoveTime.remove(0,3);
            StagePrep1 = true;
            ui->ProgramPrints->append("Performing Rough Stage Movement");
        }

        QTimer::singleShot(1000, this, SLOT(initStageSlot()));
    }
    else
    {
        fineMovement();
    }
}

void MainWindow::fineMovement()
{
    emit(on_GetPosition_clicked());

    if (GetPosition > StartingPosition -0.01 && GetPosition < StartingPosition + 0.01)
    {
        verifyStageParams();
    }
    else
    {
        if (StagePrep2 == false)
        {
            Sleep(20);
            SMC.SetVelocity(0.3);
            Sleep(20);
            SMC.AbsoluteMove(StartingPosition);
            ui->ProgramPrints->append("Fine Stage Movement");
            StagePrep2 = true;
        }
        QTimer::singleShot(1000, this, SLOT(fineMovement()));
    }
}

void MainWindow::verifyStageParams()
{
    ui->ProgramPrints->append("Verifying Stage Parameters");
    Sleep(50);
    SMC.SetAcceleration(StageAcceleration);
    Sleep(50);
    SMC.SetNegativeLimit(MinEndOfRun);
    Sleep(50);
    SMC.SetPositiveLimit(MaxEndOfRun);
    Sleep(50);
    SMC.SetVelocity(StageVelocity);
}

void MainWindow::AutoMode()
{
    if (AutoModeFlag)
    {
        int SliceCount = ui->FileList->count();
        if (SliceCount > 0)
        {
            ui->ProgramPrints->append("WARNING: Auto Mode is not accurate if you have not selected all your object image files");
            int TotalPrintTime = PrintHeight / PrintSpeed;  //  um/(um/s) = s
            //ui->ProgramPrints->append("PrintHeight: " + QString::number(PrintHeight) + " PrintSpeed: " + QString::number(PrintSpeed) + "  TotalPrintTime: " + QString::number(TotalPrintTime));
            ExposureTime = (TotalPrintTime*1000) / SliceCount;
            //ui->ProgramPrints->append("Total Print Time: " + QString::number(TotalPrintTime) + " Slice Count" + QString::number(SliceCount) + "  ExposureTime: " + QString::number(ExposureTime));
            ui->ExposureTimeParam->setValue(ExposureTime);
            ui->ProgramPrints->append("Calculated Exposure Time: " + QString::number(ExposureTime) + " ms");

            SliceThickness = PrintHeight / SliceCount;
            ui->SliceThicknessParam->setValue(SliceThickness);
            ui->ProgramPrints->append("Calculated Slice Thickness: " + QString::number(SliceThickness) + " um");
        }
        else
        {
            ui->ProgramPrints->append("No Object Image Files Detected, Please Select Image Files First");
            ui->AutoCheckBox->setChecked(false);
            emit(on_AutoCheckBox_stateChanged(1));
        }
    }
}

bool MainWindow::initConfirmationScreen()
{
    bool retVal = false;
    QMessageBox confScreen;
    QPushButton *cancelButton = confScreen.addButton(QMessageBox::Cancel);
    QPushButton *okButton = confScreen.addButton(QMessageBox::Ok);
    confScreen.setStyleSheet("QLabel{min-width:300 px; font-size: 24px; text-align:center;} QPushButton{ width:150px; font-size: 18px; } QTextEdit{min-height:150px; font-size: 16px;}");
    confScreen.setText("Please Confirm Print Parameters");

    QString DetailedText;
    DetailedText += "Initial Exposure Time: " + QString::number(InitialExposure) + "s\n";
    DetailedText += "Starting Position: " + QString::number(StartingPosition) + " mm\n";
    DetailedText += "Slice Thickness: " + QString::number(SliceThickness*1000) + " μm\n";
    if (AutoModeFlag)
    {
        DetailedText += "Auto Mode Active \n";
        DetailedText += "Print Speed: " + QString::number(PrintSpeed) + " μm/s\n";
        DetailedText += "Print Height: " + QString::number(PrintHeight) + " μm\n";
    }
    else
    {
        DetailedText += "Auto Mode Not Active\n";
    }
    if (PrintScript == 1)
    {
        DetailedText += "Exposure Time Controlled by Print Script\n";
    }
    else
    {
        DetailedText += "Exposure Time: " + QString::number(ExposureTime/1000) + " ms\n";
    }
    DetailedText += "UV Intensity: " + QString::number(UVIntensity) + "\n";
    DetailedText += "Dark Time " + QString::number(DarkTime/1000) + " ms\n";

    DetailedText += "Stage Velocity: " + QString::number(StageVelocity) + " mm/s\n";
    DetailedText += "Stage Acceleration: " + QString::number(StageAcceleration) + " mm/s^2\n";
    DetailedText += "Max End Of Run: " + QString::number(MaxEndOfRun) + " mm\n";
    DetailedText += "Min End Of Run: " + QString::number(MinEndOfRun) + " mm";
    confScreen.setDetailedText(DetailedText);

    confScreen.exec();

    if (confScreen.clickedButton() == cancelButton)
    {
        ui->ProgramPrints->append("Print Parameters Not Confirmed");
        ui->ProgramPrints->append("Cancelling Print");
    }
    else if (confScreen.clickedButton() == okButton)
    {
        ui->ProgramPrints->append("Print Parameters Confirmed");
        ui->ProgramPrints->append(DetailedText);
        ui->ProgramPrints->append("Proceding to Stage Movement and Image Upload");
        retVal = true;
    }
    else
    {
        ui->ProgramPrints->append("ConfScreen Error");
    }
    return retVal;
}
/*******************************************Settings Functions*********************************************/

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

    settings.setValue("SliceThickness", SliceThickness);
    settings.setValue("StartingPosition", StartingPosition);
    settings.setValue("InitialExposure", InitialExposure);
    settings.setValue("PrintSpeed", PrintSpeed);
    settings.setValue("PrintHeight", PrintHeight);

    settings.setValue("MaxImageUpload", MaxImageUpload);

    settings.setValue("LogFileDestination", LogFileDestination);
    settings.setValue("ImageFileDirectory", ImageFileDirectory);
}

void MainWindow::loadSettings()
{
    if (loadSettingsFlag == false) //ensures that this only runs once
    {
        QSettings settings;
        ExposureTime = settings.value("ExposureTime", 1000).toDouble();
        DarkTime = settings.value("DarkTime", 1000).toDouble();
        UVIntensity = settings.value("UVIntensity", 12).toDouble();

        StageVelocity = settings.value("StageVelocity", 10).toDouble();
        StageAcceleration = settings.value("StageAcceleration", 5).toDouble();
        MaxEndOfRun = settings.value("MaxEndOfRun", 60).toDouble();
        MinEndOfRun = settings.value("MinEndOfRun", 0).toDouble();


        StartingPosition = settings.value("StartingPosition", 5).toDouble();
        InitialExposure = settings.value("InitialExposure", 10).toDouble();
        SliceThickness = settings.value("SliceThickness", 200).toDouble();
        PrintSpeed = settings.value("PrintSpeed", 40).toDouble();
        PrintHeight = settings.value("PrintHeight", 5000).toDouble();

        MaxImageUpload = settings.value("MaxImageUpload", 50).toDouble();

        LogFileDestination = settings.value("LogFileDestination", "C://").toString();
        ImageFileDirectory = settings.value("ImageFileDirectory", "C://").toString();

        loadSettingsFlag = true;
    }
}

void MainWindow::initSettings()
{
    ui->ExposureTimeParam->setValue(ExposureTime/1000);
    ui->DarkTimeParam->setValue(DarkTime/1000);
    ui->UVIntensityParam->setValue(UVIntensity);

    ui->StageVelocityParam->setValue(StageVelocity);
    ui->StageAccelParam->setValue(StageAcceleration);
    ui->MaxEndOfRun->setValue(MaxEndOfRun);
    ui->MinEndOfRunParam->setValue(MinEndOfRun);

    ui->SliceThicknessParam->setValue(SliceThickness*1000);
    ui->StartingPositionParam->setValue(StartingPosition);
    ui->InitialAdhesionParameter->setValue(InitialExposure);
    ui->PrintSpeedParam->setValue(PrintSpeed);
    ui->PrintHeightParam->setValue(PrintHeight);

    ui->MaxImageUpload->setValue(MaxImageUpload);

    ui->LogFileLocation->setText(LogFileDestination);
}

/*******************************************Plot Functions*********************************************/

void MainWindow::initPlot()
{
    ui->LivePlot->addGraph();
    ui->LivePlot->graph(0)->setName("Print Progress");
    ui->LivePlot->xAxis->setLabel("Time (s)");
    ui->LivePlot->yAxis->setLabel("Position (mm)");
    if (PrintScript ==1)
    {
        TotalPrintTimeS = 0;
        for (uint i= 0; i < PrintScriptList.size(); i++)
            {
                ui->ProgramPrints->append("Exp. Time: " + QString::number(PrintScriptList.at(i).toDouble()/1000));
                TotalPrintTimeS += PrintScriptList.at(i).toDouble()/1000;
                ui->ProgramPrints->append("Current Total :" + QString::number(TotalPrintTimeS));
            }
        TotalPrintTimeS += nSlice * (DarkTime/(1000*1000));
        TotalPrintTimeS += InitialExposure + 5;
        TotalPrintTimeS += nSlice * 0.1;
        RemainingPrintTimeS = TotalPrintTimeS;
        ui->LivePlot->xAxis->setRange(0, TotalPrintTimeS*1.1);
    }
    else
    {
        ui->LivePlot->xAxis->setRange(0, InitialExposure+5+0.1*nSlice+(1.5*(nSlice*(ExposureTime+DarkTime))/(1000*1000)));
    }
    ui->LivePlot->yAxis->setRange(0.9*(StartingPosition - nSlice*SliceThickness),1.1*StartingPosition);
    ui->LivePlot->replot();
}

void MainWindow::updatePlot()
{
    QTime updateTime = QTime::currentTime();
    int TimeElapsed = PrintStartTime.secsTo(updateTime);

    double CurrentPos = GetPosition;//StartingPosition - (layerCount*SliceThickness);

    qv_x.append(TimeElapsed);
    qv_y.append(CurrentPos);

    ui->LivePlot->graph(0)->setData(qv_x,qv_y);
    ui->LivePlot->clearItems();

    //Update Layer label
    QString Layer = " Layer: " + QString::number(layerCount) + "/" + QString::number(nSlice);
    QCPItemText *textLabel1 = new QCPItemText(ui->LivePlot);
    textLabel1->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    textLabel1->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel1->position->setCoords(0.98, 0.05); // place position at center/top of axis rect
    textLabel1->setText(Layer);
    textLabel1->setFont(QFont(font().family(), 12)); // make font a bit larger
    textLabel1->setPen(QPen(Qt::black)); // show black border around text


    //Update Remaining Time Label
    QString RemainingTime;
    if (PrintScript == 1)
    {
        if (layerCount > 0)
        {
            RemainingPrintTimeS -= PrintScriptList.at(layerCount - 1).toDouble()/1000;
        }
        RemainingTime = "Est. Remaining Time: " + QString::number(RemainingPrintTimeS) + "s";
    }
    else
    {
        RemainingTime = "Est. Remaining Time: " + QString::number(((ExposureTime+DarkTime)/(1000*1000))*(nSlice-layerCount)+InitialExposure) + "s";
    }
    QCPItemText *textLabel2 = new QCPItemText(ui->LivePlot);
    textLabel2->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    textLabel2->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel2->position->setCoords(0.98, 0.0); // place position at center/top of axis rect
    textLabel2->setText(RemainingTime);
    textLabel2->setFont(QFont(font().family(), 12)); // make font a bit larger
    textLabel2->setPen(QPen(Qt::black)); // show black border around text

    ui->LivePlot->replot(QCustomPlot::rpQueuedReplot);
}
/*************************************************************
 * ********************OUTSIDE CODE***************************
 * ***********************************************************/


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

