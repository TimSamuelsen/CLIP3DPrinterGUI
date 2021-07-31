#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

DLP9000 DLP; //DLP object for calling functions from dlp9000.cpp, test if this still works without being a static

//Module level variables
static bool InitialExposureFlag = true; //Flag to indicate print process to do intial expore first, set false after intial exposure never set true again
//Settings are static so they can be accessed from outside anywhere in this module

//Module level variables for print parameters
//*static double SliceThickness;
//*static double StageVelocity;
//*static double StageAcceleration;
//*static double StartingPosition;
//*static double MaxEndOfRun;
//*static double MinEndOfRun;
//*static double ExposureTime;
//*static double DarkTime;
//*static uint InitialExposure;
//*static int UVIntensity;
//*static int MaxImageUpload = 20;

//Injection parameters
//*static double InfusionRate;
//*static double InfusionVolume;
//*static double InitialVolume;
//*static double InjectionDelayParam;
//*int InjectionDelayFlag = 0;


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
QStringList StageVelocityScriptList;
QStringList StageAccelerationScriptList;
QStringList PumpHeightScriptList;
QStringList LayerThicknessScriptList;
static int PrintScript = 0; //For selecting type of printscript, currently: 0 = no printscript, 1 = exposure time print script

//Projection mode parameters
//*static int ProjectionMode = POTF; //For selecting projection mode, 0 = POTF mode, 1 = Video Pattern HDMI
int BitLayer = 1;
int FrameCount = 0;
int ReSyncFlag = 0;
int ReSyncCount = 0;

//Bit depth selection parameters
//*static int BitMode = 1;
static int VP8Bit = OFF;
QStringList FrameList;

//Stepped motion parameters
static int MotionMode = 0; //Set stepped motion as default
static bool inMotion;
static double PrintEnd;

//Pumping parameter
bool PumpingMode = 0;
bool ContinuousInjection = false;
double PumpingParameter;

