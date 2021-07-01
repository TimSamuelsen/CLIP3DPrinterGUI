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
#include "imagepopout.h"
#include <opencv2/opencv.hpp>

#define NormalTime
#define QuickTime
#define ON   1
#define OFF  0
//For selecting motion mode
#define CONTINUOUS  1
#define STEPPED     0
//For selecting projection mode
#define VIDEOPATTERN 1
#define POTF 0
//For selecting printer
#define CLIP30UM 0
#define ICLIP 1


DLP9000 DLP; //DLP object for calling functions from dlp9000.cpp, test if this still works without being a static
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
static int MaxImageUpload = 20;
static double InfusionRate;
static double InfusionVolume;

//Auto parameter selection mode
static double PrintSpeed;
static double PrintHeight;
static bool AutoModeFlag = false; //true when automode is on, false otherwise

//Print Process Variables
static uint nSlice = 0; //number of slices to be printed
static uint layerCount = 0; //current layer
static int remainingImages; //Keeps count of remaining images to be printed

//Plot Variables
static double TotalPrintTimeS = 0; //For calculating print times
static double RemainingPrintTimeS; //For calculating remaining pr int times

//Settings and log variables
static bool loadSettingsFlag = false; //For ensuring the settings are only loaded once, (May not be needed)
QDateTime CurrentDateTime; //Current time
QString LogFileDestination; //for storing log file destination in settings
QString ImageFileDirectory; //For storing image file directory in settings
QTime PrintStartTime; //Get start time for log
static double GetPosition; //Holds
static bool StagePrep1 = false; //For lowering stage
static bool StagePrep2 = false; //For lowering stage

//Print script parameters
QStringList ExposureScriptList; //Printscript is taken from a .csv file and stored in this variable
QStringList LEDScriptList;
QStringList DarkTimeScriptList;
QStringList InjectionVolumeScriptList;
QStringList InjectionRateScriptList;
static int PrintScript = 0; //For selecting type of printscript, currently: 0 = no printscript, 1 = exposure time print script

//Projection mode parameters
static int ProjectionMode = POTF; //For selecting projection mode, 0 = POTF mode, 1 = Video Pattern HDMI
static bool VideoLocked;
int BitLayer = 1;
int FrameCount = 0;
int ReSyncFlag = 0;
int ReSyncCount = 0;

//Bit depth selection parameters
static int BitMode = 1;

//Stepped motion parameters
static int MotionMode = 0; //Set stepped motion as default
static bool inMotion;
static double PrintEnd;

//Pumping parameter
bool PumpingMode = 0;
bool ContinuousInjection = false;
double PumpingParameter;
uint StageMode = 0; //Selects which stage to use

//Stage select parameter
static Stage_t StageType = STAGE_SMC;
static int PrinterType = CLIP30UM;

/**
 * @brief MainWindow::MainWindow
 * @param parent
 * Creates the mainwindow, gets current time for print log,
 * loads and initializes saved settings and initializes plot
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    //Create new window
    ui = (new Ui::MainWindow);
    ui->setupUi(this);

    //Initialize features
    CurrentDateTime = QDateTime::currentDateTime(); //get current time for startup time
    loadSettings(); //load settings from settings file
    initSettings(); //initialize settings by updating ui
    initPlot(); //initiallize the plot window
}

/**
 * @brief MainWindow::timerTimeout
 * Currently not in use, to be used for ui refresh rate
 */
void MainWindow::timerTimeout(void)
{
    //emit(on_GetPosition_clicked());
}

/**
 * @brief MainWindow::~MainWindow
 * Called when the main window is closed
 */
MainWindow::~MainWindow()
{
    saveSettings(); //Save settings upon closing application main window
    //SMC.SMC100CClose();
    delete ui;
}

/**
 * @brief MainWindow::on_ManualStage_clicked
 * Open the manual stage control window, sends home
 * command to stage and closes mainwindow connection
 */
void MainWindow::on_ManualStage_clicked()
{
    //Create new manual stage control window
    ManualStageUI = new ManualStageControl();
    ManualStageUI->show();

    //Initialize stage for manual stage control
    //SMC.Home(); //Home command is issued to make sure the stage is referenced upon startup
    Stage.StageHome(StageType);
    //SMC.SMC100CClose(); //Close main window connection to allow manual stage control to take over, ideally this would not be needed and it would stay on the same connection.
    Stage.StageClose(StageType);

    //Update Terminal and Stage Connection Indicator
    ui->ProgramPrints->append("Manual Stage Control Entered"); //Print to terminal
    ui->StageConnectionIndicator->setStyleSheet("background:rgb(0, 255, 255); border: 1px solid black;"); //Set color of indicator to teal to indicate in manual stage control
    ui->StageConnectionIndicator->setText("Manual Control"); //Set text on indicator to indicate in manual stage control
}

/**
 * @brief MainWindow::on_pushButton_clicked
 * Opens the Manual Pump Control window and closes the mainwindow
 * pump connection
 */
void MainWindow::on_pushButton_clicked()
{
    //Create new manual pump control window
    ManualPumpUI = new manualpumpcontrol();
    ManualPumpUI->show();

    ui->ProgramPrints->append("Manual Pump Control Entered"); //Print to terminal
    Pump.PSerial.closeDevice(); //Closes main window pump connection to allow manual pump control to take over
}

/**
 * @brief MainWindow::on_ImageProcess_clicked
 * Opens the image processing window
 */
void MainWindow::on_ImageProcess_clicked()
{
    //Create new image processing window
    ImageProcessUI = new imageprocessing();
    ImageProcessUI->show();
    ui->ProgramPrints->append("Opening Image Processing");
}

/**
 * @brief MainWindow::on_GetPosition_clicked
 * Gets position, saves it in module level variable GetPosition,
 * updates slider and prints to terminal window
 */
void MainWindow::on_GetPosition_clicked()
{
    //Get position and store it in char*, could store directly in QString but if result is null it can cause issues
    //char* ReadPosition = SMC.GetPosition();
    char* ReadPosition = Stage.StageGetPosition(StageType);

    //If the string length returned from SMC.GetPosition is > 1, then no errors have occurred
    if (strlen(ReadPosition) > 1)
    {
        QString CurrentPosition = QString::fromUtf8(ReadPosition); //convert char* to QString
        CurrentPosition.remove(0,3); //Removes address and command code
        CurrentPosition.chop(2); //To be removed...
        ui->CurrentPositionIndicator->setText(CurrentPosition); //Update current position indicator
        ui->ProgramPrints->append("Stage is currently at: " + CurrentPosition + " mm"); //Print to terminal
        ui->CurrentStagePos->setSliderPosition(CurrentPosition.toDouble()); //Update slider position
        GetPosition = CurrentPosition.toDouble(); //Update module variable GetPosition (to be accessed from other functions)
    }
}

/*********************************************Mode Selection*********************************************/
/**
 * @brief MainWindow::on_POTFcheckbox_clicked
 * Sets projection mode to POTF
 */
void MainWindow::on_POTFcheckbox_clicked()
{
    //Make sure that the checkbox is checked before proceeding
    if (ui->POTFcheckbox->isChecked())
    {
        ui->VP_HDMIcheckbox->setChecked(false); //Uncheck the Video Pattern checkbox
        ProjectionMode = POTF; //Set projection mode to POTF
        DLP.setIT6535Mode(0); //Turn off HDMI connection
        LCR_SetMode(PTN_MODE_OTF); //Set light engine to POTF mode
    }
}

/**
 * @brief MainWindow::on_VP_HDMIcheckbox_clicked
 * Sets projection mode to Video Pattern
 */
void MainWindow::on_VP_HDMIcheckbox_clicked()
{
    //Make sure that the checkbox is checked before proceeding
    if (ui->VP_HDMIcheckbox->isChecked())
    {
        ui->POTFcheckbox->setChecked(false); //Uncheck the Video Pattern checkbox
        initImagePopout(); //Open projection window
        ProjectionMode = VIDEOPATTERN; //Set projection mode to video pattern

        //Set video display off
        if (LCR_SetMode(PTN_MODE_DISABLE) < 0)
        {
            showError("Unable to switch to video mode");
            ui->VP_HDMIcheckbox->setChecked(false);
            ui->POTFcheckbox->setChecked(true);
            emit(on_POTFcheckbox_clicked());
            return;
        }
        DLP.setIT6535Mode(1); //Set IT6535 reciever to HDMI input
        //API_VideoConnector_t hdmi = VIDEO_CON_HDMI;
        //LCR_SetIT6535PowerMode(hdmi);
        Check4VideoLock();

    }
}