//Stage select parameter
//*static Stage_t StageType = STAGE_SMC;
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
    Stage.StageHome(m_PrintSettings.StageType);
    //SMC.SMC100CClose(); //Close main window connection to allow manual stage control to take over, ideally this would not be needed and it would stay on the same connection.
    Stage.StageClose(m_PrintSettings.StageType);

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
    char* ReadPosition = Stage.StageGetPosition(m_PrintSettings.StageType);

    //If the string length returned from SMC.GetPosition is > 1, then no errors have occurred
    if (strnlen(ReadPosition,50) > 1)
    {
        QString CurrentPosition = QString::fromUtf8(ReadPosition); //convert char* to QString
        if (PrinterType == CLIP30UM){
            CurrentPosition.remove(0,3); //Removes address and command code
            CurrentPosition.chop(2); //To be removed...
        }
        else if(PrinterType == ICLIP){

        }
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
        EnableParameter(MAX_IMAGE, ON);
        m_PrintSettings.ProjectionMode = POTF; //Set projection mode to POTF
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
        EnableParameter(MAX_IMAGE, OFF);
        initImagePopout(); //Open projection window
        m_PrintSettings.ProjectionMode = VIDEOPATTERN; //Set projection mode to video pattern

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
    QMessageBox VlockPopup; //prep video popout
    static uint8_t repeatCount = 0; //start repeatCount at 0
    unsigned char HWStatus, SysStatus, MainStatus; //Prep variables for LCR_GetStatus

    //If first attempt
    if (repeatCount == 0)
        ui->ProgramPrints->append("Attempting to Lock to Video Source");

    //if attempt to get status is succesful
    if (LCR_GetStatus(&HWStatus, &SysStatus, &MainStatus) == 0)
    {
        //If BIT3 in main status is set, then video source has already been locked
        if(MainStatus & BIT3){
            ui->ProgramPrints->append("External Video Source Locked");
            VlockPopup.close();

            //If attempt to set to Video pattern mode is not succesful
            if (LCR_SetMode(PTN_MODE_VIDEO) < 0)
            {
                showError("Unable to switch to video pattern mode");
                ui->VP_HDMIcheckbox->setChecked(false); //Uncheck video pattern mode

                //Reset to POTF mode
                ui->POTFcheckbox->setChecked(true);
                emit(on_POTFcheckbox_clicked());
            }
            ui->ProgramPrints->append("Video Pattern Mode Enabled");
            return;
        }
        else{ //Video source is currently not locked
            ui->ProgramPrints->append("External Video Source Not Locked, Please Wait");

            //Get current display mode and print to terminal, used for debug
            API_DisplayMode_t testDM;
            LCR_GetMode(&testDM);
            ui->ProgramPrints->append(QString::number(testDM));

            //Repeats 15 times (15 seconds) before telling user that an error has occurred
            if(repeatCount < 15)
            {
                //Start 1 second timer, after it expires check if video source is locked
                QTimer::singleShot(1000, this, SLOT(Check4VideoLock()));
                repeatCount++;
            }
            else //System has attempted 15 times to lock to video source, and attempt is determined to be a failure
            {
                ui->ProgramPrints->append("External Video Source Lock Failed");
                showError("External Video Source Lock Failed");
                ui->VP_HDMIcheckbox->setChecked(false); //Uncheck video pattern mode

                //Reset to POTF mode
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
    ImagePopoutUI = new imagepopout; //prep new image popout window
    ImagePopoutUI->show(); //Display new window
    ImagePopoutUI->windowHandle()->setScreen(qApp->screens()[1]); //Set window to extended display
    ImagePopoutUI->showFullScreen(); //set window to fullscreen

    //Prep image list for video pattern mode
    QStringList imageList;
    //For every file in the file list, enter it into imageList
    for(int i = 0; (i < (ui->FileList->count())); i++)
    {
        QListWidgetItem * item;
        item = ui->FileList->item(i);
        imageList << item->text();
    }
}

/**
 * @brief MainWindow::on_DICLIPSelect_clicked
 * Preps the system for use with iCLIP printer
 */
void MainWindow::on_DICLIPSelect_clicked()
{
    m_PrintSettings.StageType = STAGE_GCODE; //Set stage to Gcode for the iCLIP printer
    PrinterType = ICLIP; //Update printer type

    //Enable the injection parameters
    EnableParameter(CONTINUOUS_INJECTION, ON);
    EnableParameter(INJECTION_VOLUME, ON);
    EnableParameter(INJECTION_RATE, ON);
    EnableParameter(INITIAL_VOLUME, ON);
    EnableParameter(INJECTION_DELAY, ON);

    //Disable the starting position selection
    EnableParameter(STARTING_POSITION, OFF);

    //Disable the stage software limits
    EnableParameter(MAX_END, OFF);
    EnableParameter(MIN_END, OFF);
}

/**
 * @brief MainWindow::on_CLIPSelect_clicked
 * Preps the system for use with the 30 um printer
 */
void MainWindow::on_CLIPSelect_clicked()
{
    m_PrintSettings.StageType = STAGE_SMC; //Set stage to SMC100CC for the 30 um CLIP printer
    PrinterType = CLIP30UM; //Update printer type

    //Disable the injection parameters
    EnableParameter(CONTINUOUS_INJECTION, OFF);
    EnableParameter(INJECTION_VOLUME, OFF);
    EnableParameter(INJECTION_RATE, OFF);
    EnableParameter(INITIAL_VOLUME, OFF);
    EnableParameter(INJECTION_DELAY, OFF);

    //Enable the starting position selection
    EnableParameter(STARTING_POSITION, ON);

    //Enable the stage software limits
    EnableParameter(MAX_END, ON);
    EnableParameter(MIN_END, ON);
}


/**
 * @brief MainWindow::on_SetBitDepth_clicked
 * Sets the module variable BitMode which is used to determine the bit depth
 * the user wishes to print with
 */
void MainWindow::on_SetBitDepth_clicked()
{
    m_PrintSettings.BitMode = ui->BitDepthParam->value(); //Grab value from GUI bit depth parameter
    ui->ProgramPrints->append("Bit-Depth set to: " + QString::number(m_PrintSettings.BitMode));
}

/**
 * @brief MainWindow::on_SteppedMotion_clicked
 * Sets motion mode to stepped, this is the default
 */
void MainWindow::on_SteppedMotion_clicked()
{
    //If the stepped motion checkbox is checked
    if(ui->SteppedMotion->isChecked() == true)
    {
        MotionMode = STEPPED; //Set MotionMode to STEPPED
        ui->ContinuousMotion->setChecked(false); //Updates UI to reflect stepped mode
        ui->ProgramPrints->append("Stepped Motion Selected");
        EnableParameter(DARK_TIME, ON); //Enable dark time selection
    }
}

/**
 * @brief MainWindow::on_ContinuousMotion_clicked
 * Sets motion mode to continuous
 */
void MainWindow::on_ContinuousMotion_clicked()
{
    //If the continuos motion checkbox is checked
    if(ui->ContinuousMotion->isChecked() == true)
    {
        MotionMode = CONTINUOUS; //Set MotionMode to Continuous
        ui->SteppedMotion->setChecked(false); //Update UI to reflect continuous mode
        ui->ProgramPrints->append("Continuous Motion Selected");

        //Disable dark time and set it to 0, because there is no dark time in Continuous mode
        m_PrintSettings.DarkTime = 0;
        EnableParameter(DARK_TIME, OFF);
        ui->DarkTimeParam->setValue(0);
    }
}

/**
 * @brief MainWindow::on_pumpingCheckBox_clicked
 * Enables pumping mode in which the stage movement is exaggerated
 * to avoid the part sticking to the window
 */
void MainWindow::on_pumpingCheckBox_clicked()
{
    //If pumping checkbox is checked
    if(ui->pumpingCheckBox->isChecked() == true){
        PumpingMode = ON; //Enable pumping mode

        //Update UI
        EnableParameter(PUMP_HEIGHT, ON);
        ui->ProgramPrints->append("Pumping Enabled");
    }
    else{
        PumpingMode = OFF; //Disable pumping mode

        //Update UI
        EnableParameter(PUMP_HEIGHT, OFF);
        ui->ProgramPrints->append("Pumping Disabled");
    }
}

/**
 * @brief MainWindow::on_setPumping_clicked
 * Sets the pumping depth parameter
 */
void MainWindow::on_setPumping_clicked()
{
    //If pumping is currently enabled
    if(PumpingMode == ON){
        PumpingParameter = ui->pumpingParameter->value()/1000; //Grab value from UI and divide by 1000 to scale units
        ui->ProgramPrints->append("Pumping depth set to: " + QString::number(PumpingParameter*1000) + " μm");
    }
    else{ //Pumping is currently disabled
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
    //If the confirmation screen was approved by user
    if (initConfirmationScreen())
    {
        LCR_PatternDisplay(0); //Turn off any projection
        initStageSlot(); //Initializes stage with correct parameters and moves controlled to starting position

        //If file list is not empty
        if (ui->FileList->count() > 0)
        {
            //Upload images for initial exposure
            QListWidgetItem * firstItem = ui->FileList->item(0); //Get first image
            QStringList firstImage;

            //Create an imagelist that contain n copies of first image, where n = InitialExposure in integer seconds
            for (uint i = 0; i < m_PrintSettings.InitialExposure; i++)
            {
                firstImage << firstItem->text();
            }

            //Add initial exposure patterns to local memory
            DLP.AddPatterns(firstImage, m_PrintSettings, m_PrintScript, m_PrintControls);
            //If in POTF mode
            if(m_PrintSettings.ProjectionMode == POTF){
                nSlice = ui->FileList->count();

                //Grab images from the file list up until the max image upload is reached
                QListWidgetItem * item;
                QStringList imageList;
                for(int i = 1; (i < (ui->FileList->count())); i++)
                {
                        item = ui->FileList->item(i);
                        imageList << item->text();
                        if ((i + m_PrintSettings.InitialExposure) > m_PrintSettings.MaxImageUpload)
                        {
                            break;
                        }
                }

                //Add patterns first to local memory, send to light engine, and clear local memory
                DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls); //Set initial image to 0
                DLP.updateLUT(m_PrintSettings.ProjectionMode);
                DLP.clearElements();
                remainingImages = m_PrintSettings.MaxImageUpload - m_PrintSettings.InitialExposure;

                //Get directory of images and print to terminal
                QDir dir = QFileInfo(QFile(imageList.at(0))).absoluteDir();
                ui->ProgramPrints->append(dir.absolutePath());
            }
            //If in Video pattern mode
            else if (m_PrintSettings.ProjectionMode == VIDEOPATTERN){
                nSlice = (24/m_PrintSettings.BitMode)*ui->FileList->count(); //Calc nSlice based on bitMode and # of files
                DLP.updateLUT(m_PrintSettings.ProjectionMode); //Send pattern data to light engine
                DLP.clearElements(); //Clear local memory

                //Get image from file list and display in popout window

                QString filename =ui->FileList->item(layerCount)->text();
                QPixmap img(filename);
                ImagePopoutUI->showImage(img);

            }
            ui->ProgramPrints->append(QString::number(nSlice) + " layers to print");

            emit(on_GetPosition_clicked()); //get stage position
            initPlot(); //Initialize plot
            updatePlot(); //Update plot
            ui->StartPrint->setEnabled(true); //Enable the start print button

            //If in continuous motion mode, calculate the print end position
            if(MotionMode == CONTINUOUS){
                if(PrinterType == CLIP30UM){
                    PrintEnd = m_PrintSettings.StartingPosition - (ui->FileList->count()*m_PrintSettings.LayerThickness);
                }
                else if (PrinterType == ICLIP){
                    PrintEnd = ui->FileList->count()*m_PrintSettings.LayerThickness; //Make sure stage is zeroed
                }
                else{
                    showError("PrinterType Error, on_InitializeAndSynchronize_clicked");
                }
                ui->ProgramPrints->append("Print End for continous motion print set to: " + QString::number(PrintEnd));
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
    Stage.StageStop(m_PrintSettings.StageType); //Stop stage movement
    //Add pump stop here
    layerCount = 0xFFFFFF; //Set layer count high to stop print process
    ui->ProgramPrints->append("PRINT ABORTED");
}

/**
 * @brief MainWindow::on_StartPrint_clicked
 * Starts print process, set LED current calculates speed for continuous motion mode
 */
void MainWindow::on_StartPrint_clicked()
{
    //If settings are validated successfully and Initialization has been completed
    if (ValidateSettings() == true)
    {
        Stage.SetStageVelocity(m_PrintSettings.StageVelocity, m_PrintSettings.StageType); //Update stage velocity
        Sleep(10); //Sleep to avoid spamming motor controller
        emit(on_GetPosition_clicked()); //Get stage position to validate that stage connection is working
        Sleep(10);
        emit(on_GetPosition_clicked()); //Repeat get stage position
        ui->ProgramPrints->append("Entering Printing Procedure");

        //Set LED currents to 0 red, 0 green, set blue to chosen UVIntensity
        if (PrintScript == ON){ //If printscript is on
            LCR_SetLedCurrents(0,0,(LEDScriptList.at(0).toInt())); //Set LED intensity to first value in printscript
        }
        else{ //if printscript is off
            LCR_SetLedCurrents(0, 0, m_PrintSettings.UVIntensity); //set static LED intensity
        }

        if (MotionMode == CONTINUOUS){ //continuous motion mode
            double ContStageVelocity = (m_PrintSettings.StageVelocity)/(m_PrintSettings.ExposureTime/(1e6)); //Multiply Exposure time by 1e6 to convert from us to s to get to proper units
            ui->ProgramPrints->append("Continuous Stage Velocity set to " + QString::number(m_PrintSettings.StageVelocity) + "/" + QString::number(m_PrintSettings.ExposureTime) + " = " + QString::number(ContStageVelocity) + " mm/s");
            Stage.SetStageVelocity(ContStageVelocity, m_PrintSettings.StageType);
        }

        PrintStartTime = QTime::currentTime(); //Get print start time from current time
        DLP.startPatSequence(); //Start projection

        //Based on projection mode enter correct print process
        if (m_PrintSettings.ProjectionMode == POTF){ //if in POTF mode
            PrintProcess(); //Start print process
            ui->ProgramPrints->append("Entering POTF print process");
        }
        else{ //else in Video Pattern Mode
            DLP.clearElements();
            PrintProcessVP(); //Start video pattern print process
            ui->ProgramPrints->append("Entering Video Pattern print process");
        }

        //If continuous injection is on then start pump infusion
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
    //If not at the end of the print
    if (layerCount +1 <= nSlice)
    {
        //If there are no remaining images, reupload
        if (remainingImages <= 0)
        {
            //If motion mode is set to continuous, pause stage movement during re-upload
            if (MotionMode == CONTINUOUS){
                Stage.StageStop(m_PrintSettings.StageType);
            }
            LCR_PatternDisplay(0); //Stop projection

            //Grab new images from the imageList starting at current layerCount, capped at max image upload
            QListWidgetItem * item;
            QStringList imageList;
            uint count = 0;
            int MaxImageReupload = m_PrintSettings.MaxImageUpload;
            if (m_PrintSettings.MaxImageUpload > 390)
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

            //Add patterns to local memory, upload to light engine, then clear local memory
            DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
            DLP.updateLUT(m_PrintSettings.ProjectionMode);
            DLP.clearElements();

            remainingImages = count - 1; //Set the remaining images to the # of images uploaded - 1
            layerCount++; //Update layer count
            Sleep(50); //Add delay to allow for light engine process to run
            DLP.startPatSequence(); //Start projection
            if(PrinterType == ICLIP){ //delay is needed for iCLIP printer only
                Sleep(500);
            }
            SetExposureTimer(0, PrintScript, PumpingMode); //Set the exposure time

            //if in continuous motion mode restart movement
            if(MotionMode == CONTINUOUS){
                Stage.StageAbsoluteMove(PrintEnd, m_PrintSettings.StageType);
                inMotion = true;
            }
            ui->ProgramPrints->append("Reupload succesful, current layer: " + QString::number(layerCount));
            ui->ProgramPrints->append(QString::number(remainingImages + 1) + " images uploaded");
        }
        //If in intial exposure mode
        else if (InitialExposureFlag == true)
        {
            SetExposureTimer(InitialExposureFlag, PrintScript, PumpingMode); //Set exposure timer with InitialExposureFlag high
            updatePlot();
            InitialExposureFlag = false; //Set InitialExposureFlag low
            inMotion = false; //Stage is currently not in motion, used for continuos stage movement
            ui->ProgramPrints->append("POTF Exposing Initial Layer " + QString::number(m_PrintSettings.InitialExposure) + "s");
        }
        else
        {
            SetExposureTimer(0, PrintScript, PumpingMode); //set exposure time
            ui->ProgramPrints->append("Layer: " + QString::number(layerCount));

            //Grab the current image file name and print to
            QString filename =ui->FileList->item(layerCount)->text();
            ui->ProgramPrints->append("Image File: " + filename);
            layerCount++; //Update layerCount for new layer
            remainingImages--; //Decrement remaining image counter
            if(MotionMode == STEPPED){
                updatePlot();
            }
            else if (MotionMode == CONTINUOUS){
                if(PrinterType == ICLIP){
                    PrintInfuse();
                    ui->ProgramPrints->append("Injecting " + QString::number(m_InjectionSettings.InfusionVolume) + "ul at " + QString::number(m_InjectionSettings.InfusionRate) + "ul/s");
                }
            }
            double OldPosition = GetPosition;
            emit(on_GetPosition_clicked());
            ui->ProgramPrints->append("Stage moved: " + QString::number(OldPosition - GetPosition));
        }
        return;
    }
    else
    {
        PrintComplete();
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
            for(int i = FrameCount; i < FrameCount + (5*m_PrintSettings.BitMode); i++)
            {
                for (int j = 0; j < (24/m_PrintSettings.BitMode); j++)
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
            if (m_PrintSettings.ProjectionMode == VIDEOPATTERN && m_PrintSettings.BitMode == 8){
                //VP8bitWorkaround();
                //ui->ProgramPrints->append("Using VP 8-bit workaround");
                DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
            }
            else{
                DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
            }
            DLP.updateLUT(m_PrintSettings.ProjectionMode); //update LUT on light engine to upload pattern data
            DLP.clearElements(); //clear pattern data buffer
            Sleep(50); //small delay to ensure that the new patterns are uploaded
            ui->ProgramPrints->append("Resync succesful, frame: " + QString::number(FrameCount) + " layer: " + QString::number(layerCount) + "New Patterns: " + QString::number(count));
            DLP.startPatSequence();
            Sleep(5);
            SetExposureTimer(0, PrintScript, PumpingMode);
            layerCount++; //increment layer counter
            remainingImages--; //decrement remaining images
            BitLayer += m_PrintSettings.BitMode;
        }
        else if (InitialExposureFlag == true) //Initial Exposure
        {
            SetExposureTimer(InitialExposureFlag, PrintScript, PumpingMode);
            BitLayer = 1; //set to first bitlayer
            ReSyncFlag = 1; //resync after intial exposure
            ui->ProgramPrints->append("VP Exposing Initial Layer " + QString::number(m_PrintSettings.InitialExposure) + "s");
            InitialExposureFlag = 0;
        }
        else //Normal print process
        {
            //Start exposuretime timers
            SetExposureTimer(0, PrintScript, PumpingMode);
            //Print information to log
            ui->ProgramPrints->append("VP Layer: " + QString::number(layerCount));
            QString filename =ui->FileList->item(FrameCount)->text();
            ui->ProgramPrints->append("Image File: " + filename);

            layerCount++; //increment layer counter
            remainingImages--; //decrement remaining images

            //updatePlot();
            BitLayer += m_PrintSettings.BitMode;
        }
    }
    else //print complete
    {
        PrintComplete();
    }
}

/**
 * @brief MainWindow::pumpingSlot
 * Pumping slot for when pumping is activated,
 * intermediary step between exposure time and dark time
 */
void MainWindow::pumpingSlot(void)
{
    QTimer::singleShot(m_PrintSettings.DarkTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
    if(PrintScript == ON){
        if (layerCount < PumpHeightScriptList.size()){
            Stage.StageRelativeMove(-PumpHeightScriptList.at(layerCount).toDouble(), m_PrintSettings.StageType);
            ui->ProgramPrints->append("Pumping " + QString::number(PumpHeightScriptList.at(layerCount).toDouble()*1000) +" um");
        }
    }
    else{
        Stage.StageRelativeMove(-PumpingParameter, m_PrintSettings.StageType);
        ui->ProgramPrints->append("Pumping " + QString::number(PumpingParameter*1000) +" um");
    }
}

/**
 * @brief MainWindow::ExposureTimeSlot
 * Handles all dark time actions required
 */
void MainWindow::ExposureTimeSlot(void)
{
    //Set dark timer first for most accurate timing
    SetDarkTimer(PrintScript, MotionMode); //Set dark timer first for best timing
    //Record current time in terminal
    ui->ProgramPrints->append(QTime::currentTime().toString("hh.mm.ss.zzz"));
    updatePlot(); //update plot early in dark time

    //Video pattern mode handling
    if (m_PrintSettings.ProjectionMode == VIDEOPATTERN) //If in video pattern mode
    {
        if (BitLayer > 24) //If end of frame has been reached
        {
            FrameCount++; //increment frame counter
            ui->ProgramPrints->append("New Frame: " + QString::number(FrameCount));
            if (FrameCount < ui->FileList->count()){ //if not at end of file list
                QPixmap img(ui->FileList->item(FrameCount)->text()); //select next image
                ImagePopoutUI->showImage(img); //display next image
            }
            BitLayer = 1; //Reset BitLayer to first layer
            ReSyncCount++; //Add to the resync count
            //If 120 frames have been reached, prepare for resync
            if (ReSyncCount > (120-24)/(24/m_PrintSettings.BitMode)){
                ReSyncFlag = 1;
                ReSyncCount = 0;
            }
        }
    }

    //Print Script handling
    if(PrintScript == ON)
    {
        PrintScriptApply(layerCount, ExposureScriptList, EXPOSURE_TIME);
        PrintScriptApply(layerCount, LEDScriptList, LED_INTENSITY);
        PrintScriptApply(layerCount, DarkTimeScriptList, DARK_TIME);
        PrintScriptApply(layerCount, LayerThicknessScriptList, LAYER_THICKNESS);
        PrintScriptApply(layerCount, StageVelocityScriptList, STAGE_VELOCITY);
        PrintScriptApply(layerCount, StageAccelerationScriptList, STAGE_ACCELERATION);
        if (PumpingMode == ON){
            PrintScriptApply(layerCount, PumpHeightScriptList, PUMP_HEIGHT);
        }
        if (PrinterType == ICLIP){
            PrintScriptApply(layerCount, InjectionVolumeScriptList, INJECTION_VOLUME);
            PrintScriptApply(layerCount, InjectionRateScriptList, INJECTION_RATE);
        }
    }

    //Injection handling
    if (PrinterType == ICLIP)
    {
        if (m_InjectionSettings.InjectionDelayFlag == PRE){
            PrintInfuse();
            QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(StageMove()));
            ui->ProgramPrints->append("Pre-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
        }
        else if(m_InjectionSettings.InjectionDelayFlag == POST){
            StageMove();
            QTimer::singleShot(m_InjectionSettings.InjectionDelayParam, Qt::PreciseTimer, this, SLOT(PrintInfuse()));
             ui->ProgramPrints->append("Post-Injection Delay: " + QString::number(m_InjectionSettings.InjectionDelayParam));
        }
        else{
            PrintInfuse();
            //Stage.StageRelativeMove(-m_PrintSettings.LayerThickness, StageType);
            StageMove();
            ui->ProgramPrints->append("No injection delay");
        }
        ui->ProgramPrints->append("Injecting " + QString::number(m_InjectionSettings.InfusionVolume) + "ul at " + QString::number(m_InjectionSettings.InfusionRate) + "ul/s");
    }
    else{
        StageMove();
        Sleep(5); //5 ms delay for stage com
        emit(on_GetPosition_clicked());
    }
}

/**
 * @brief MainWindow::DarkTimeSlot
 * Dummy slot, may be removed
 */
void MainWindow::DarkTimeSlot(void)
{
    if(m_PrintSettings.ProjectionMode == POTF){
        PrintProcess();
    }
    else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
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
        QTimer::singleShot(m_PrintSettings.InitialExposure*1000, Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
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
                QTimer::singleShot(m_PrintSettings.ExposureTime/1000, Qt::PreciseTimer, this, SLOT(pumpingSlot()));
            }
            else{
                QTimer::singleShot(m_PrintSettings.ExposureTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
            }
            ui->ProgramPrints->append("Exposing: " + QString::number(m_PrintSettings.ExposureTime/1000) + " ms");
        }
    }
}

void MainWindow::SetDarkTimer(int PrintScript, int MotionMode)
{
    double DarkTimeSelect = m_PrintSettings.DarkTime;
    if (MotionMode == STEPPED)
    {
        if (PrintScript == ON){
            if(layerCount < DarkTimeScriptList.size()){
                DarkTimeSelect = DarkTimeScriptList.at(layerCount).toDouble();
            }
            else{
                DarkTimeSelect = DarkTimeScriptList.at(layerCount-2).toDouble();
            }
        }
        else{
            showError("Dark time settings error, SetDarkTime");

        }
        QTimer::singleShot(DarkTimeSelect/1000,  Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
        ui->ProgramPrints->append("Dark time: " + QString::number(DarkTimeSelect));
    }
    else{
        if (inMotion == false){
            Stage.StageAbsoluteMove(PrintEnd, m_PrintSettings.StageType);
        }
        PrintProcess();
    }
}

/**
 * @brief MainWindow::PrintInfuse
 * Function for handling injection for iCLIP
 */
void MainWindow::PrintInfuse()
{
    if (ContinuousInjection == OFF){
        Pump.ClearVolume();
        Sleep(15);
        Pump.StartInfusion();
        ui->ProgramPrints->append("PrintInfuse");
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

        EnableParameter(EXPOSURE_TIME, OFF);
        EnableParameter(LED_INTENSITY, OFF);
        EnableParameter(DARK_TIME, OFF);
        EnableParameter(LAYER_THICKNESS, OFF);
        EnableParameter(STAGE_VELOCITY, OFF);
        EnableParameter(STAGE_ACCELERATION, OFF);
        EnableParameter(PUMP_HEIGHT, OFF);
        EnableParameter(INJECTION_VOLUME, OFF);
        EnableParameter(INJECTION_RATE, OFF);
    }
    else //printscript is not checked
    {
        PrintScript = 0;
        ui->SelectPrintScript->setEnabled(false);
        ui->ClearPrintScript->setEnabled(false);
        ui->PrintScriptFile->setEnabled(false);

        EnableParameter(EXPOSURE_TIME, ON);
        EnableParameter(LED_INTENSITY, ON);
        EnableParameter(DARK_TIME, ON);
        EnableParameter(LAYER_THICKNESS, ON);
        EnableParameter(STAGE_VELOCITY, ON);
        EnableParameter(STAGE_ACCELERATION, ON);
        EnableParameter(PUMP_HEIGHT, ON);
        EnableParameter(INJECTION_VOLUME, ON);
        EnableParameter(INJECTION_RATE, ON);
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
            LayerThicknessScriptList.append(line.split(',').at(3));
            StageVelocityScriptList.append(line.split(',').at(4));
            StageAccelerationScriptList.append(line.split(',').at(5));
            PumpHeightScriptList.append(line.split(',').at(6));
            if(PrinterType == ICLIP){
                InjectionVolumeScriptList.append(line.split(',').at(7));
                InjectionRateScriptList.append(line.split(',').at(8));
            }
    }
     //For testing
    for (int i= 0; i < ExposureScriptList.size(); i++)
    {
        if(PrinterType == ICLIP){
            ui->ProgramPrints->append(ExposureScriptList.at(i) + "," + LEDScriptList.at(i) + "," + DarkTimeScriptList.at(i) + "," + LayerThicknessScriptList.at(i) + "," + StageVelocityScriptList.at(i)
                                      + "," + StageAccelerationScriptList.at(i) + "," + PumpHeightScriptList.at(i) + "," + InjectionVolumeScriptList.at(i) + "," + InjectionRateScriptList.at(i));
        }
        else{
            ui->ProgramPrints->append(ExposureScriptList.at(i) + "," + LEDScriptList.at(i) + "," + DarkTimeScriptList.at(i) + "," + LayerThicknessScriptList.at(i) + "," + StageVelocityScriptList.at(i)
                                      + "," + StageAccelerationScriptList.at(i) + "," + PumpHeightScriptList.at(i));
        }
    }
    ui->ProgramPrints->append("Print List has: " + QString::number(ExposureScriptList.size()) + " exposure time entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(LEDScriptList.size()) + " LED intensity entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(DarkTimeScriptList.size()) + " dark time entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(LayerThicknessScriptList.size()) + " layer thickness entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(StageVelocityScriptList.size()) + " stage velocity entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(StageAccelerationScriptList.size()) + " stage acceleration entries");
    ui->ProgramPrints->append("Print List has: " + QString::number(PumpHeightScriptList.size()) + + " pump height entries");

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
    Stage.StageClose(m_PrintSettings.StageType);
    QString COMSelect = ui->COMPortSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    //if (SMC.SMC100CInit(COM) == true && SMC.Home() == true)
    if (Stage.StageInit(COM, m_PrintSettings.StageType) == true && Stage.StageHome(m_PrintSettings.StageType) == true)
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
    if (MaxImageVal < (m_PrintSettings.InitialExposure + 1)) //Verifies that the max image upload is greater than initial exposure time
    {
        MaxImageVal = m_PrintSettings.InitialExposure + 1;
    }
    else if (MaxImageVal > 398) //Verifies that max image upload is not greater than the storage limit on light engine
    {
        MaxImageVal = 398;
    }
    else //If MaxImageVal is is an acceptable range
    {
        m_PrintSettings.MaxImageUpload = MaxImageVal; //This line might not be needed
    }
    m_PrintSettings.MaxImageUpload = MaxImageVal;
    QString MaxImageUploadString = "Set Max Image Upload to: " + QString::number(m_PrintSettings.MaxImageUpload);
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
    m_PrintSettings.InitialExposure = (ui->InitialAdhesionParameter->value());
    QString InitialExposureString = "Set Initial Exposure to: " + QString::number(m_PrintSettings.InitialExposure) + " s";
    ui->ProgramPrints->append(InitialExposureString);
}

/**
 * @brief MainWindow::on_SetStartingPosButton_clicked
 * Sets Starting Position variable from the value inputted by the user
 * (also doubles as debug tool for GetPosition(), will be removed)
 */
void MainWindow::on_SetStartingPosButton_clicked()
{
    m_PrintSettings.StartingPosition = (ui->StartingPositionParam->value());
    QString StartingPositionString = "Set Starting Position to: " + QString::number(m_PrintSettings.StartingPosition) + " mm";
    ui->ProgramPrints->append(StartingPositionString);
    //QString CurrentPosition = SMC.GetPosition();
    QString CurrentPosition = Stage.StageGetPosition(m_PrintSettings.StageType);
    CurrentPosition = CurrentPosition.remove(0,3);
    ui->ProgramPrints->append("Stage is currently at: " + CurrentPosition + "mm");
    ui->CurrentPositionIndicator->setText(CurrentPosition);
}

/**
 * @brief MainWindow::on_Setm_PrintSettings.LayerThickness_clicked
 * Sets m_PrintSettings.LayerThickness variable from the value inputted by the user
 */
void MainWindow::on_SetSliceThickness_clicked()
{
    m_PrintSettings.LayerThickness = (ui->SliceThicknessParam->value()/1000);
    QString ThicknessString = "Set Slice Thickness to: " + QString::number(m_PrintSettings.LayerThickness*1000) + " μm";
    ui->ProgramPrints->append(ThicknessString);
    //VP8bitWorkaround();
}
/*******************************************Stage Parameters********************************************/
/**
 * @brief MainWindow::on_SetStageVelocity_clicked
 * Sets StageVelocity variable from the value inputted by the user,
 * also directly sends command to stage to set velocity
 */
void MainWindow::on_SetStageVelocity_clicked()
{
    m_PrintSettings.StageVelocity = (ui->StageVelocityParam->value());
    //SMC.SetVelocity(StageVelocity); //Sends command to stage
    Stage.SetStageVelocity(m_PrintSettings.StageVelocity, m_PrintSettings.StageType);
    QString VelocityString = "Set Stage Velocity to: " + QString::number(m_PrintSettings.StageVelocity) +" mm/s";
    ui->ProgramPrints->append(VelocityString);
}

/**
 * @brief MainWindow::on_SetStageAcceleration_clicked
 * Sets StageAcceleration variable from the value inputted by the user,
 * also directly sends command to stage to set acceleration
 */
void MainWindow::on_SetStageAcceleration_clicked()
{
    m_PrintSettings.StageAcceleration = (ui->StageAccelParam->value());
    //SMC.SetAcceleration(StageAcceleration); //Sends command to stage
    Stage.SetStageAcceleration(m_PrintSettings.StageAcceleration, m_PrintSettings.StageType);
    QString AccelerationString = "Set Stage Acceleration to: " + QString::number(m_PrintSettings.StageAcceleration) + " mm/s";
    ui->ProgramPrints->append(AccelerationString);
}

/**
 * @brief MainWindow::on_SetMaxEndOfRun_clicked
 * Sets MaxEndOfRun variable from the value inputted by the user,
 * also directly sends command to stage to set max end of run
 */
void MainWindow::on_SetMaxEndOfRun_clicked()
{
    m_PrintSettings.MaxEndOfRun = (ui->MaxEndOfRun->value());
    Stage.SetStagePositiveLimit(m_PrintSettings.MaxEndOfRun, m_PrintSettings.StageType);
    QString MaxEndOfRunString = "Set Max End Of Run to: " + QString::number(m_PrintSettings.MaxEndOfRun) + " mm";
    ui->ProgramPrints->append(MaxEndOfRunString);
}

/**
 * @brief MainWindow::on_SetMinEndOfRun_clicked
 * Sets MinEndOfRun variable from the value inputted by the user,
 * also directly sends command to stage to set min end of run
 */
void MainWindow::on_SetMinEndOfRun_clicked()
{
    m_PrintSettings.MinEndOfRun = (ui->MinEndOfRunParam->value());
    Stage.SetStageNegativeLimit(m_PrintSettings.MinEndOfRun, m_PrintSettings.StageType);
    QString MinEndOfRunString = "Set Min End Of Run to: " + QString::number(m_PrintSettings.MinEndOfRun) + " mm";
    ui->ProgramPrints->append(MinEndOfRunString);
}

/******************************************Light Engine Parameters********************************************/
/**
 * @brief MainWindow::on_SetDarkTime_clicked
 * Sets DarkTime variable from the value selected by the user
 */
void MainWindow::on_SetDarkTime_clicked()
{
    m_PrintSettings.DarkTime = (ui->DarkTimeParam->value()*1000);
    QString DarkTimeString = "Set Dark Time to: " + QString::number(m_PrintSettings.DarkTime/1000) + " ms";
    ui->ProgramPrints->append(DarkTimeString);
}

/**
 * @brief MainWindow::on_SetExposureTime_clicked
 * Sets ExposureTime variable from the value selected by the user
 */
void MainWindow::on_SetExposureTime_clicked()
{
    m_PrintSettings.ExposureTime = (ui->ExposureTimeParam->value()*1000);
    QString ExposureTimeString = "Set Exposure Time to: " + QString::number(m_PrintSettings.ExposureTime/1000) + " ms";
    ui->ProgramPrints->append(ExposureTimeString);
}

/**
 * @brief MainWindow::on_SetUVIntensity_clicked
 * Sets UVIntensity from the value selected by the user
 */
void MainWindow::on_SetUVIntensity_clicked()
{
    m_PrintSettings.UVIntensity = (ui->UVIntensityParam->value());
    QString UVIntensityString = "Set UV Intensity to: " + QString::number(m_PrintSettings.UVIntensity);
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
    m_InjectionSettings.InfusionRate = ui->InfuseRateParam->value();
    Pump.SetInfuseRate(m_InjectionSettings.InfusionRate);
    QString InfusionRateString = "Set Infusion Rate to: " + QString::number(m_InjectionSettings.InfusionRate) + " μl/s";
    ui->ProgramPrints->append(InfusionRateString);
}

/**
 * @brief MainWindow::on_SetVolPerLayer_clicked
 */
void MainWindow::on_SetVolPerLayer_clicked()
{
    m_InjectionSettings.InfusionVolume = ui->VolPerLayerParam->value();
    Pump.SetTargetVolume(m_InjectionSettings.InfusionVolume);
    QString InfusionVolumeString = "Set Infusion Volume per layer to: " + QString::number(m_InjectionSettings.InfusionVolume) + " ul";
    ui->ProgramPrints->append(InfusionVolumeString);
}

void MainWindow::on_SetInitialVolume_clicked()
{
    m_InjectionSettings.InitialVolume = ui->InitialVolumeParam->value();
}

void MainWindow::on_PreMovementCheckbox_clicked()
{
    if (ui->PreMovementCheckbox->isChecked() == true){
        ui->PostMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = PRE;
        ui->ProgramPrints->append("Pre-Injection delay enabled");
    }
    else{
        ui->PostMovementCheckbox->setChecked(false);
        ui->PreMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = OFF;
        ui->ProgramPrints->append("Injection delay disabled");
    }
}

void MainWindow::on_PostMovementCheckbox_clicked()
{
    if (ui->PostMovementCheckbox->isChecked() == true){
        ui->PreMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = POST;
        ui->ProgramPrints->append("Post-Injection delay enabled");
    }
    else{
        ui->PostMovementCheckbox->setChecked(false);
        ui->PreMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = OFF;
        ui->ProgramPrints->append("Injection delay disabled");
    }
}

void MainWindow::on_SetInjectionDelay_clicked()
{
    m_InjectionSettings.InjectionDelayParam = ui->InjectionDelayParam->value();
    ui->ProgramPrints->append("Injection delay set to: " + QString::number(m_InjectionSettings.InjectionDelayParam));
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
    ui->ProgramPrints->append("ERROR: " + errMsg);
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
    QString CurrentPosition = Stage.StageGetPosition(m_PrintSettings.StageType);
    CurrentPosition = CurrentPosition.remove(0,3);
    if (CurrentPosition.toInt() > (m_PrintSettings.StartingPosition - 0.1) && CurrentPosition.toInt() < (m_PrintSettings.StartingPosition +0.1))
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
    if (m_PrintSettings.StageType == STAGE_SMC)
    {
        if (GetPosition < (m_PrintSettings.StartingPosition-3.2))
        {
            if (StagePrep1 == false)
            {
                //SMC.SetVelocity(3);
                Stage.SetStageVelocity(3, m_PrintSettings.StageType);
                Sleep(20);
                //SMC.AbsoluteMove((StartingPosition-3));
                Stage.StageAbsoluteMove(m_PrintSettings.StartingPosition-3, m_PrintSettings.StageType);
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

    if (GetPosition > m_PrintSettings.StartingPosition -0.01 && GetPosition < m_PrintSettings.StartingPosition + 0.01)
    {
        verifyStageParams();
    }
    else
    {
        if (StagePrep2 == false)
        {
            Sleep(20);
            //SMC.SetVelocity(0.3);
            Stage.SetStageVelocity(0.3, m_PrintSettings.StageType);
            Sleep(20);
            //SMC.AbsoluteMove(StartingPosition);
            Stage.StageAbsoluteMove(m_PrintSettings.StartingPosition, m_PrintSettings.StageType);
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
    Stage.SetStageAcceleration(m_PrintSettings.StageAcceleration, m_PrintSettings.StageType);
    Sleep(50);
    Stage.SetStageNegativeLimit(m_PrintSettings.MinEndOfRun, m_PrintSettings.StageType);
    Sleep(50);
    Stage.SetStagePositiveLimit(m_PrintSettings.MaxEndOfRun, m_PrintSettings.StageType);
    Sleep(50);
    //SMC.SetVelocity(StageVelocity);
    Stage.SetStageVelocity(m_PrintSettings.StageVelocity, m_PrintSettings.StageType);
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
            m_PrintSettings.ExposureTime = (TotalPrintTime*1000) / SliceCount;
            //ui->ProgramPrints->append("Total Print Time: " + QString::number(TotalPrintTime) + " Slice Count" + QString::number(SliceCount) + "  ExposureTime: " + QString::number(ExposureTime));
            ui->ExposureTimeParam->setValue(m_PrintSettings.ExposureTime);
            ui->ProgramPrints->append("Calculated Exposure Time: " + QString::number(m_PrintSettings.ExposureTime) + " ms");

            m_PrintSettings.LayerThickness = PrintHeight / SliceCount;
            ui->SliceThicknessParam->setValue(m_PrintSettings.LayerThickness);
            ui->ProgramPrints->append("Calculated Slice Thickness: " + QString::number(m_PrintSettings.LayerThickness) + " um");
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

    if(m_PrintSettings.ProjectionMode == POTF){
        DetailedText += "POTF projection mode selected\n";
    }
    else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
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

    DetailedText += "Max Image Upload: " + QString::number(m_PrintSettings.MaxImageUpload) + "images\n";
    DetailedText += "Bit Depth set to: " + QString::number(m_PrintSettings.BitMode) + "\n";
    DetailedText += "Initial Exposure Time: " + QString::number(m_PrintSettings.InitialExposure) + "s\n";
    if (PrinterType == CLIP30UM){
        DetailedText += "Starting Position: " + QString::number(m_PrintSettings.StartingPosition) + " mm\n";
    }
    DetailedText += "Slice Thickness: " + QString::number(m_PrintSettings.LayerThickness*1000) + " μm\n";

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
        DetailedText += "Exposure Time: " + QString::number(m_PrintSettings.ExposureTime/1000) + " ms\n";
        DetailedText += "UV Intensity: " + QString::number(m_PrintSettings.UVIntensity) + "\n";
        DetailedText += "Dark Time " + QString::number(m_PrintSettings.DarkTime/1000) + " ms\n";
    }

    DetailedText += "Stage Velocity: " + QString::number(m_PrintSettings.StageVelocity) + " mm/s\n";
    if (PrinterType == CLIP30UM){
        DetailedText += "Stage Acceleration: " + QString::number(m_PrintSettings.StageAcceleration) + " mm/s^2\n";
        DetailedText += "Max End Of Run: " + QString::number(m_PrintSettings.MaxEndOfRun) + " mm\n";
        DetailedText += "Min End Of Run: " + QString::number(m_PrintSettings.MinEndOfRun) + " mm\n";
    }
    else if (PrinterType == ICLIP){
        if(ContinuousInjection == ON){
            DetailedText += "Continuous injection enabled";
        }
        else{
            DetailedText += "Continuous injection disabled";
        }
        DetailedText += "Infusion volume per layer: " + QString::number(m_InjectionSettings.InfusionVolume) + "ul\n";
        DetailedText += "Infusion rate per layer: " + QString::number(m_InjectionSettings.InfusionRate) + "ul/s";
    }
    DetailedText += " \r\n \r\n";
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
    //Validate m_PrintSettings.LayerThickness
    if (m_PrintSettings.LayerThickness <= 0){
        showError("Invalid Slice Thickness");
        ui->ProgramPrints->append("Invalid Slice Thickness");
        return false;
    }
    //Validate StageVelocity
    else if (m_PrintSettings.StageVelocity <= 0 || m_PrintSettings.StageVelocity > 10){
        showError("Invalid Stage Velocity");
        ui->ProgramPrints->append("Invalid Stage Velocity");
        return false;
    }
    //Validate StageAcceleration
    else if (m_PrintSettings.StageAcceleration <= 0 || m_PrintSettings.StageAcceleration > 10){
        showError("Invalid Stage Acceleration");
        ui->ProgramPrints->append("Invalid Stage Acceleration");
        return false;
    }
    //Validate MaxEndOfRun
    else if (m_PrintSettings.MaxEndOfRun < -5 || m_PrintSettings.MaxEndOfRun > 65 || m_PrintSettings.MinEndOfRun >= m_PrintSettings.MaxEndOfRun){
        showError("Invalid Max End Of Run");
        ui->ProgramPrints->append("Invalid Max End Of Run");
        return false;
    }
    //Validate StageMinEndOfRun
    else if (m_PrintSettings.MinEndOfRun < -5 || m_PrintSettings.MinEndOfRun > 65 || m_PrintSettings.MinEndOfRun >= m_PrintSettings.MaxEndOfRun){
        showError("Invalid Min End Of Run");
        ui->ProgramPrints->append("Invalid Min End Of Run");
        return false;
    }
    //Validate DarkTime
    else if (m_PrintSettings.DarkTime < 0){
        showError("Invalid Dark Time");
        ui->ProgramPrints->append("Invalid Dark Time");
        return false;
    }
    //Validate ExposureTime
    else if (m_PrintSettings.ExposureTime <= 0){
        showError("Invalid Exposure Time");
        ui->ProgramPrints->append("Invalid Exposure Time");
        return false;
    }
    //Validate UVIntensity
    else if (m_PrintSettings.UVIntensity < 1 || m_PrintSettings.UVIntensity > 255){
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
    settings.setValue("ExposureTime", m_PrintSettings.ExposureTime);
    settings.setValue("DarkTime", m_PrintSettings.DarkTime);
    settings.setValue("UVIntensity", m_PrintSettings.UVIntensity);

    settings.setValue("StageVelocity", m_PrintSettings.StageVelocity);
    settings.setValue("StageAcceleration", m_PrintSettings.StageAcceleration);
    settings.setValue("MaxEndOfRun", m_PrintSettings.MaxEndOfRun);
    settings.setValue("MinEndOfRun", m_PrintSettings.MinEndOfRun);

    settings.setValue("m_PrintSettings.LayerThickness", m_PrintSettings.LayerThickness);
    settings.setValue("StartingPosition", m_PrintSettings.StartingPosition);
    settings.setValue("InitialExposure", m_PrintSettings.InitialExposure);
    settings.setValue("PrintSpeed", PrintSpeed);
    settings.setValue("PrintHeight", PrintHeight);

    settings.setValue("m_PrintSettings.MaxImageUpload", m_PrintSettings.MaxImageUpload);

    settings.setValue("LogFileDestination", LogFileDestination);
    settings.setValue("ImageFileDirectory", ImageFileDirectory);

    settings.setValue("PrinterType", PrinterType);
    settings.setValue("StageType", m_PrintSettings.StageType);

    settings.setValue("MotionMode", MotionMode);
    settings.setValue("PumpingMode",PumpingMode);
    settings.setValue("PumpingParameter", PumpingParameter);
    settings.setValue("BitMode", m_PrintSettings.BitMode);

    settings.setValue("ContinuousInjection", ui->ContinuousInjection->isChecked());
    settings.setValue("InfusionRate", m_InjectionSettings.InfusionRate);
    settings.setValue("InfusionVolume", m_InjectionSettings.InfusionVolume);


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
        m_PrintSettings.ExposureTime = settings.value("ExposureTime", 1000).toDouble();
        m_PrintSettings.DarkTime = settings.value("DarkTime", 1000).toDouble();
        m_PrintSettings.UVIntensity = settings.value("UVIntensity", 12).toDouble();

        m_PrintSettings.StageVelocity = settings.value("StageVelocity", 10).toDouble();
        m_PrintSettings.StageAcceleration = settings.value("StageAcceleration", 5).toDouble();
        m_PrintSettings.MaxEndOfRun = settings.value("MaxEndOfRun", 60).toDouble();
        m_PrintSettings.MinEndOfRun = settings.value("MinEndOfRun", 0).toDouble();


        m_PrintSettings.StartingPosition = settings.value("StartingPosition", 5).toDouble();
        m_PrintSettings.InitialExposure = settings.value("InitialExposure", 10).toDouble();
        m_PrintSettings.LayerThickness = settings.value("m_PrintSettings.LayerThickness", 200).toDouble();
        PrintSpeed = settings.value("PrintSpeed", 40).toDouble();
        PrintHeight = settings.value("PrintHeight", 5000).toDouble();

        m_PrintSettings.MaxImageUpload = settings.value("MaxImageUpload", 50).toDouble();

        LogFileDestination = settings.value("LogFileDestination", "C://").toString();
        ImageFileDirectory = settings.value("ImageFileDirectory", "C://").toString();

        PrinterType = settings.value("PrinterType", CLIP30UM).toDouble();

        MotionMode = settings.value("MotionMode", STEPPED).toDouble();
        PumpingMode = settings.value("PumpingMode", 0).toDouble();
        PumpingParameter = settings.value("PumpingParameter", 0).toDouble();
        m_PrintSettings.BitMode = settings.value("BitMode", 1).toDouble();
        m_InjectionSettings.InfusionRate = settings.value("InfusionRate", 5).toDouble();
        m_InjectionSettings.InfusionVolume = settings.value("InfusionVolume", 5).toDouble();

        ContinuousInjection = settings.value("ContinuousInjection", OFF).toInt();
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
    ui->ExposureTimeParam->setValue(m_PrintSettings.ExposureTime/1000);
    ui->DarkTimeParam->setValue(m_PrintSettings.DarkTime/1000);
    ui->UVIntensityParam->setValue(m_PrintSettings.UVIntensity);

    ui->StageVelocityParam->setValue(m_PrintSettings.StageVelocity);
    ui->StageAccelParam->setValue(m_PrintSettings.StageAcceleration);
    ui->MaxEndOfRun->setValue(m_PrintSettings.MaxEndOfRun);
    ui->MinEndOfRunParam->setValue(m_PrintSettings.MinEndOfRun);

    ui->SliceThicknessParam->setValue(m_PrintSettings.LayerThickness*1000);
    ui->StartingPositionParam->setValue(m_PrintSettings.StartingPosition);
    ui->InitialAdhesionParameter->setValue(m_PrintSettings.InitialExposure);
    ui->PrintSpeedParam->setValue(PrintSpeed);
    ui->PrintHeightParam->setValue(PrintHeight);

    ui->MaxImageUpload->setValue(m_PrintSettings.MaxImageUpload);

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
        if(ContinuousInjection == ON){
            ui->ContinuousInjection->setChecked(true);
            emit(on_ContinuousInjection_clicked());
        }
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
    ui->BitDepthParam->setValue(m_PrintSettings.BitMode);
    ui->InfuseRateParam->setValue(m_InjectionSettings.InfusionRate);
    ui->VolPerLayerParam->setValue(m_InjectionSettings.InfusionVolume);
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
        TotalPrintTimeS += nSlice * (m_PrintSettings.DarkTime/(1000*1000));
        TotalPrintTimeS += m_PrintSettings.InitialExposure + 5;
        TotalPrintTimeS += nSlice * 0.1;
        RemainingPrintTimeS = TotalPrintTimeS;
        ui->LivePlot->xAxis->setRange(0, TotalPrintTimeS*1.1);
    }
    else
    {
        ui->LivePlot->xAxis->setRange(0, m_PrintSettings.InitialExposure+5+0.1*nSlice+(1.5*(nSlice*(m_PrintSettings.ExposureTime+m_PrintSettings.DarkTime))/(1000*1000)));
    }
    ui->LivePlot->yAxis->setRange(0.9*(m_PrintSettings.StartingPosition - nSlice*m_PrintSettings.LayerThickness),1.1*m_PrintSettings.StartingPosition);
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

    double CurrentPos = GetPosition;//StartingPosition - (layerCount*m_PrintSettings.LayerThickness);

    qv_x.append(TimeElapsed);
    qv_y.append(CurrentPos);

    ui->LivePlot->graph(0)->setData(qv_x,qv_y);
    ui->LivePlot->clearItems();

    //Update Layer label
    QString Layer = " Layer: " + QString::number(layerCount) + "/" + QString::number(nSlice);
    QCPItemText *textLabel1 = new QCPItemText(ui->LivePlot);
    textLabel1->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    textLabel1->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel1->position->setCoords(0.98, 0.07); // place position at center/top of axis rect
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
        RemainingTime = "Est. Remaining Time: " + QString::number(((m_PrintSettings.ExposureTime+m_PrintSettings.DarkTime)/(1000*1000))*(nSlice-layerCount)+m_PrintSettings.InitialExposure) + "s";
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

void MainWindow::EnableParameter(Parameter_t Parameter, bool State)
{
    switch(Parameter)
    {
        case EXPOSURE_TIME:
            ui->ExposureTimeParam->setEnabled(State);
            ui->SetExposureTime->setEnabled(State);
            ui->ExposureTimeBox->setEnabled(State);
            break;
        case LED_INTENSITY:
            ui->UVIntensityParam->setEnabled(State);
            ui->SetUVIntensity->setEnabled(State);
            ui->UVIntensityBox->setEnabled(State);
            break;
        case DARK_TIME:
            ui->DarkTimeParam->setEnabled(State);
            ui->SetDarkTime->setEnabled(State);
            ui->DarkTimeBox->setEnabled(State);
            break;
        case LAYER_THICKNESS:
            ui->SliceThicknessParam->setEnabled(State);
            ui->SetSliceThickness->setEnabled(State);
            ui->LayerThicknessBox->setEnabled(State);
            break;
        case STAGE_VELOCITY:
            ui->StageVelocityParam->setEnabled(State);
            ui->SetStageVelocity->setEnabled(State);
            ui->StageVelocityBox->setEnabled(State);
            break;
        case STAGE_ACCELERATION:
            ui->StageAccelParam->setEnabled(State);
            ui->SetStageAcceleration->setEnabled(State);
            ui->StageAccelerationBox->setEnabled(State);
            break;
        case PUMP_HEIGHT:
            ui->pumpingParameter->setEnabled(State);
            ui->setPumping->setEnabled(State);
            break;
        case INJECTION_VOLUME:
            ui->VolPerLayerParam->setEnabled(State);
            ui->SetVolPerLayer->setEnabled(State);
            ui->VolPerLayerBox->setEnabled(State);
            break;
        case INJECTION_RATE:
            ui->InfuseRateParam->setEnabled(State);
            ui->SetInfuseRate->setEnabled(State);
            ui->InfusionRateBox->setEnabled(State);
            break;
        case MAX_IMAGE:
            ui->MaxImageUpload->setEnabled(State);
            ui->SetMaxImageUpload->setEnabled(State);
            ui->MaxImageUploadBox->setEnabled(State);
            break;
        case INITIAL_VOLUME:
            ui->InitialVolumeParam->setEnabled(State);
            ui->SetInitialVolume->setEnabled(State);
            ui->InitialVolumeBox->setEnabled(State);
            break;
        case CONTINUOUS_INJECTION:
            ui->ContinuousInjection->setEnabled(State);
            break;
        case STARTING_POSITION:
            ui->StartingPositionParam->setEnabled(State);
            ui->SetStartingPosButton->setEnabled(State);
            ui->StartingPositionBox->setEnabled(State);
            break;
        case MAX_END:
            ui->MaxEndOfRun->setEnabled(State);
            ui->SetMaxEndOfRun->setEnabled(State);
            ui->MaxEndOfRunBox->setEnabled(State);
            break;
        case MIN_END:
            ui->MinEndOfRunParam->setEnabled(State);
            ui->SetMinEndOfRun->setEnabled(State);
            ui->MinEndOfRunBox->setEnabled(State);
            break;
        case INJECTION_DELAY:
            ui->InjectionDelayParam->setEnabled(State);
            ui->SetInjectionDelay->setEnabled(State);
            ui->InjectionDelayBox->setEnabled(State);
            break;
        default:
            break;
    }
}

/**
 * @brief MainWindow::PrintEnd
 */
void MainWindow::PrintComplete()
{
    ui->ProgramPrints->append("Print Complete");
    saveText();
    saveSettings();
    Stage.StageStop(m_PrintSettings.StageType);
    Sleep(50);
    Stage.SetStageVelocity(2, m_PrintSettings.StageType);
    Sleep(50);
    emit(on_GetPosition_clicked());
    Sleep(50);
    if (PrinterType == CLIP30UM){
        if (m_PrintSettings.MinEndOfRun > 0){
            Stage.StageAbsoluteMove(m_PrintSettings.MinEndOfRun, m_PrintSettings.StageType);
        }
        else{
            Stage.StageAbsoluteMove(0, m_PrintSettings.StageType);
        }
    }
    else if(PrinterType == ICLIP){
        Pump.Stop();
    }
}

/**
 * @brief MainWindow::PrintScriptValidate
 * @param layerCount
 * @param Script
 * @return
 * Validating printscript to avoid segmentation faults
 */
bool MainWindow::PrintScriptApply(uint layerCount, QStringList Script, Parameter_t DynamicVar)
{
    bool returnVal = false; //set default to be false
    if (layerCount > 0){
        if (layerCount < Script.size()){
            if (Script.at(layerCount).toDouble() == Script.at(layerCount - 1).toDouble()){
                //do nothing to avoid spamming
            }
            else{
                returnVal = true;
                switch(DynamicVar)
                {
                    case EXPOSURE_TIME:
                        //don't need to account for this atm
                        ui->LiveValue1->setText(QString::number(ExposureScriptList.at(layerCount).toInt()));
                        break;
                    case LED_INTENSITY:
                        LCR_SetLedCurrents(0, 0, (Script.at(layerCount).toInt()));
                        ui->LiveValue2->setText(QString::number(Script.at(layerCount).toInt()));
                        ui->ProgramPrints->append("LED Intensity set to: " + QString::number(Script.at(layerCount).toInt()));
                        break;
                    case DARK_TIME:
                        ui->ProgramPrints->append("Dark Time set to: " + QString::number(Script.at(layerCount).toDouble()));
                        ui->LiveValue3->setText(QString::number(Script.at(layerCount).toInt()));
                        break;
                    case LAYER_THICKNESS:
                        //Currently handled by StageMove() function
                        break;
                    case STAGE_VELOCITY:
                        Stage.SetStageVelocity(Script.at(layerCount).toDouble(), m_PrintSettings.StageType);
                        ui->ProgramPrints->append("New stage velocity set to: " + QString::number(Script.at(layerCount).toDouble()) + " mm/s");
                        Sleep(10); //delay for stage com
                        break;
                    case STAGE_ACCELERATION:
                        Stage.SetStageAcceleration(Script.at(layerCount).toDouble(), m_PrintSettings.StageType);
                        ui->ProgramPrints->append("New stage acceleration set to: " + QString::number(Script.at(layerCount).toDouble()) + " mm/s");
                        Sleep(10); //delay for stage com
                        break;
                    case PUMP_HEIGHT:
                        //currently handled by StageMove function
                        break;
                    case INJECTION_VOLUME:
                        Pump.SetTargetVolume(InjectionVolumeScriptList.at(layerCount).toDouble());
                        ui->LiveValue4->setText(QString::number(InjectionVolumeScriptList.at(layerCount).toDouble()));
                        ui->ProgramPrints->append("Injection Volume set to : " + QString::number(InjectionVolumeScriptList.at(layerCount).toDouble()));
                        break;
                    case INJECTION_RATE:
                        Pump.SetInfuseRate(InjectionRateScriptList.at(layerCount).toDouble());
                        ui->LiveValue5->setText(QString::number(InjectionRateScriptList.at(layerCount).toDouble()));
                        ui->ProgramPrints->append("Injection Rate set to: " + QString::number(InjectionRateScriptList.at(layerCount).toDouble()));
                        break;
                    default:
                        break;
                }
            }
        }
    }
    else{ //If layercount = 0, or first layer
        switch(DynamicVar)
        {
            case INJECTION_VOLUME:
                Pump.SetInfuseRate(InjectionRateScriptList.at(layerCount).toDouble());
                break;
            case INJECTION_RATE:
                Pump.SetTargetVolume(InjectionVolumeScriptList.at(layerCount).toDouble());
                break;
            default:
                break;
        }
    }
    return returnVal;
}

/**
 * @brief MainWindow::StageMove
 *
 */
void MainWindow::StageMove()
{
    //If printscript is active, filter out layerCount = 0 and layerCount is greater than script length
    if (PrintScript == ON){
        if (layerCount > 0){
            if (layerCount < LayerThicknessScriptList.size() && layerCount < StageAccelerationScriptList.size()){
                double LayerThickness = LayerThicknessScriptList.at(layerCount).toDouble()/1000; //grab layer thickness from script list
                ui->ProgramPrints->append("Moving Stage: " + QString::number(LayerThicknessScriptList.at(layerCount).toDouble()) + " um");
                Stage.StageRelativeMove(-LayerThickness, m_PrintSettings.StageType); //Move stage 1 layer thickness

                //If pumping mode is active, grab pump height from script and move stage Pump height - layer thickness
                if (PumpingMode == ON){
                    double PumpParam = PumpHeightScriptList.at(layerCount).toDouble();
                    Stage.StageRelativeMove(PumpParam - LayerThickness, m_PrintSettings.StageType);
                }
            }
        }
    }
    else{
        if (PumpingMode == ON){
            Stage.StageRelativeMove(PumpingParameter - m_PrintSettings.LayerThickness, m_PrintSettings.StageType);
            ui->ProgramPrints->append("Pumping active, moving stage: " + QString::number(PumpingParameter - m_PrintSettings.LayerThickness) + " um");
        }
        else{
            Stage.StageRelativeMove(-m_PrintSettings.LayerThickness, m_PrintSettings.StageType);
            ui->ProgramPrints->append("Moving stage: " + QString::number(m_PrintSettings.LayerThickness) + " um");
        }
    }
}

/**
 * @brief MainWindow::VP8bitWorkaround
 * Used as a workaround for the exposure clipping
 * seen in Video Pattern mode due to Bit Blanking
 */
void MainWindow::VP8bitWorkaround()
{
    VP8Bit = ON;
    QListWidgetItem * item;
    QStringList imageList;
    QStringList ExposureTimeList;
    QStringList DarkTimeList;
    QStringList LEDlist;
    double LayerCount = 0;
    for (int i = 0; i < ui->FileList->count(); i++)
    {
        item = ui->FileList->item(i);
        //if (i > layerCount + MaxImageReupload){
            //break;
        //}
        double ExpTime;
        double DTime;
        if(PrintScript == ON){
            ExpTime = ExposureScriptList.at(i).toDouble();
            DTime = DarkTimeScriptList.at(i).toDouble();
        }
        else{
            ExpTime = m_PrintSettings.ExposureTime;
            DTime= m_PrintSettings.DarkTime;
        }
        bool FrameFinished = false;
        while(!FrameFinished){
            if(ExpTime > 33) //if exposure time > frame time
            {
                ExposureTimeList.append(QString::number(33));
                DarkTimeList.append(QString::number(0));
                LEDlist.append(LEDScriptList.at(i));
                FrameList.append(QString::number(0));
                ExpTime -= 33;
                imageList << item->text();
                LayerCount++;
                ui->ProgramPrints->append("ET: 33, DT: 0, LED: " + LEDScriptList.at(i));
            }
            else{ //frame finished, add remaining exposure time now with dark time
                ExposureTimeList.append(QString::number(ExpTime));
                DarkTimeList.append(QString::number(DTime));
                LEDlist.append(LEDScriptList.at(i));
                FrameList.append(QString::number(1));
                LayerCount++;
                imageList << item->text();
                FrameFinished = true;
                ui->ProgramPrints->append("ET: " + QString::number(ExpTime) + ", DT: " + QString::number(DTime) + ", LED: " + LEDScriptList.at(i));
            }
        }
        //update nSlice
    }
    DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
    ui->ProgramPrints->append("Etime: " + QString::number(ExposureTimeList.count()) + ", Dtime: " + QString::number(DarkTimeList.count()) + ", LEDlist: " + QString::number(LEDlist.count()) + ", Images: " + QString::number(imageList.count()));
}
/*************************************************************
 * ********************Graveyard***************************
 * ***********************************************************/
//Snippets that may be useful in the future but the overall functionality has been deprecated