/**
 * @brief MainWindow::Check4VideoLock
 * Validates that the video source is locked, this is needed for Video Pattern Mode
 */
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
            if (LCR_SetMode(PTN_MODE_VIDEO) < 0)
            {
                showError("Unable to switch to video pattern mode");
                ui->VP_HDMIcheckbox->setChecked(false);
                ui->POTFcheckbox->setChecked(true);
                emit(on_POTFcheckbox_clicked());
            }
            ui->ProgramPrints->append("Video Pattern Mode Enabled");
            return;
        }
        else{
            ui->ProgramPrints->append("External Video Source Not Locked, Please Wait");
            //API_VideoConnector_t testVC;
            //LCR_GetIT6535PowerMode(&testVC);
            API_DisplayMode_t testDM;
            LCR_GetMode(&testDM);
            ui->ProgramPrints->append(QString::number(testDM));

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

/**
 * @brief MainWindow::initImagePopout
 * Initializes the image popout window that is used for video pattern mode
 */
void MainWindow::initImagePopout()
{
    ImagePopoutUI = new imagepopout;
    ImagePopoutUI->show();
    ImagePopoutUI->windowHandle()->setScreen(qApp->screens()[1]);
    ImagePopoutUI->showFullScreen();
    QStringList imageList;
    QListWidgetItem * item;
    for(int i = 0; (i < (ui->FileList->count())); i++)
    {
            item = ui->FileList->item(i);
            imageList << item->text();
    }
}

/**
 * @brief MainWindow::on_DICLIPSelect_clicked
 * Currently just a dummy fuction used for testing
 */
void MainWindow::on_DICLIPSelect_clicked()
{
    //initImagePopout();
    //ProjectionMode = 1;
    StageType = STAGE_GCODE;
    PrinterType = ICLIP;

    ui->VolPerLayerParam->setEnabled(true);
    ui->SetVolPerLayer->setEnabled(true);
    ui->InfuseRateParam->setEnabled(true);
    ui->SetInfuseRate->setEnabled(true);

    ui->StartingPositionParam->setEnabled(false);
    ui->SetStartingPosButton->setEnabled(false);

    ui->StageAccelParam->setEnabled(false);
    ui->SetStageAcceleration->setEnabled(false);
    ui->MaxEndOfRun->setEnabled(false);
    ui->SetMaxEndOfRun->setEnabled(false);
    ui->MinEndOfRunParam->setEnabled(false);
    ui->SetMinEndOfRun->setEnabled(false);
}

void MainWindow::on_CLIPSelect_clicked()
{
    StageType = STAGE_SMC;
    PrinterType = CLIP30UM;

    ui->VolPerLayerParam->setEnabled(false);
    ui->SetVolPerLayer->setEnabled(false);
    ui->InfuseRateParam->setEnabled(false);
    ui->SetInfuseRate->setEnabled(false);

    ui->StartingPositionParam->setEnabled(true);
    ui->SetStartingPosButton->setEnabled(true);

    ui->StageAccelParam->setEnabled(true);
    ui->SetStageAcceleration->setEnabled(true);
    ui->MaxEndOfRun->setEnabled(true);
    ui->SetMaxEndOfRun->setEnabled(true);
    ui->MinEndOfRunParam->setEnabled(true);
    ui->SetMinEndOfRun->setEnabled(true);
}


/**
 * @brief MainWindow::on_SetBitDepth_clicked
 * Sets the module variable BitMode which is used to determine the bit depth
 * the user wishes to print with
 */
void MainWindow::on_SetBitDepth_clicked()
{
    BitMode = ui->BitDepthParam->value();
    ui->ProgramPrints->append("Bit-Depth set to: " + QString::number(BitMode));
}

/**
 * @brief MainWindow::on_SteppedMotion_clicked
 * Sets motion mode to stepped, this is the default
 */
void MainWindow::on_SteppedMotion_clicked()
{
    if(ui->SteppedMotion->isChecked() == true)
    {
        MotionMode = 0;
        ui->ContinuousMotion->setChecked(false);
        ui->ProgramPrints->append("Stepped Motion Selected");
        ui->SetDarkTime->setEnabled(true);
    }
}

/**
 * @brief MainWindow::on_ContinuousMotion_clicked
 * Sets motion mode to continuous
 */
void MainWindow::on_ContinuousMotion_clicked()
{
    if(ui->ContinuousMotion->isChecked() == true)
    {
        MotionMode = 1;
        ui->SteppedMotion->setChecked(false);
        ui->ProgramPrints->append("Continuous Motion Selected");
        DarkTime = 0;
        ui->DarkTimeParam->setValue(0);
        ui->SetDarkTime->setEnabled(false);
    }
}

/**
 * @brief MainWindow::on_pumpingCheckBox_clicked
 * Enables pumping mode in which the stage movement is exaggerated
 * to avoid the part sticking to the window
 */
void MainWindow::on_pumpingCheckBox_clicked()
{
    if(ui->pumpingCheckBox->isChecked() == true){
        PumpingMode = 1;
        ui->pumpingParameter->setEnabled(true);
        ui->setPumping->setEnabled(true);
        ui->ProgramPrints->append("Pumping Enabled");
    }
    else{
        PumpingMode = 0;
        ui->pumpingParameter->setEnabled(false);
        ui->setPumping->setEnabled(false);
        ui->ProgramPrints->append("Pumping Disabled");
    }
}

/**
 * @brief MainWindow::on_setPumping_clicked
 * Sets the pumping depth parameter
 */
void MainWindow::on_setPumping_clicked()
{
    if(PumpingMode == 1){
        PumpingParameter = ui->pumpingParameter->value()/1000;
        ui->ProgramPrints->append("Pumping depth set to: " + QString::number(PumpingParameter*1000) + " μm");
    }
    else{
        ui->ProgramPrints->append("Please enable pumping before setting pumping parameter");
    }
}
/***************************************Print Functionality*********************************************/
/**
 * @brief MainWindow::on_InitializeAndSynchronize_clicked
 * Prepares the system for printing, moves stage to starting position
 * inits plot, uploads images to the light engine
 */
void MainWindow::on_InitializeAndSynchronize_clicked()
{
    if (initConfirmationScreen())
    {
        LCR_PatternDisplay(0); //Turn off any projection
        initStageSlot(); //Initializes stage with correct parameters and moves controlled to starting position

        if (ui->FileList->count() > 0)
        {
            //Upload images for initial exposure
            QListWidgetItem * firstItem = ui->FileList->item(0); //Get first image
            QStringList firstImage;
            //Create an imagelist that contain n copies of first image, where n = InitialExposure in integer seconds
            for (uint i = 0; i < InitialExposure; i++)
            {
                firstImage << firstItem->text();
            }
            DLP.AddPatterns(firstImage, 1000*1000, 0, 0, 0, ExposureScriptList, DarkTimeScriptList,ProjectionMode, BitMode, InitialExposureFlag); //Printscript is set to 0 for initial exposure time, 0 for initial image
            if(ProjectionMode == POTF){
                nSlice = ui->FileList->count();
            }
            else if (ProjectionMode == VIDEOPATTERN){
                nSlice = (24/BitMode)*ui->FileList->count();
                DLP.updateLUT(ProjectionMode);
                DLP.clearElements();
            }
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
            if (ProjectionMode == POTF){
                DLP.AddPatterns(imageList,ExposureTime,DarkTime, PrintScript, 0, ExposureScriptList, DarkTimeScriptList, ProjectionMode, BitMode, 0); //Set initial image to 0
                DLP.updateLUT(ProjectionMode);
                DLP.clearElements();
                remainingImages = MaxImageUpload - InitialExposure;
            }
            QDir dir = QFileInfo(QFile(imageList.at(0))).absoluteDir();
            ui->ProgramPrints->append(dir.absolutePath());
            emit(on_GetPosition_clicked());
            initPlot();
            updatePlot();
            ui->StartPrint->setEnabled(true);
            if(MotionMode == 1){
                if(PrinterType == CLIP30UM){
                    PrintEnd = StartingPosition - (ui->FileList->count()*SliceThickness);
                }
                else if (PrinterType == ICLIP){
                    PrintEnd = ui->FileList->count()*SliceThickness; //Make sure stage is zeroed
                }
                else{
                    showError("PrinterType Error, on_InitializeAndSynchronize_clicked");
                }
                ui->ProgramPrints->append("Print End for continous motion print set to: " + QString::number(PrintEnd));
            }
            if(ProjectionMode == VIDEOPATTERN){
                QString filename =ui->FileList->item(layerCount)->text();
                QPixmap img(filename);
                ImagePopoutUI->showImage(img);
            }
        }
    }
}

/**
 * @brief MainWindow::on_AbortPrint_clicked
 * Aborts print and acts as e-stop. Stops light engine projection,
 * stage movement and print process
 */
void MainWindow::on_AbortPrint_clicked()
{
    LCR_PatternDisplay(0); //Turn off light engine projection
    //SMC.StopMotion(); //Stop stage movement
    Stage.StageStop(StageType);
    layerCount = 0xFFFFFF; //Set layer count high to stop print process
    ui->ProgramPrints->append("PRINT ABORTED");
}

/**
 * @brief MainWindow::on_StartPrint_clicked
 * Starts print process, set LED current calculates speed for continuous motion mode
 */
void MainWindow::on_StartPrint_clicked()
{
    if (ValidateSettings() == true) //If settings are validated successfully and Initialization has been completed
    {
        //SMC.SetVelocity(StageVelocity); //Update stage velocity
        Stage.SetStageVelocity(StageVelocity, StageType);
        Sleep(20); //Sleep to avoid spamming motor controller
        emit(on_GetPosition_clicked()); //Get stage position to validate that stage connection is working
        Sleep(20);
        emit(on_GetPosition_clicked());
        if(ProjectionMode == POTF){
            nSlice = ui->FileList->count();  //# of images selected = # of slices
        }
        else if (ProjectionMode == VIDEOPATTERN){
            nSlice = (24/BitMode)*ui->FileList->count();
        }

        ui->ProgramPrints->append("Entering Printing Procedure");
        //Set LED currents to 0 red, 0 green, set blue to chosen UVIntensity
        if (PrintScript == ON){ //If printscript is on
            LCR_SetLedCurrents(0,0,(LEDScriptList.at(0).toInt())); //Set LED intensity to first value in printscript
        }
        else{ //if printscript is off
            LCR_SetLedCurrents(0, 0, UVIntensity); //set static LED intensity
        }
        if (MotionMode == CONTINUOUS){ //continuous motion mode
            double ContStageVelocity = (SliceThickness)/(ExposureTime/(1e6)); //Multiply Exposure time by 1e6 to convert from us to s to get to proper units
            ui->ProgramPrints->append("Continuous Stage Velocity set to " + QString::number(SliceThickness) + "/" + QString::number(ExposureTime) + " = " + QString::number(ContStageVelocity) + " mm/s");
            //SMC.SetVelocity(ContStageVelocity); //Set stage velocity to the calculated velocity
            Stage.SetStageVelocity(ContStageVelocity, StageType);
        }
        PrintStartTime = QTime::currentTime(); //Get print start time from current time
        DLP.startPatSequence(); //Start projection
        if (ProjectionMode == POTF){ //if in POTF mode
            PrintProcess(); //Start print process
            ui->ProgramPrints->append("Entering POTF print process");
        }
        else{ //else in Video Pattern Mode
            DLP.clearElements();
            PrintProcessVP();
            ui->ProgramPrints->append("Entering Video Pattern print process");
        }
        if (ContinuousInjection == ON){
            Pump.StartInfusion();
        }
    }
}

/**
 * @brief MainWindow::PrintProcess
 * Print process, for detailed description see documentation
 */
void MainWindow::PrintProcess(void)
{
    if (layerCount +1 <= nSlice)
    {
        if (remainingImages <= 0)
        {
            if (MotionMode == 1){
                //SMC.StopMotion();
                Stage.StageStop(StageType);
            }
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
            if (PrintScript == ON)
            {
                ui->ProgramPrints->append("Using Print Script");
            }
            DLP.AddPatterns(imageList,ExposureTime,DarkTime, PrintScript, layerCount, ExposureScriptList, DarkTimeScriptList, ProjectionMode, BitMode, 0);
            DLP.updateLUT(ProjectionMode);
            DLP.clearElements();
            remainingImages = count - 1;
            layerCount++;
            Sleep(50);
            DLP.startPatSequence();
            SetExposureTimer(0, PrintScript, PumpingMode);
            if(MotionMode == 1){
                //SMC.AbsoluteMove(PrintEnd);
                Stage.StageAbsoluteMove(PrintEnd, StageType);
            }
            else{
                //updatePlot();
            }
            ui->ProgramPrints->append("Reupload succesful, current layer: " + QString::number(layerCount));
            ui->ProgramPrints->append(QString::number(remainingImages + 1) + " images uploaded");
        }
        else if (InitialExposureFlag == true)
        {
            SetExposureTimer(InitialExposureFlag, PrintScript, PumpingMode);
            updatePlot();
            InitialExposureFlag = false;
            ui->ProgramPrints->append("POTF Exposing Initial Layer " + QString::number(InitialExposure) + "s");
            inMotion = false;
        }
        else
        {
            SetExposureTimer(0, PrintScript, PumpingMode);
            ui->ProgramPrints->append("Layer: " + QString::number(layerCount));
            QString filename =ui->FileList->item(layerCount)->text();
            ui->ProgramPrints->append("POTF Exposing: " + QString::number(ExposureTime/1000) + " ms");
            ui->ProgramPrints->append("Image File: " + filename);
            layerCount++;
            remainingImages--;
            if(MotionMode == STEPPED){
                updatePlot();
            }
            else if (MotionMode == CONTINUOUS){
                if(PrinterType == ICLIP){
                    PrintInfuse();
                    ui->ProgramPrints->append("Injecting " + QString::number(InfusionVolume) + "ul at " + QString::number(InfusionRate) + "ul/s");
                }
            }
            double OldPosition = GetPosition;
            emit(on_GetPosition_clicked());
            ui->ProgramPrints->append("Stage moved: " + QString::number(OldPosition - GetPosition));
            if (ProjectionMode == 1)
            {
                QPixmap img(filename);
                ImagePopoutUI->showImage(img);
            }
        }
        return;
    }
    else
    {
        ui->ProgramPrints->append("Print Complete");
        saveText();
        saveSettings();

        //SMC.StopMotion();
        Stage.StageStop(StageType);
        LCR_PatternDisplay(0);
        Sleep(50);
        //SMC.SetVelocity(2);
        Stage.SetStageVelocity(2, StageType);
        Sleep(50);
        emit(on_GetPosition_clicked());
        Sleep(50);
        if (MinEndOfRun > 0)
        {
            Stage.StageAbsoluteMove(MinEndOfRun, StageType);
            //SMC.AbsoluteMove(MinEndOfRun);
        }
        else
        {
           Stage.StageAbsoluteMove(0, StageType);
           //SMC.AbsoluteMove(0);
        }

        return;
    }
}

/**
 * @brief MainWindow::PrintProcessVP
 * Separate print process for Video Pattern mode
 */
void MainWindow::PrintProcessVP()
{
    if (layerCount +1 <= nSlice)
    {
        if (ReSyncFlag == 1) //if resync
        {
            ReSyncFlag = 0;
            if (LCR_PatternDisplay(0) < 0)
                showError("Unable to stop pattern display");
            Sleep(10);
            QListWidgetItem * item;
            QStringList imageList;
            uint count = 0;
            //if (BitMode == 8){
              //  uploadCount = 16;
            //}
            //else{
             //   uploadCount = 5;
            //}
            for(int i = FrameCount; i < FrameCount + (5*BitMode); i++)
            {
                for (int j = 0; j < (24/BitMode); j++)
                {
                    if (i < ui->FileList->count()){
                        item = ui->FileList->item(i);
                        imageList << item->text();
                        count++;
                    }
                        else{
                            ui->ProgramPrints->append("VP Image segmentation fault");
                            break;
                        }
                }
                ui->ProgramPrints->append(QString::number(count) + " patterns uploaded");
            }
            //Add pattern data to buffer to prepare for pattern upload
            DLP.AddPatterns(imageList,ExposureTime,DarkTime, PrintScript, layerCount, ExposureScriptList, DarkTimeScriptList, ProjectionMode, BitMode, 0);
            DLP.updateLUT(ProjectionMode); //update LUT on light engine to upload pattern data
            DLP.clearElements(); //clear pattern data buffer
            Sleep(50); //small delay to ensure that the new patterns are uploaded
            ui->ProgramPrints->append("Resync succesful, frame: " + QString::number(FrameCount) + " layer: " + QString::number(layerCount) + "New Patterns: " + QString::number(count));
            DLP.startPatSequence();
            Sleep(5);
            SetExposureTimer(0, PrintScript, PumpingMode);
            layerCount++; //increment layer counter
            remainingImages--; //decrement remaining images
            BitLayer += BitMode;
        }
        else if (InitialExposureFlag == true) //Initial Exposure
        {
            SetExposureTimer(InitialExposureFlag, PrintScript, PumpingMode);
            BitLayer = 1; //set to first bitlayer
            ReSyncFlag = 1; //resync after intial exposure
            ui->ProgramPrints->append("VP Exposing Initial Layer " + QString::number(InitialExposure) + "s");
            InitialExposureFlag = 0;
            //layerCount++;
        }
        else //Normal print process
        {
            //Start exposuretime timers
            SetExposureTimer(0, PrintScript, PumpingMode);
            //Print information to log
            ui->ProgramPrints->append("VP Layer: " + QString::number(layerCount));
            QString filename =ui->FileList->item(FrameCount)->text();
            //ui->ProgramPrints->append("VP Exposing: " + QString::number(ExposureTime/1000) + " ms");
            ui->ProgramPrints->append("Image File: " + filename);

            layerCount++; //increment layer counter
            remainingImages--; //decrement remaining images

            //updatePlot();
            BitLayer += BitMode;
        }
    }
    else //print complete
    {
        //USB_Close();
        ui->ProgramPrints->append("Print Complete");
        saveText();
        saveSettings();
        Stage.StageStop(StageType);
        //SMC.StopMotion();
        Sleep(50);
        Stage.SetStageVelocity(2, StageType);
        //SMC.SetVelocity(2);
        Sleep(50);
        emit(on_GetPosition_clicked());
        Sleep(50);
        if (PrinterType == CLIP30UM){
            if (MinEndOfRun > 0){
                //SMC.AbsoluteMove(MinEndOfRun);
                Stage.StageAbsoluteMove(MinEndOfRun, StageType);
            }
            else{
               //SMC.AbsoluteMove(0);
                Stage.StageAbsoluteMove(0, StageType);
            }
        }
        else if(PrinterType == ICLIP){
            Pump.Stop();
        }
        return;
    }
}

/**
 * @brief MainWindow::pumpingSlot
 * Pumping slot for when pumping is activated,
 * intermediary step between exposure time and dark time
 */
void MainWindow::pumpingSlot(void)
{
    QTimer::singleShot(DarkTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
    //SMC.RelativeMove(-PumpingParameter); //Move the stage back the desired pumping distance
    Stage.StageRelativeMove(-PumpingParameter, StageType);
    ui->ProgramPrints->append("Pumping " + QString::number(PumpingParameter*1000) +" um");
}

/**
 * @brief MainWindow::ExposureTimeSlot
 * Handles all dark time actions required
 */
void MainWindow::ExposureTimeSlot(void)
{
    if (MotionMode == 0){
        SetDarkTimer(PrintScript);
        ui->ProgramPrints->append(QTime::currentTime().toString("hh.mm.ss.zzz"));
    }

    updatePlot();

    if (ProjectionMode == VIDEOPATTERN) //If in video pattern mode
    {
        //Add if statement here if last exposure time
        if (PrintScript == ON && layerCount > 0)
        {
            if((ExposureScriptList.at(layerCount).toInt()) != ExposureScriptList.at(layerCount - 1).toInt())
            {

            }
        }
        if (BitLayer > 24) //If end of frame has been reached
        {
            FrameCount++; //increment frame counter
            ui->ProgramPrints->append("New Frame: " + QString::number(FrameCount));
            if (FrameCount < ui->FileList->count()){
                QPixmap img(ui->FileList->item(FrameCount)->text()); //select next image
                ImagePopoutUI->showImage(img); //display next image
            }
            BitLayer = 1;
            ReSyncCount++;
            if (ReSyncCount > (120-24)/(24/BitMode)){
                ReSyncFlag = 1;
                ReSyncCount = 0;
            }
        }
        emit(on_GetPosition_clicked());
    }
    if (PrinterType == ICLIP)
    {
        if (PrintScript == ON)
        {
            if(layerCount < InjectionRateScriptList.size() && layerCount < InjectionVolumeScriptList.size())
            {
                if (layerCount > 0){
                    if ((InjectionRateScriptList.at(layerCount).toInt()) == InjectionRateScriptList.at(layerCount-1).toInt()){
                        //do nothing, this avoids spamming the light engine when LED intensity is constant
                    }
                    else{
                         Pump.SetInfuseRate(InjectionRateScriptList.at(layerCount).toDouble());
                         ui->LiveValue5->setText(QString::number(InjectionRateScriptList.at(layerCount).toDouble()));
                         Sleep(10);
                    }
                    if ((InjectionVolumeScriptList.at(layerCount).toInt()) == InjectionVolumeScriptList.at(layerCount-1).toInt()){
                        //do nothing, this avoids spamming the light engine when LED intensity is constant

                    }
                    else{
                         Pump.SetTargetVolume(InjectionVolumeScriptList.at(layerCount).toDouble());
                         ui->LiveValue4->setText(QString::number(InjectionVolumeScriptList.at(layerCount).toDouble()));
                    }
                }
                else{
                    Pump.SetInfuseRate(InjectionRateScriptList.at(layerCount).toDouble());
                    Sleep(10);
                    Pump.SetTargetVolume(InjectionVolumeScriptList.at(layerCount).toDouble());
                }
            }
        }
        PrintInfuse();
        ui->ProgramPrints->append("Injecting " + QString::number(InfusionVolume) + "ul at " + QString::number(InfusionRate) + "ul/s");
    }

    if(MotionMode == 0){
        if(PumpingMode == 1){
            emit(on_GetPosition_clicked());
            updatePlot();
            Stage.StageRelativeMove(PumpingParameter - SliceThickness, StageType);
            //SMC.RelativeMove((PumpingParameter - SliceThickness)); //pumping param is in um, slicethickness is in mm
        }
        else{
            //SMC.RelativeMove(-SliceThickness);
            Stage.StageRelativeMove(-SliceThickness, StageType);
        }

        if(PrintScript == 1)
        {
            if (layerCount < LEDScriptList.size()){
                if (layerCount > 0)
                {
                    if ((LEDScriptList.at(layerCount).toInt()) == LEDScriptList.at(layerCount-1).toInt()){
                        //do nothing, this avoids spamming the light engine when LED intensity is constant
                    }
                    else{
                         LCR_SetLedCurrents(0, 0, (LEDScriptList.at(layerCount).toInt()));
                         ui->LiveValue2->setText(QString::number(LEDScriptList.at(layerCount).toInt()));
                    }
                }
                else
                {
                    LCR_SetLedCurrents(0, 0, (LEDScriptList.at(layerCount).toInt()));
                }
                ui->ProgramPrints->append("LED Intensity: " + LEDScriptList.at(layerCount));
            }
            else{
                ui->ProgramPrints->append(QString::number(layerCount) + QString::number(sizeof(LEDScriptList)));
            }
        }
        if (PrintScript == ON){
            if(layerCount < DarkTimeScriptList.size()){
                ui->ProgramPrints->append("Dark Time: " + QString::number(DarkTimeScriptList.at(layerCount).toDouble()));
                ui->LiveValue1->setText(QString::number(ExposureScriptList.at(layerCount).toInt()));
                ui->LiveValue3->setText(QString::number(DarkTimeScriptList.at(layerCount).toInt()));
            }
        }
        else{
            ui->ProgramPrints->append("Dark Time: " + QString::number(DarkTime/1000) + " ms");
        }
        ui->ProgramPrints->append("Moving Stage: " + QString::number(SliceThickness*1000) + " um");
    }
    else if(MotionMode == 1){
        if (inMotion == false){
            //SMC.AbsoluteMove(PrintEnd);
            Stage.StageAbsoluteMove(PrintEnd, StageType);
        }
        PrintProcess();
    }
}

/**
 * @brief MainWindow::DarkTimeSlot
 * Dummy slot, may be removed
 */
void MainWindow::DarkTimeSlot(void)
{
    if(ProjectionMode == POTF){
        PrintProcess();
    }
    else if(ProjectionMode == VIDEOPATTERN){
        PrintProcessVP();
    }
    else{
        showError("Incorrect Projection Mode");
    }
}

void MainWindow::SetExposureTimer(int InitialExposureFlag, int PrintScript, int PumpingMode)
{
    if (InitialExposureFlag == 1)
    {
        if (PumpingMode == 1){
            QTimer::singleShot(InitialExposure*1000, Qt::PreciseTimer, this, SLOT(pumpingSlot()));
        }
        else{
            QTimer::singleShot(InitialExposure*1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
        }
    }
    else
    {
        if (PrintScript == 1){
            if (PumpingMode == 1){
                QTimer::singleShot(ExposureScriptList.at(layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(pumpingSlot()));
            }
            else{
                QTimer::singleShot(ExposureScriptList.at(layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
            }
            ui->ProgramPrints->append("Exposing: " + QString::number(ExposureScriptList.at(layerCount).toDouble()) + " ms");
        }
        else{
            if (PumpingMode == 1){
                QTimer::singleShot(ExposureTime/1000, Qt::PreciseTimer, this, SLOT(pumpingSlot()));
            }
            else{
                QTimer::singleShot(ExposureTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
            }
            ui->ProgramPrints->append("Exposing: " + QString::number(ExposureTime/1000) + " ms");
        }
    }
}

void MainWindow::SetDarkTimer(int PrintScript)
{
    if (PrintScript == ON){
        if(layerCount < DarkTimeScriptList.size()){
            QTimer::singleShot(DarkTimeScriptList.at(layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
        }
    }
    else if(PrintScript == OFF){
        QTimer::singleShot(DarkTime/1000, Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
    }
    else{
        showError("Dark time settings error, SetDarkTime");
    }
}

void MainWindow::PrintInfuse()
{
    if (ContinuousInjection == OFF){
        Pump.ClearVolume();
        Sleep(15);
        Pump.StartInfusion();
    }
}
/*********************************************File Handling*********************************************/
/**
 * @brief MainWindow::on_SelectFile_clicked
 * Select all object image files to project
 */
void MainWindow::on_SelectFile_clicked()
{
    //Open files from last directory chosen (stored in settings), limited to bitmapped image file formats
    QStringList file_name = QFileDialog::getOpenFileNames(this,"Open Object Image Files",ImageFileDirectory,"*.bmp *.png *.tiff *.tif");
    if (file_name.count() > 0) //If images were selected
    {
        QDir ImageDirectory = QFileInfo(file_name.at(0)).absoluteDir();
        ImageFileDirectory = ImageDirectory.filePath(file_name.at(0));
        for (uint16_t i = 0; i < file_name.count(); i++)
        {
            ui->FileList->addItem(file_name.at(i)); //Add each file name to FileList, will be displayed for user
        }
    }
    else
    {
        ui->ProgramPrints->append("Please select more images");
    }
    int SliceCount = ui->FileList->count();
    ui->ProgramPrints->append(QString::number(SliceCount) + " Images Currently Selected");
}

/**
 * @brief MainWindow::on_LogFileBrowse_clicked
 * Select directory to store log files
 */
void MainWindow::on_LogFileBrowse_clicked()
{
    LogFileDestination = QFileDialog::getExistingDirectory(this, "Open Log File Destination");
    ui->LogFileLocation->setText(LogFileDestination);
}

/**
 * @brief MainWindow::on_ClearImageFiles_clicked
 * Clear image files selected
 */
void MainWindow::on_ClearImageFiles_clicked()
{
    ui->FileList->clear();
    //initConfirmationScreen();
}

/**
 * @brief MainWindow::on_UsePrintScript_clicked
 * Used to select whether to use set print variables or print script
 */
void MainWindow::on_UsePrintScript_clicked()
{
    if (ui->UsePrintScript->checkState()) //If printscript is checked
    {
        PrintScript = 1;
        ui->SelectPrintScript->setEnabled(true);
        ui->ClearPrintScript->setEnabled(true);
        ui->PrintScriptFile->setEnabled(true);

        ui->ExposureTimeParam->setEnabled(false);
        ui->SetExposureTime->setEnabled(false);
        ui->UVIntensityParam->setEnabled(false);
        ui->SetUVIntensity->setEnabled(false);
        ui->DarkTimeParam->setEnabled(false);
        ui->SetDarkTime->setEnabled(false);
    }
    else //printscript is not checked
    {
        PrintScript = 0;
        ui->SelectPrintScript->setEnabled(false);
        ui->ClearPrintScript->setEnabled(false);
        ui->PrintScriptFile->setEnabled(false);

        ui->ExposureTimeParam->setEnabled(true);
        ui->SetExposureTime->setEnabled(true);
        ui->UVIntensityParam->setEnabled(true);
        ui->SetUVIntensity->setEnabled(true);
        ui->DarkTimeParam->setEnabled(true);
        ui->SetDarkTime->setEnabled(true);
    }
}

/**
 * @brief MainWindow::on_SelectPrintScript_clicked
 * User selects print script from file, the print script is parsed
 * and the exposure time and LED intensity is stored in
 * module level QStringLists: ExposureScriptList and LEDScriptList
 */
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
            ExposureScriptList.append(line.split(',').at(0));
            LEDScriptList.append(line.split(',').at(1));
            DarkTimeScriptList.append(line.split(',').at(2));
            if(PrinterType == ICLIP){
                InjectionVolumeScriptList.append(line.split(',').at(3));
                InjectionRateScriptList.append(line.split(',').at(4));
            }
    }
     //For testing
    for (int i= 0; i < ExposureScriptList.size(); i++)
    {
        if(PrinterType == ICLIP){
            ui->ProgramPrints->append(ExposureScriptList.at(i) + "," + LEDScriptList.at(i) + "," + DarkTimeScriptList.at(i) + "," + InjectionVolumeScriptList.at(i) + "," + InjectionRateScriptList.at(i));
        }
        else{
            ui->ProgramPrints->append(ExposureScriptList.at(i) + "," + LEDScriptList.at(i) + "," + DarkTimeScriptList.at(i));
        }
    }
    ui->ProgramPrints->append("Print List has: " + QString::number(ExposureScriptList.size()) + " exposure time entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(LEDScriptList.size()) + " LED intensity entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(DarkTimeScriptList.size()) + " dark time entries");
    if (ExposureScriptList.size() > 0)
    {
        ui->LiveValueList1->setCurrentIndex(1);
        ui->LiveValue1->setText(QString::number(ExposureScriptList.at(0).toInt()));
        ui->LiveValueList2->setCurrentIndex(2);
        ui->LiveValue2->setText(QString::number(LEDScriptList.at(0).toInt()));
        ui->LiveValueList3->setCurrentIndex(3);
        ui->LiveValue3->setText(QString::number(DarkTimeScriptList.at(0).toInt()));
        if(PrinterType == ICLIP){
            if (InjectionVolumeScriptList.size() > 0 && InjectionRateScriptList.size() > 0){
                ui->ProgramPrints->append("Print List has: " + QString::number(InjectionVolumeScriptList.size()) + " injection volume entries");
                ui->ProgramPrints->append("Print List has: " + QString::number(InjectionRateScriptList.size()) + " injection rate entries");
                ui->LiveValueList4->setCurrentIndex(4);
                ui->LiveValue4->setText(QString::number(InjectionVolumeScriptList.at(0).toDouble()));
                ui->LiveValueList5->setCurrentIndex(5);
                ui->LiveValue5->setText(QString::number(InjectionRateScriptList.at(0).toDouble()));
            }
        }
    }
}

/**
 * @brief MainWindow::on_ClearPrintScript_clicked
 * Clears selected print list
 */
void MainWindow::on_ClearPrintScript_clicked()
{
    ui->PrintScriptFile->clear();
    if (PrintScript == 0)
    {

    }
}
/*******************************************Peripheral Connections*********************************************/
/**
 * @brief MainWindow::on_LightEngineConnectButton_clicked
 * Connect to light engine, gets last error code to validate that
 * no errors have occured and connection is working
 */
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

/**
 * @brief MainWindow::on_StageConnectButton_clicked
 * Connects to stage, if succesful gets position, sends
 * home commands and gets position
 */
void MainWindow::on_StageConnectButton_clicked()
{
    //SMC.SMC100CClose();
    Stage.StageClose(StageType);
    QString COMSelect = ui->COMPortSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    //if (SMC.SMC100CInit(COM) == true && SMC.Home() == true)
    if (Stage.StageInit(COM, StageType) == true && Stage.StageHome(StageType) == true)
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

/**
 * @brief MainWindow::on_PumpConnectButton_clicked
 * Connects to pump
 */
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

/*********************************************Print Parameters*********************************************/
/**
 * @brief MainWindow::on_ResinSelect_activated
 * @param arg1: The resin selected
 * Prints the resin selected to the terminal
 */
void MainWindow::on_ResinSelect_activated(const QString &arg1)
{
    ui->ProgramPrints->append(arg1 + " Selected");
}

/**
 * @brief MainWindow::on_AutoCheckBox_stateChanged
 * @param arg1: The state of the AutoCheckBox
 * If AutoCheckBox is set, go into auto mode
 */
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

/**
 * @brief MainWindow::on_AutoCheckBox_clicked
 * Guard on edge case that sets the checkbox while it's actual state is unchecked
 */
void MainWindow::on_AutoCheckBox_clicked()
{
    int SliceCount = ui->FileList->count();
    if (SliceCount < 1)
    {
        ui->AutoCheckBox->setChecked(false);
    }
}

/**
 * @brief MainWindow::on_SetMaxImageUpload_clicked
 * Sets the max image upload, used to avoid large error buildup
 * or large wait times due to large uploads
 */
void MainWindow::on_SetMaxImageUpload_clicked()
{
    uint MaxImageVal = ui->MaxImageUpload->value(); //Grab value from UI
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


/**
 * @brief MainWindow::on_setPrintSpeed_clicked
 * Sets PrintSpeed variable from value inputted by user, also triggers automode
 */
void MainWindow::on_setPrintSpeed_clicked()
{
    PrintSpeed = (ui->PrintSpeedParam->value());
    QString PrintSpeedString = "Set Print Speed to: " + QString::number(PrintSpeed) + " um/s";
    ui->ProgramPrints->append(PrintSpeedString);
    AutoMode();
}

/**
 * @brief MainWindow::on_SetPrintHeight_clicked
 * Sets PrintHeight variable from value inputted by user, also triggers automode
 */
void MainWindow::on_SetPrintHeight_clicked()
{
    PrintHeight = (ui->PrintHeightParam->value());
    QString PrintHeightString = "Set Print Speed to: " + QString::number(PrintHeight) + "" ;
    ui->ProgramPrints->append(PrintHeightString);
    AutoMode();
}

/**
 * @brief MainWindow::on_SetIntialAdhesionTimeButton_clicked
 * Sets InitialExposure variable from the value inputted by the user
 */
void MainWindow::on_SetIntialAdhesionTimeButton_clicked()
{
    InitialExposure = (ui->InitialAdhesionParameter->value());
    QString InitialExposureString = "Set Initial Exposure to: " + QString::number(InitialExposure) + " s";
    ui->ProgramPrints->append(InitialExposureString);
}

/**
 * @brief MainWindow::on_SetStartingPosButton_clicked
 * Sets Starting Position variable from the value inputted by the user
 * (also doubles as debug tool for GetPosition(), will be removed)
 */
void MainWindow::on_SetStartingPosButton_clicked()
{
    StartingPosition = (ui->StartingPositionParam->value());
    QString StartingPositionString = "Set Starting Position to: " + QString::number(StartingPosition) + " mm";
    ui->ProgramPrints->append(StartingPositionString);
    //QString CurrentPosition = SMC.GetPosition();
    QString CurrentPosition = Stage.StageGetPosition(StageType);
    CurrentPosition = CurrentPosition.remove(0,3);
    ui->ProgramPrints->append("Stage is currently at: " + CurrentPosition + "mm");
    ui->CurrentPositionIndicator->setText(CurrentPosition);
}

/**
 * @brief MainWindow::on_SetSliceThickness_clicked
 * Sets Slicethickness variable from the value inputted by the user
 */
void MainWindow::on_SetSliceThickness_clicked()
{
    SliceThickness = (ui->SliceThicknessParam->value()/1000);
    QString ThicknessString = "Set Slice Thickness to: " + QString::number(SliceThickness*1000) + " μm";
    ui->ProgramPrints->append(ThicknessString);
}
/*******************************************Stage Parameters********************************************/
/**
 * @brief MainWindow::on_SetStageVelocity_clicked
 * Sets StageVelocity variable from the value inputted by the user,
 * also directly sends command to stage to set velocity
 */
void MainWindow::on_SetStageVelocity_clicked()
{
    StageVelocity = (ui->StageVelocityParam->value());
    //SMC.SetVelocity(StageVelocity); //Sends command to stage
    Stage.SetStageVelocity(StageVelocity, StageType);
    QString VelocityString = "Set Stage Velocity to: " + QString::number(StageVelocity) +" mm/s";
    ui->ProgramPrints->append(VelocityString);
}

/**
 * @brief MainWindow::on_SetStageAcceleration_clicked
 * Sets StageAcceleration variable from the value inputted by the user,
 * also directly sends command to stage to set acceleration
 */
void MainWindow::on_SetStageAcceleration_clicked()
{
    StageAcceleration = (ui->StageAccelParam->value());
    //SMC.SetAcceleration(StageAcceleration); //Sends command to stage
    Stage.SetStageAcceleration(StageAcceleration, StageType);
    QString AccelerationString = "Set Stage Acceleration to: " + QString::number(StageAcceleration) + " mm/s";
    ui->ProgramPrints->append(AccelerationString);
}

/**
 * @brief MainWindow::on_SetMaxEndOfRun_clicked
 * Sets MaxEndOfRun variable from the value inputted by the user,
 * also directly sends command to stage to set max end of run
 */
void MainWindow::on_SetMaxEndOfRun_clicked()
{
    MaxEndOfRun = (ui->MaxEndOfRun->value());
    //SMC.SetPositiveLimit(MaxEndOfRun); //Sends command to stage
    Stage.SetStagePositiveLimit(MaxEndOfRun, StageType);
    QString MaxEndOfRunString = "Set Max End Of Run to: " + QString::number(MaxEndOfRun) + " mm";
    ui->ProgramPrints->append(MaxEndOfRunString);
}

/**
 * @brief MainWindow::on_SetMinEndOfRun_clicked
 * Sets MinEndOfRun variable from the value inputted by the user,
 * also directly sends command to stage to set min end of run
 */
void MainWindow::on_SetMinEndOfRun_clicked()
{
    MinEndOfRun = (ui->MinEndOfRunParam->value());
    //SMC.SetNegativeLimit(MinEndOfRun); //Sends command to stage
    Stage.SetStageNegativeLimit(MinEndOfRun, StageType);
    QString MinEndOfRunString = "Set Min End Of Run to: " + QString::number(MinEndOfRun) + " mm";
    ui->ProgramPrints->append(MinEndOfRunString);
}

/******************************************Light Engine Parameters********************************************/
/**
 * @brief MainWindow::on_SetDarkTime_clicked
 * Sets DarkTime variable from the value selected by the user
 */
void MainWindow::on_SetDarkTime_clicked()
{
    DarkTime = (ui->DarkTimeParam->value()*1000);
    QString DarkTimeString = "Set Dark Time to: " + QString::number(DarkTime/1000) + " ms";
    ui->ProgramPrints->append(DarkTimeString);
}

/**
 * @brief MainWindow::on_SetExposureTime_clicked
 * Sets ExposureTime variable from the value selected by the user
 */
void MainWindow::on_SetExposureTime_clicked()
{
    ExposureTime = (ui->ExposureTimeParam->value()*1000);
    QString ExposureTimeString = "Set Exposure Time to: " + QString::number(ExposureTime/1000) + " ms";
    ui->ProgramPrints->append(ExposureTimeString);
}

/**
 * @brief MainWindow::on_SetUVIntensity_clicked
 * Sets UVIntensity from the value selected by the user
 */
void MainWindow::on_SetUVIntensity_clicked()
{
    UVIntensity = (ui->UVIntensityParam->value());
    QString UVIntensityString = "Set UV Intensity to: " + QString::number(UVIntensity);
    ui->ProgramPrints->append(UVIntensityString);
}
/*******************************************Pump Parameters********************************************/
/**
 * @brief MainWindow::on_ContinuousInjection_clicked
 */
void MainWindow::on_ContinuousInjection_clicked()
{
    if (ui->ContinuousInjection->isChecked() == true){
        Pump.SetTargetVolume(0);
        ContinuousInjection = true;
        ui->ProgramPrints->append("Continuous Injection Selected");
    }
    else{
        ContinuousInjection = false;
        ui->ProgramPrints->append("Continuous Injection Disabled");
    }
}


/**
 * @brief MainWindow::on_SetInfuseRate_clicked
 */
void MainWindow::on_SetInfuseRate_clicked()
{
    InfusionRate = ui->InfuseRateParam->value();
    Pump.SetInfuseRate(InfusionRate);
    QString InfusionRateString = "Set Infusion Rate to: " + QString::number(InfusionRate) + " μl/s";
    ui->ProgramPrints->append(InfusionRateString);
}

/**
 * @brief MainWindow::on_SetVolPerLayer_clicked
 */
void MainWindow::on_SetVolPerLayer_clicked()
{
    InfusionVolume = ui->InfuseRateParam->value();
    Pump.SetTargetVolume(InfusionVolume);
    QString InfusionVolumeString = "Set Infusion Volume per layer to: " + QString::number(InfusionVolume) + " ul";
    ui->ProgramPrints->append(InfusionVolumeString);
}

void MainWindow::on_SetInitialVolume_clicked()
{

}

/*******************************************Live Value Monitoring********************************************/
/**
 * @brief MainWindow::on_LiveValueList1_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList1_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV1: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList2_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList2_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV2: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList3_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList3_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV3: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList4_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList4_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV4: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList5_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList5_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV5: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList6_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList6_activated(const QString &arg1)
{
    ui->ProgramPrints->append("LV6: " + arg1);
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

/**
 * @brief MainWindow::saveText
 * Saves Terminal Window printout to .txt log upon print
 * completion or abort
 */
void MainWindow::saveText()
{
     QString Log = ui->ProgramPrints->toPlainText();
     QString LogDate = CurrentDateTime.toString("yyyy-MM-dd");
     QString LogTime = CurrentDateTime.toString("hh.mm.ss");
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

/**
 * @brief MainWindow::validateStartingPosition
 * Validates whether stage has reached it's starting position,
 * repeats at 1 Hz until starting position is reached. Deprecated??
 */
void MainWindow::validateStartingPosition()
{
    QString CurrentPosition = Stage.StageGetPosition(StageType);
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

/**
 * @brief MainWindow::initStageSlot
 * Initializes stage and moves it to starting position
 */
void MainWindow::initStageSlot(void)
{
    emit(on_GetPosition_clicked());
    if (StageType == STAGE_SMC)
    {
        if (GetPosition < (StartingPosition-3.2))
        {
            if (StagePrep1 == false)
            {
                //SMC.SetVelocity(3);
                Stage.SetStageVelocity(3, StageType);
                Sleep(20);
                //SMC.AbsoluteMove((StartingPosition-3));
                Stage.StageAbsoluteMove(StartingPosition-3, StageType);
                Sleep(20);
                //QString MoveTime = SMC.GetMotionTime();
                //MoveTime.remove(0,3);
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
    else //stage is in gcode mode
    {
        ui->ProgramPrints->append("Auto stage initialization disabled for iCLIP, please move stage to endstop with manual controls");
    }
}

/**
 * @brief MainWindow::fineMovement
 * Once the stage is within 3 mm of the window move to slow movement to
 * avoid creating bubbles in the resin
 */
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
            //SMC.SetVelocity(0.3);
            Stage.SetStageVelocity(0.3, StageType);
            Sleep(20);
            //SMC.AbsoluteMove(StartingPosition);
            Stage.StageAbsoluteMove(StartingPosition, StageType);
            ui->ProgramPrints->append("Fine Stage Movement");
            StagePrep2 = true;
        }
        QTimer::singleShot(1000, this, SLOT(fineMovement()));
    }
}

/**
 * @brief MainWindow::verifyStageParams
 * Resends the correct stage parameter to the stage
 */
void MainWindow::verifyStageParams()
{
    ui->ProgramPrints->append("Verifying Stage Parameters");
    Sleep(50);
    //SMC.SetAcceleration(StageAcceleration);
    Stage.SetStageAcceleration(StageAcceleration, StageType);
    Sleep(50);
    //SMC.SetNegativeLimit(MinEndOfRun);
    Stage.SetStageNegativeLimit(MinEndOfRun, StageType);
    Sleep(50);
    //SMC.SetPositiveLimit(MaxEndOfRun);
    Stage.SetStagePositiveLimit(MaxEndOfRun, StageType);
    Sleep(50);
    //SMC.SetVelocity(StageVelocity);
    Stage.SetStageVelocity(StageVelocity, StageType);
}

/**
 * @brief MainWindow::AutoMode
 * Calculates exposure time and slice thickness based on input
 * print height and print speed
 */
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

/**
 * @brief MainWindow::initConfirmationScreen
 * @return bool: true if user has confirmed, false otherwise
 * Initializes the confirmation screen so that the user can
 * confirm whether the input parameters are correct
 */
bool MainWindow::initConfirmationScreen()
{
    bool retVal = false;
    QMessageBox confScreen;
    QPushButton *cancelButton = confScreen.addButton(QMessageBox::Cancel);
    QPushButton *okButton = confScreen.addButton(QMessageBox::Ok);
    confScreen.setStyleSheet("QLabel{min-width:300 px; font-size: 24px; text-align:center;} QPushButton{ width:150px; font-size: 18px; } QTextEdit{min-height:150px; font-size: 16px;}");
    confScreen.setText("Please Confirm Print Parameters");

    QString DetailedText;
    if(PrinterType == CLIP30UM){
        DetailedText += "Printer set to CLIP 30um\n";
    }
    else if(PrinterType == ICLIP){
        DetailedText += "Printer set to iCLIP\n";
    }

    if(ProjectionMode == POTF){
        DetailedText += "POTF projection mode selected\n";
    }
    else if(ProjectionMode == VIDEOPATTERN){
        DetailedText += "Video Pattern projection mode selected\n";
    }

    if(MotionMode == STEPPED){
        DetailedText += "Motion mode set to stepped\n";
    }
    else if(MotionMode == CONTINUOUS){
        DetailedText += "Motion mode set to continuous\n";
    }

    if(PumpingMode == OFF){
        DetailedText += "Pumping disabled\n";
    }
    else if(PumpingMode == ON){
        DetailedText += "Pumping Enabled\n";
    }

    DetailedText += "Max Image Upload: " + QString::number(MaxImageUpload) + "images\n";
    DetailedText += "Bit Depth set to: " + QString::number(BitMode) + "\n";
    DetailedText += "Initial Exposure Time: " + QString::number(InitialExposure) + "s\n";
    if (PrinterType == CLIP30UM){
        DetailedText += "Starting Position: " + QString::number(StartingPosition) + " mm\n";
    }
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
        DetailedText += "Exposure Time controlled by print script\n";
        DetailedText += "UV Intensity controlled by print script\n";
        DetailedText += "Dark Time controlled by print script\n";
    }
    else
    {
        DetailedText += "Exposure Time: " + QString::number(ExposureTime/1000) + " ms\n";
        DetailedText += "UV Intensity: " + QString::number(UVIntensity) + "\n";
        DetailedText += "Dark Time " + QString::number(DarkTime/1000) + " ms\n";
    }

    DetailedText += "Stage Velocity: " + QString::number(StageVelocity) + " mm/s\n";
    if (PrinterType == CLIP30UM){
        DetailedText += "Stage Acceleration: " + QString::number(StageAcceleration) + " mm/s^2\n";
        DetailedText += "Max End Of Run: " + QString::number(MaxEndOfRun) + " mm\n";
        DetailedText += "Min End Of Run: " + QString::number(MinEndOfRun) + " mm\n";
    }
    else if (PrinterType == ICLIP){
        DetailedText += "Infusion volume per layer: " + QString::number(InfusionVolume) + "ul\n";
        DetailedText += "Infusion rate per layer: " + QString::number(InfusionRate) + "ul/s";
    }
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

/**
 * @brief MainWindow::ValidateSettings
 * @return bool: true if settings are valid, false otherwise
 *
 */
bool MainWindow::ValidateSettings(void)
{
    //Validate Slicethickness
    if (SliceThickness <= 0){
        showError("Invalid Slice Thickness");
        ui->ProgramPrints->append("Invalid Slice Thickness");
        return false;
    }
    //Validate StageVelocity
    else if (StageVelocity <= 0 || StageVelocity > 10){
        showError("Invalid Stage Velocity");
        ui->ProgramPrints->append("Invalid Stage Velocity");
        return false;
    }
    //Validate StageAcceleration
    else if (StageAcceleration <= 0 || StageAcceleration > 10){
        showError("Invalid Stage Acceleration");
        ui->ProgramPrints->append("Invalid Stage Acceleration");
        return false;
    }
    //Validate MaxEndOfRun
    else if (MaxEndOfRun < -5 || MaxEndOfRun > 65 || MinEndOfRun >= MaxEndOfRun){
        showError("Invalid Max End Of Run");
        ui->ProgramPrints->append("Invalid Max End Of Run");
        return false;
    }
    //Validate StageMinEndOfRun
    else if (MinEndOfRun < -5 || MinEndOfRun > 65 || MinEndOfRun >= MaxEndOfRun){
        showError("Invalid Min End Of Run");
        ui->ProgramPrints->append("Invalid Min End Of Run");
        return false;
    }
    //Validate DarkTime
    else if (DarkTime < 0){
        showError("Invalid Dark Time");
        ui->ProgramPrints->append("Invalid Dark Time");
        return false;
    }
    //Validate ExposureTime
    else if (ExposureTime <= 0){
        showError("Invalid Exposure Time");
        ui->ProgramPrints->append("Invalid Exposure Time");
        return false;
    }
    //Validate UVIntensity
    else if (UVIntensity < 1 || UVIntensity > 255){
        showError("Invalid UV Intensity");
        ui->ProgramPrints->append("Invalid UVIntensity");
        return false;
    }
    ui->ProgramPrints->append("All Settings Valid");
    return true;
}

/*******************************************Settings Functions*********************************************/
/**
 * @brief MainWindow::saveSettings
 * Saves settings so that they are initialized upon startup
 */
void MainWindow::saveSettings()
{
    QSettings settings;
    settings.setValue("ExposureTime", ExposureTime);
    settings.setValue("DarkTime", DarkTime);
    settings.setValue("UVIntensity", UVIntensity);

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

    settings.setValue("PrinterType", PrinterType);
    settings.setValue("StageType", StageType);

    settings.setValue("MotionMode", MotionMode);
    settings.setValue("PumpingMode",PumpingMode);
    settings.setValue("PumpingParameter", PumpingParameter);
    settings.setValue("BitMode", BitMode);

    settings.setValue("InfusionRate", InfusionRate);
    settings.setValue("InfusionVolume", InfusionVolume);

    settings.setValue("StageCOM", ui->COMPortSelect->currentIndex());
    settings.setValue("PumpCOM", ui->COMPortSelectPump->currentIndex());
}

/**
 * @brief MainWindow::loadSettings
 * Loads the settings from file to module variables
 */
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

        PrinterType = settings.value("PrinterType", CLIP30UM).toDouble();

        MotionMode = settings.value("MotionMode", STEPPED).toDouble();
        PumpingMode = settings.value("PumpingMode", 0).toDouble();
        PumpingParameter = settings.value("PumpingParameter", 0).toDouble();
        BitMode = settings.value("BitMode", 1).toDouble();
        InfusionRate = settings.value("InfusionRate", 5).toDouble();
        InfusionVolume = settings.value("InfusionVolume", 5).toDouble();

        loadSettingsFlag = true;

        ui->COMPortSelect->setCurrentIndex(settings.value("StageCOM", 0).toInt());
        ui->COMPortSelectPump->setCurrentIndex(settings.value("PumpCOM", 0).toInt());
    }
}

/**
 * @brief MainWindow::initSettings
 * Updates the UI to reflect the settings
 */
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

    if(PrinterType == CLIP30UM){
        ui->CLIPSelect->setChecked(true);
        ui->DICLIPSelect->setChecked(false);
        emit(on_CLIPSelect_clicked());
    }
    else if(PrinterType == ICLIP){
        ui->DICLIPSelect->setChecked(true);
        ui->CLIPSelect->setChecked(false);
        emit(on_DICLIPSelect_clicked());
    }

    if(MotionMode == STEPPED){
        ui->SteppedMotion->setChecked(true);
        ui->ContinuousMotion->setChecked(false);
        emit(on_SteppedMotion_clicked());
    }
    else if(MotionMode == CONTINUOUS){
        ui->ContinuousMotion->setChecked(true);
        ui->SteppedMotion->setChecked(false);
        emit(on_ContinuousMotion_clicked());
    }

    if(PumpingMode == ON){
        ui->pumpingCheckBox->setChecked(true);
        emit(on_pumpingCheckBox_clicked());
    }
    else if(PumpingMode == OFF){
        ui->pumpingCheckBox->setChecked(false);
    }

    ui->pumpingParameter->setValue(PumpingParameter);
    ui->BitDepthParam->setValue(BitMode);
    ui->InfuseRateParam->setValue(InfusionRate);
    ui->VolPerLayerParam->setValue(InfusionVolume);

}

/*******************************************Plot Functions*********************************************/
/**
 * @brief MainWindow::initPlot
 * Initializes live plotting of stage position
 */
void MainWindow::initPlot()
{
    ui->LivePlot->addGraph();
    ui->LivePlot->graph(0)->setName("Print Progress");
    ui->LivePlot->xAxis->setLabel("Time (s)");
    ui->LivePlot->yAxis->setLabel("Position (mm)");
    if (PrintScript ==1)
    {
        TotalPrintTimeS = 0;
        for (int i= 0; i < ExposureScriptList.size(); i++)
            {
                //ui->ProgramPrints->append("Exp. Time: " + QString::number(ExposureScriptList.at(i).toDouble()/1000));
                TotalPrintTimeS += ExposureScriptList.at(i).toDouble()/1000;
                //ui->ProgramPrints->append("Current Total :" + QString::number(TotalPrintTimeS));
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

/**
 * @brief MainWindow::updatePlot
 * Updates current plot with new stage position
 */
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
            RemainingPrintTimeS -= ExposureScriptList.at(layerCount - 1).toDouble()/1000;
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
 * ********************Development***************************
 * ***********************************************************/
/**
 * @brief MainWindow::VP8bitWorkaround
 *
 */
void MainWindow::VP8bitWorkaround()
{

}
/*************************************************************
 * ********************Graveyard***************************
 * ***********************************************************/
//Snippets that may be useful in the future but the overall functionality has been deprecated
#if 0
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
                    ui->Pr ogramPrints->append("No Sequencer Errors have Occured");

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
#endif
