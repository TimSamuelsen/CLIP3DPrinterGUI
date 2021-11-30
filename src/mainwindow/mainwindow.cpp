#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <time.h>
#include <windows.h>

#include <Qt>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QProgressDialog>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>
#include <QTextStream>
#include <QInputDialog>
#include <QDesktopServices>
#include <QSettings>
#include <QHeaderView>
#include <QWindow>

#include "API.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stagecommands.h"
#include "pumpcommands.h"
#include "dlp9000.h"
#include "printsettings.h"

DLP9000& DLP = DLP9000::Instance();
StageCommands& Stage = StageCommands::Instance();
PumpCommands& Pump = PumpCommands::Instance();

// Auto parameter selection mode
static double PrintSpeed;
static double PrintHeight;
static bool AutoModeFlag = false;       // true when automode is on, false otherwise

// Settings and log variables
static bool loadSettingsFlag = false;   // For ensuring the settings are only loaded once
QDateTime CurrentDateTime;              // Current time
QString LogFileDestination;             // for storing log file destination in settings
QString ImageFileDirectory;             // For storing image file directory in settings
QString LogName = "CLIPGUITEST";        // For storing the log file nam
QTime PrintStartTime;                   // Get start time for log
int LastUploadTime = 0;
bool PSTableInitFlag = false;


/*!
 * \brief MainWindow::MainWindow
 * \param parent
 * Creates the mainwindow, gets current time for print log,
 * loads and initializes saved settings and initializes plot
 */
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
{
    // Create new window
    ui = (new Ui::MainWindow);
    ui->setupUi(this);

    QObject::connect(&Stage, SIGNAL(StagePrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&Stage, SIGNAL(StageError(QString)), this, SLOT(showError(QString)));
    QObject::connect(&Stage, SIGNAL(StageConnect()), this, SLOT(StageConnected()));
    QObject::connect(&Stage, SIGNAL(StageGetPositionSignal(QString)), this, SLOT(updatePosition(QString)));

    QObject::connect(&DLP, SIGNAL(DLPPrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&DLP, SIGNAL(DLPError(QString)), this, SLOT(showError(QString)));

    QObject::connect(&Pump, SIGNAL(PumpPrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&Pump, SIGNAL(PumpError(QString)), this, SLOT(showError(QString)));
    QObject::connect(&Pump, SIGNAL(PumpConnect()), this, SLOT(PumpConnected()));

    QObject::connect(&PrintControl, SIGNAL(ControlPrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&PrintControl, SIGNAL(ControlError(QString)), this, SLOT(showError(QString)));
    QObject::connect(&PrintControl, SIGNAL(GetPositionSignal()), this, SLOT(on_GetPosition_clicked()));

    //Initialize features
    CurrentDateTime = QDateTime::currentDateTime(); //get current time for startup time
    ui->SettingsWidget->initSettingsPointers(&m_PrintSettings, &m_PrintControls, &m_PrintScript, &m_InjectionSettings);
    loadSettings(); //load settings from settings file
    initSettings(); //initialize settings by updating ui
    PrintControl.getControlPointers(&m_PrintSettings, &m_PrintControls, &m_PrintScript);
    ui->GraphicWindow->initPlot(m_PrintControls, m_PrintSettings, m_PrintScript);
}

/*!
 * \brief MainWindow::~MainWindow
 * Called when the main window is closed
 * Saves settings up
 */
MainWindow::~MainWindow()
{
    saveSettings();     //Save settings upon closing application main window
    delete ui;
}

void MainWindow::initPollTimer()
{
    //QTimer *timer = new QTimer(this);
    //timer->setInterval(100);
    //connect(timer, SIGNAL(timeout()), this, SLOT(on_GetPosition_clicked()));
    //timer->start();
}

/*!
 * \brief MainWindow::on_ManualStage_clicked
 * Open the manual stage control window, sends home
 * command to stage and closes mainwindow connection
 */
void MainWindow::on_ManualStage_clicked()
{
    ManualStageUI = new ManualStageControl();
    ManualStageUI->show();

    // Update Terminal and Stage Connection Indicator
    PrintToTerminal("Manual Stage Control Entered");
}

/*!
 * \brief MainWindow::on_pushButton_clicked
 * Opens the Manual Pump Control window and closes the mainwindow
 * pump connection
 */
void MainWindow::on_ManualPumpControl_clicked()
{
    // Create new manual pump control window
    ManualPumpUI = new manualpumpcontrol();
    ManualPumpUI->show();

    PrintToTerminal("Manual Pump Control Entered"); //Print to terminal
}

/*!
 * \brief MainWindow::on_ImageProcess_clicked
 * Opens the image processing window
 */
void MainWindow::on_ImageProcess_clicked()
{
    // Create new image processing window
    ImageProcessUI = new imageprocessing();
    ImageProcessUI->show();
    PrintToTerminal("Opening Image Processing");
}

void MainWindow::on_FocusCal_clicked()
{
    FocusCalUI = new FocusCal();
    FocusCalUI->show();
    PrintToTerminal("Opening Focus Calibration");
}


/*!
 * \brief MainWindow::on_GetPosition_clicked
 * Gets position, saves it in module level variable GetPosition,
 * updates slider and prints to terminal window
 */
void MainWindow::on_GetPosition_clicked()
{
    QString CurrentPosition = Stage.StageGetPosition(m_PrintSettings.StageType);
    updatePosition(CurrentPosition);

    if (m_PrintSettings.PrinterType == CLIP30UM){
    }
    else if(m_PrintSettings.PrinterType == ICLIP){
    }
}

void MainWindow::updatePosition(QString CurrentPosition)
{
    ui->CurrentPositionIndicator->setText(CurrentPosition);
    PrintToTerminal("Stage is currently at: " + CurrentPosition + " mm");
    //PrintToTerminal("GTest: " + QTime::currentTime().toString("hh.mm.ss.zzz"));
    ui->CurrentStagePos->setSliderPosition(CurrentPosition.toDouble());
    m_PrintControls.StagePosition = CurrentPosition.toDouble();
}
/**********************************Print Process Handling******************************************/
/**
 * @brief MainWindow::on_InitializeAndSynchronize_clicked
 * Prepares the system for printing, moves stage to starting position
 * inits plot, uploads patterns to the light engine, validates parameters
 * for stage and pump
 */
void MainWindow::on_InitializeAndSynchronize_clicked()
{
  // If the confirmation screen was approved by user
  if(ui->SettingsWidget->FileListCount() > 0)
  {
    if (initConfirmationScreen())
    {
        if(m_PrintSettings.ProjectionMode == POTF){
            // n slices = n images
            m_PrintControls.nSlice = ui->SettingsWidget->FileListCount();
            // remaining number of images is max n images - n images used for initial exposure
            m_PrintControls.remainingImages = m_PrintSettings.MaxImageUpload
                                              - m_PrintSettings.InitialExposure;
            QStringList ImageList = GetImageList(m_PrintControls, m_PrintSettings);
            }
        else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
            // n slices = (n bit layers in an image (24) / selected bit depth) * n images
            // i.e. for a 24 bit layer image with a selected bit depth of 4 the
            m_PrintControls.nSlice = (24/m_PrintSettings.BitMode)*ui->SettingsWidget->FileListCount();

            // Grab first image and display in image popout
            QString filename = ui->SettingsWidget->FileListItem(m_PrintControls.layerCount);
            QPixmap img(filename);
            ImagePopoutUI->showImage(img);

            // VP8bit handling
            if (m_PrintSettings.BitMode == 8){
                m_PrintScript.VP8 = ON;
                VP8bitWorkaround();
            }
        }
        else if (m_PrintSettings.ProjectionMode == VIDEO){
            m_PrintControls.nSlice = ui->SettingsWidget->FileListCount(); // n slices = n images
        }
        QStringList ImageList = GetImageList(m_PrintControls, m_PrintSettings);

        // Initialize system hardware
        PrintControl.InitializeSystem(ImageList, m_PrintSettings, &m_PrintControls,
                                      m_PrintScript, m_InjectionSettings);
        emit(on_GetPosition_clicked());     //Sanity check for stage starting position
        ui->GraphicWindow->initPlot(m_PrintControls, m_PrintSettings, m_PrintScript);
        ui->GraphicWindow->updatePlot(m_PrintControls, m_PrintSettings, m_PrintScript);
        ui->StartPrint->setEnabled(true);
    }
  }
  else{
      showError("No image files selected");
  }
}

/**
 * @brief MainWindow::on_StartPrint_clicked
 * If settings are valid, starts print process in hardware and system, records print start time
 */
void MainWindow::on_StartPrint_clicked()
{
    // If settings are validated successfully and initialization has been completed
    if (ValidateSettings() == true){
        PrintToTerminal("Entering Printing Procedure");
        m_PrintControls.PrintStartTime = QTime::currentTime();      // Grabs the current time and saves it

        if(m_PrintScript.PrintScript){
            LCR_SetLedCurrents(0, 0, m_PrintSettings.InitialIntensity);
        }
        ui->GraphicWindow->addInitialExpLabel();
        PrintControl.StartPrint(m_PrintSettings, m_PrintScript,
                                m_InjectionSettings.ContinuousInjection);
        //initPollTimer();
        PrintProcess();
    }
}

/**
 * @brief MainWindow::on_AbortPrint_clicked
 * Aborts print and acts as e-stop. Stops light engine projection,
 * stage movement, injection, and print process
 */
void MainWindow::on_AbortPrint_clicked()
{
    PrintControl.AbortPrint(m_PrintSettings.StageType, &m_PrintControls);   // Stops peripherals
    ui->ProgramPrints->append("PRINT ABORTED");
}

/*!
 * \brief MainWindow::PrintProcess
 *
 */
void MainWindow::PrintProcess()
{
    if(m_PrintControls.layerCount + 1 <= m_PrintControls.nSlice){   // if not at print end
        // Reupload if no more images and not in initial exposure
        if (m_PrintControls.remainingImages <= 0
            && m_PrintControls.InitialExposureFlag == false
            && m_PrintSettings.ProjectionMode != VIDEO){
            QStringList ImageList =  GetImageList(m_PrintControls, m_PrintSettings);
            int imagesUploaded = PrintControl.ReuploadHandler(ImageList, m_PrintControls,
                                                              m_PrintSettings, m_PrintScript,
                                                              m_InjectionSettings.ContinuousInjection);
            m_PrintControls.remainingImages = imagesUploaded;
            PrintToTerminal("Exiting Reupload: " + QTime::currentTime().toString("hh.mm.ss.zzz"));
        }
        if(m_PrintSettings.ProjectionMode == VIDEO){
            if(m_PrintControls.layerCount < ui->SettingsWidget->FileListCount()){
                QTime StartTime = QTime::currentTime();
                QPixmap img(ui->SettingsWidget->FileListItem(m_PrintControls.layerCount));
                ImagePopoutUI->showImage(img);
                LastUploadTime = StartTime.msecsTo(QTime::currentTime());
                PrintToTerminal("Upload: " + QString::number(LastUploadTime) + " ms");
            }
        }
        SetExposureTimer();
        if (m_PrintSettings.PrinterType == ICLIP){
            on_GetPosition_clicked();
        }
        PrintControl.PrintProcessHandler(&m_PrintControls, m_PrintSettings, m_InjectionSettings);
    }
    else{
        PrintComplete();
    }
}

/**
 * @brief MainWindow::pumpingSlot
 * Pumping slot for when pumping is activated,
 * acts as intermediary step between exposure time and dark time
 */
void MainWindow::pumpingSlot(void)
{
    QTimer::singleShot(m_PrintSettings.DarkTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
    PrintControl.StagePumpingHandler(m_PrintControls.layerCount, m_PrintSettings, m_PrintScript);
}

/**
 * @brief MainWindow::ExposureTimeSlot
 * Post exposure time slot, handles dark time actions such as stage movements and injection,
 * updating the plot, handling video pattern frames
 */
void MainWindow::ExposureTimeSlot(void)
{
    if (m_PrintSettings.ProjectionMode == VIDEO
        && m_PrintControls.FrameCount < ui->SettingsWidget->FileListCount())
    {
        PrintToTerminal("LoadTimeImStart: " + QTime::currentTime().toString("hh.mm.ss.zzz"));
        QPixmap img(ui->SettingsWidget->FileListItem(m_PrintControls.FrameCount));
        ImagePopoutUI->showImage(img);
        PrintToTerminal("LoadTimeImEnd: " + QTime::currentTime().toString("hh.mm.ss.zzz"));
    }
    //Set dark timer first for most accurate timing
    SetDarkTimer();
    //Record current time in terminal
    PrintToTerminal("Exp. end: " + QTime::currentTime().toString("hh.mm.ss.zzz"));
    ui->GraphicWindow->updatePlot(m_PrintControls, m_PrintSettings, m_PrintScript);

    //Video pattern mode handling
    if (m_PrintSettings.ProjectionMode == VIDEOPATTERN){ //If in video pattern mode
        if (PrintControl.VPFrameUpdate(&m_PrintControls, m_PrintSettings.BitMode, m_PrintSettings.ResyncVP) == true){
            if (m_PrintControls.FrameCount < ui->SettingsWidget->FileListCount()){ //if not at end of file list
                QPixmap img(ui->SettingsWidget->FileListItem(m_PrintControls.FrameCount)); //select next image
                ImagePopoutUI->showImage(img); //display next image
            }
        }
    }
    else if(m_PrintSettings.ProjectionMode == VIDEO && m_PrintSettings.MotionMode == STEPPED){
        if(m_PrintControls.FrameCount < ui->SettingsWidget->FileListCount()){
            QPixmap img;
            ImagePopoutUI->showImage(img);
        }
    }

    //Print Script handling
    if(m_PrintScript.PrintScript == ON){
        PrintScriptHandler(m_PrintControls, m_PrintSettings, m_PrintScript);
    }

    //Dark time handling
    PrintControl.DarkTimeHandler(m_PrintControls, m_PrintSettings, m_PrintScript, m_InjectionSettings);
}

/**
 * @brief MainWindow::DarkTimeSlot
 * Dummy slot, may be removed
 */
void MainWindow::DarkTimeSlot(void)
{
    if (m_PrintControls.layerCount == 0){
        if (m_PrintScript.PrintScript == ON){
            PrintScriptHandler(m_PrintControls, m_PrintSettings, m_PrintScript);
        }
        else{
            LCR_SetLedCurrents(0, 0, m_PrintSettings.UVIntensity);
        }
    }
    PrintProcess();
    ui->ProgramPrints->append("Dark end: " + QTime::currentTime().toString("hh.mm.ss.zzz"));
    if (m_PrintSettings.PrinterType == ICLIP){
        on_GetPosition_clicked();
    }
}

/*!
 * \brief MainWindow::SetExposureTimer
 * Sets the exposure time and slot called upon timer expiration. The exposure types include:
 * initial exposure, normal exposure, pumped exposure, print script exposure and pumped print script
 * exposure
 */
void MainWindow::SetExposureTimer()
{
  if (m_PrintControls.InitialExposureFlag == 1){
      int InitialExpMs = m_PrintSettings.InitialExposure*1000;
      if(m_PrintSettings.ProjectionMode == ON){ // TODO: why is this like this??
        InitialExpMs += (m_PrintSettings.InitialDelay)*1000;
      }
      QTimer::singleShot(InitialExpMs, Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
  }
  else{
    float ExposureTime = 0;
    switch (m_PrintControls.ExposureType){
      case EXPOSURE_NORM:
        QTimer::singleShot(m_PrintSettings.ExposureTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
        PrintToTerminal("Exposure: " + QString::number(m_PrintSettings.ExposureTime/1000) + " ms");
        break;
      case EXPOSURE_PUMP:
        ExposureTime = m_PrintSettings.ExposureTime/1000;   //Convert from us to ms
        QTimer::singleShot(ExposureTime/1000, Qt::PreciseTimer, this, SLOT(pumpingSlot()));
        PrintToTerminal("Exposure: " + QString::number(ExposureTime/1000) + " ms");
        PrintToTerminal("Preparing for pumping");
        break;
      case EXPOSURE_PS:
        if (m_PrintControls.layerCount < m_PrintScript.ExposureScriptList.count()){
            ExposureTime = m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount).toDouble();
        }
        else{
            ExposureTime = m_PrintScript.ExposureScriptList.at(m_PrintScript.ExposureScriptList.count()-1).toDouble();
        }
        QTimer::singleShot(ExposureTime, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
        PrintToTerminal("Exposure: " + QString::number(ExposureTime) + " ms");
        break;
      case EXPOSURE_PS_PUMP:
        if (m_PrintControls.layerCount < m_PrintScript.ExposureScriptList.count()){
            ExposureTime = m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount).toDouble();
        }
        else{
            ExposureTime = m_PrintScript.ExposureScriptList.at(m_PrintScript.ExposureScriptList.count()-1).toDouble();
        }
        QTimer::singleShot(ExposureTime, Qt::PreciseTimer, this, SLOT(pumpingSlot()));
        PrintToTerminal("Exposure: " + QString::number(ExposureTime) + " ms, preparing for pumping");
        break;
      default:
        break;
    }
  }
}

/*!
 * \brief MainWindow::SetDarkTimer
 * Sets the dark time timer based on motion mode and print script status
 */
void MainWindow::SetDarkTimer()
{
    double DarkTimeSelect = m_PrintSettings.DarkTime/1000;
    if (m_PrintSettings.MotionMode == STEPPED){
        if (m_PrintScript.PrintScript == ON){
            if(m_PrintControls.layerCount < m_PrintScript.DarkTimeScriptList.size()){
                DarkTimeSelect = m_PrintScript.DarkTimeScriptList.at(m_PrintControls.layerCount).toDouble();
            }
            else{
                DarkTimeSelect = m_PrintScript.DarkTimeScriptList.at(m_PrintScript.DarkTimeScriptList.count()-2).toDouble();
            }
        }
        if(m_PrintSettings.ProjectionMode == VIDEO && DarkTimeSelect > 0.5 * LastUploadTime){
            QTimer::singleShot(DarkTimeSelect-LastUploadTime, Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
        }
        else{
            QTimer::singleShot(DarkTimeSelect,  Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
        }
        PrintToTerminal("Dark Time: " + QString::number(DarkTimeSelect) + " ms");

    }
    else{
        if (m_PrintControls.inMotion == false){
            Stage.StageAbsoluteMove(m_PrintControls.PrintEnd, m_PrintSettings.StageType);
        }
        if (m_PrintControls.layerCount == 0 && m_PrintScript.PrintScript == OFF){
            LCR_SetLedCurrents(0, 0, m_PrintSettings.UVIntensity);
        }

        PrintProcess();
    }
}

/***************************************Mode Selection*********************************************/
/*!
 * \brief MainWindow::on_POTFcheckbox_clicked
 * Sets projection mode to POTF
 */
void MainWindow::on_POTFcheckbox_clicked()
{
    //Make sure that the checkbox is checked before proceeding
    if (ui->POTFcheckbox->isChecked())
    {
        ui->VP_HDMIcheckbox->setChecked(false);     //Uncheck the Video Pattern checkbox
        ui->VideoCheckbox->setChecked(false);

        ui->SettingsWidget->EnableParameter(MAX_IMAGE, ON);             //Enable the max image upload parameter
        ui->SettingsWidget->EnableParameter(VP_RESYNC, OFF);
        ui->SettingsWidget->EnableParameter(INITIAL_DELAY, OFF);
        //EnableParameter(DISPLAY_CABLE, OFF);
        m_PrintSettings.ProjectionMode = POTF;      // Set projection mode to POTF
        DLP.setIT6535Mode(0);                       // Turn off HDMI connection
        LCR_SetMode(PTN_MODE_OTF);                  // Set light engine to POTF mode
    }
}

void MainWindow::on_VideoCheckbox_clicked()
{
    if (ui->VideoCheckbox->isChecked()){
        ui->POTFcheckbox->setChecked(false);
        ui->VP_HDMIcheckbox->setChecked(false);
        ui->SettingsWidget->EnableParameter(MAX_IMAGE, OFF);
        ui->SettingsWidget->EnableParameter(VP_RESYNC, OFF);
        //EnableParameter(DISPLAY_CABLE, ON);
        int DisplayCable = ui->DisplayCableList->currentIndex();
        DLP.setIT6535Mode(DisplayCable); // Set IT6535 reciever to correct display cable
        m_PrintSettings.ProjectionMode = VIDEO;
        if(LCR_SetMode(PTN_MODE_DISABLE) < 0){
           PrintToTerminal("Unable to switch to video mode");
           ui->POTFcheckbox->setChecked(true);
           on_POTFcheckbox_clicked();
        }
        else{
            initImagePopout(); // Open projection window
        }
    }
}

/*!
 * \brief MainWindow::on_VP_HDMIcheckbox_clicked
 * Sets projection mode to Video Pattern
 */
void MainWindow::on_VP_HDMIcheckbox_clicked()
{
    // Make sure that the checkbox is checked before proceeding
    if (ui->VP_HDMIcheckbox->isChecked()){
        ui->VideoCheckbox->setChecked(false);
        ui->POTFcheckbox->setChecked(false); // Uncheck the Video Pattern checkbox
        ui->SettingsWidget->EnableParameter(MAX_IMAGE, OFF);
        ui->SettingsWidget->EnableParameter(VP_RESYNC, ON);
        ui->SettingsWidget->EnableParameter(INITIAL_DELAY, ON);
        //EnableParameter(DISPLAY_CABLE, ON);
        m_PrintSettings.ProjectionMode = VIDEOPATTERN; //Set projection mode to video pattern

        // Set video display off
        if (LCR_SetMode(PTN_MODE_DISABLE) < 0){
            PrintToTerminal("Unable to switch to video mode");
           ui->POTFcheckbox->setChecked(true);
            on_POTFcheckbox_clicked();
            // add a close image popout
        }
        else{
            initImagePopout(); // Open projection window
            DLP.setIT6535Mode(1); //Set IT6535 reciever to HDMI input
            Check4VideoLock();
        }
    }
}

/*!
 * \brief MainWindow::Check4VideoLock
 * Validates that the video source is locked, this is needed for Video Pattern Mode
 */
void MainWindow::Check4VideoLock()
{
    QMessageBox VlockPopup; //prep video popout
    static uint8_t repeatCount = 0; //start repeatCount at 0
    unsigned char HWStatus, SysStatus, MainStatus; //Prep variables for LCR_GetStatus

    //If first attempt
    if (repeatCount == 0)
        PrintToTerminal("Attempting to Lock to Video Source");

    //if attempt to get status is successful
    if (LCR_GetStatus(&HWStatus, &SysStatus, &MainStatus) == 0)
    {
        //If BIT3 in main status is set, then video source has already been locked
        if(MainStatus & BIT3){
            PrintToTerminal("External Video Source Locked");
            VlockPopup.close();

            //If attempt to set to Video pattern mode is not successful
            if (LCR_SetMode(PTN_MODE_VIDEO) < 0)
            {
                showError("Unable to switch to video pattern mode");
                ui->VP_HDMIcheckbox->setChecked(false); //Uncheck video pattern mode

                //Reset to POTF mode
                ui->POTFcheckbox->setChecked(true);
                emit(on_POTFcheckbox_clicked());
            }
            PrintToTerminal("Video Pattern Mode Enabled");
            return;
        }
        else{ //Video source is currently not locked
            PrintToTerminal("External Video Source Not Locked, Please Wait");

            //Get current display mode and print to terminal, used for debug
            API_DisplayMode_t testDM;
            LCR_GetMode(&testDM);
            PrintToTerminal(QString::number(testDM));

            //Repeats 15 times (15 seconds) before telling user that an error has occurred
            if(repeatCount < 15)
            {
                //Start 1 second timer, after it expires check if video source is locked
                QTimer::singleShot(1000, this, SLOT(Check4VideoLock()));
                repeatCount++;
            }
            else //System has attempted 15 times to lock to video source
            {
                PrintToTerminal("External Video Source Lock Failed");
                showError("External Video Source Lock Failed");
                ui->VP_HDMIcheckbox->setChecked(false); //Uncheck video pattern mode

                //Reset to POTF mode
                ui->POTFcheckbox->setChecked(true);
                emit(on_POTFcheckbox_clicked());
            }
        }
    }
}

/*!
 * \brief MainWindow::initImagePopout
 * Initializes the image popout window that is used for video pattern mode
 */
void MainWindow::initImagePopout()
{
    if (QGuiApplication::screens().count() > 1){
        ImagePopoutUI = new imagepopout;    // prep new image popout window
        ImagePopoutUI->show();              // Display new window
        ImagePopoutUI->windowHandle()->setScreen(qApp->screens()[1]); //Set window to extended
        ImagePopoutUI->showFullScreen();    // set window to fullscreen

        // Prep image list for video pattern mode
        QStringList imageList;
        // For every file in the file list, enter it into imageList
        for(int i = 0; (i < (ui->SettingsWidget->FileListCount())); i++){
            imageList << ui->SettingsWidget->FileListItem(i);
        }
    }
    else{
        showError("No external displays connected");
    }
}

/*!
 * \brief MainWindow::on_DICLIPSelect_clicked
 * Preps the system for use with iCLIP printer
 */
void MainWindow::on_DICLIPSelect_clicked()
{
    m_PrintSettings.StageType = STAGE_GCODE;    // Set stage to Gcode for the iCLIP printer
    m_PrintSettings.PrinterType = ICLIP;        // Update printer type

    // Enable the injection parameters
    ui->SettingsWidget->EnableParameter(CONTINUOUS_INJECTION, ON);
    ui->SettingsWidget->EnableParameter(INJECTION_VOLUME, ON);
    ui->SettingsWidget->EnableParameter(INJECTION_RATE, ON);
    ui->SettingsWidget->EnableParameter(INITIAL_VOLUME, ON);
    ui->SettingsWidget->EnableParameter(INJECTION_DELAY, ON);

    // Disable the starting position selection
    ui->SettingsWidget->EnableParameter(STARTING_POSITION, OFF);

    // Disable the stage software limits
    ui->SettingsWidget->EnableParameter(MAX_END, OFF);
    ui->SettingsWidget->EnableParameter(MIN_END, OFF);
}

/*!
 * \brief MainWindow::on_CLIPSelect_clicked
 * Preps the system for use with the 30 um printer
 */
void MainWindow::on_CLIPSelect_clicked()
{
    m_PrintSettings.StageType = STAGE_SMC; //Set stage to SMC100CC for the 30 um CLIP printer
    m_PrintSettings.PrinterType = CLIP30UM; //Update printer type

    //Disable the injection parameters
    ui->SettingsWidget->EnableParameter(CONTINUOUS_INJECTION, OFF);
    ui->SettingsWidget->EnableParameter(INJECTION_VOLUME, OFF);
    ui->SettingsWidget->EnableParameter(INJECTION_RATE, OFF);
    ui->SettingsWidget->EnableParameter(INITIAL_VOLUME, OFF);
    ui->SettingsWidget->EnableParameter(INJECTION_DELAY, OFF);

    //Enable the starting position selection
    ui->SettingsWidget->EnableParameter(STARTING_POSITION, ON);

    //Enable the stage software limits
    ui->SettingsWidget->EnableParameter(MAX_END, ON);
    ui->SettingsWidget->EnableParameter(MIN_END, ON);
}

/*********************************************File Handling*********************************************/

/*!
 * \brief MainWindow::on_LogFileBrowse_clicked
 * Select the directory to store log files
 */
void MainWindow::on_LogFileBrowse_clicked()
{
    LogFileDestination = QFileDialog::getExistingDirectory(this, "Open Log File Destination");
    ui->LogFileLocation->setText(LogFileDestination);
}

/*******************************************Peripheral Connections*********************************************/
/*!
 * \brief MainWindow::on_LightEngineConnectButton_clicked
 * Connect to light engine, gets last error code to validate that
 * no errors have occured and connection is working
 */
void MainWindow::on_LightEngineConnectButton_clicked()
{
    if (DLP.InitProjector()){   // if connection was succesful
        PrintToTerminal("Light Engine Connected");
        ui->LightEngineIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->LightEngineIndicator->setText("Connected");
        uint Code;
        //Their GUI does not call reader Errorcode
        if (LCR_ReadErrorCode(&Code) >= 0){
            PrintToTerminal("Last Error Code: " + QString::number(Code));
        }
        else{
            PrintToTerminal("Failed to get last error code");
        }
    }
    else{
        PrintToTerminal("Light Engine Connection Failed");
        ui->LightEngineIndicator->setStyleSheet("background:rgb(255, 0, 0); border: 1px solid black;");
        ui->LightEngineIndicator->setText("Disconnected");
    }
}

/*!
 * \brief MainWindow::on_StageConnectButton_clicked
 * Connects to stage, if successful gets position, sends
 * home commands and gets position
 */
void MainWindow::on_StageConnectButton_clicked()
{
    Stage.StageClose(m_PrintSettings.StageType);
    QString COMSelect = ui->COMPortSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (Stage.StageInit(COM, m_PrintSettings.StageType) == true
        && Stage.StageHome(m_PrintSettings.StageType) == true){
        ui->StageConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0);border: 1px solid black;");
        ui->StageConnectionIndicator->setText("Connected");
        Sleep(10);
        emit(on_GetPosition_clicked());
    }
    else {
        PrintToTerminal("Stage Connection Failed");
        ui->StageConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0);border: 1px solid black;");
        ui->StageConnectionIndicator->setText("Disconnected");
    }
}

void MainWindow::StageConnected()
{
    PrintToTerminal("Stage Connected");
    ui->StageConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0);"
                                                "border: 1px solid black;");
    ui->StageConnectionIndicator->setText("Connected");
    emit(on_GetPosition_clicked());
}

/*!
 * \brief MainWindow::on_PumpConnectButton_clicked
 * Connects to pump
 */
void MainWindow::on_PumpConnectButton_clicked()
{
    QString COMSelect = ui->COMPortSelectPump->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (Pump.PumpInitConnection(COM)){
        ui->PumpConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0);"
                                                   "border: 1px solid black;");
        ui->PumpConnectionIndicator->setText("Connected");
    }
    else{
        PrintToTerminal("Pump Connection Failed");
        ui->PumpConnectionIndicator->setStyleSheet("background:rgb(255, 0, 0);"
                                                   "border: 1px solid black;");
        ui->PumpConnectionIndicator->setText("Disconnected");
    }
}

void MainWindow::PumpConnected()
{
    PrintToTerminal("Pump Connected");
    ui->PumpConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0);"
                                               "border: 1px solid black;");
    ui->PumpConnectionIndicator->setText("Connected");
}
/************************************Print Parameters**********************************************/

/*******************************************Stage Parameters********************************************/

/******************************************Light Engine Parameters********************************************/

/*******************************************Pump Parameters********************************************/

/*******************************************Helper Functions********************************************/
/**
 * @brief MainWindow::showError
 * helper function to show the appropriate API Error message
 * @param errMsg - I - error message to be shown
 */
void MainWindow::showError(QString errMsg)
{
    PrintToTerminal("ERROR: " + errMsg);

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
     QString LogTitle = LogFileDestination + "/" + LogName + "_" + LogDate + "_"
                        + LogTime + ".txt";
     PrintToTerminal(LogTitle);
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
    confScreen.setStyleSheet("QPushButton{ width: 85px; } "
                             "QLabel{font-size: 20px;}"
                             "QTextEdit{font-size: 16px;}");
    confScreen.setText("Please Confirm Print Parameters "); //nospaces

    QString DetailedText;
    DetailedText += LogName + "\n";
    if(m_PrintSettings.PrinterType == CLIP30UM){
        DetailedText += "Printer Type: CLIP 30um\n";
    }
    else if(m_PrintSettings.PrinterType == ICLIP){
        DetailedText += "Printer Type: iCLIP\n";
    }

    DetailedText += "Resin: " + m_PrintSettings.Resin + "\n";

    if(m_PrintSettings.ProjectionMode == POTF){
        DetailedText += "Projection Mode: POTF\n";
    }
    else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        DetailedText += "Projection Mode: Video Pattern\n";
    }
    else if(m_PrintSettings.ProjectionMode == VIDEO){
        DetailedText += "Projection Mode: Video\n";
    }

    if(m_PrintSettings.MotionMode == STEPPED){
        DetailedText += "Motion Mode: stepped\n";
    }
    else if(m_PrintSettings.MotionMode == CONTINUOUS){
        DetailedText += "Motion Mode: continuous\n";
    }

    if(m_PrintSettings.PumpingMode == OFF){
        DetailedText += "Pumping: disabled\n";
    }
    else if(m_PrintSettings.PumpingMode == ON){
        DetailedText += "Pumping: enabled\n";
    }

    if (m_PrintSettings.ProjectionMode == POTF){
        DetailedText += "Max Image Upload: " + QString::number(m_PrintSettings.MaxImageUpload) + "\n";
    }
    else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        DetailedText += "Resync Rate: " + QString::number(m_PrintSettings.ResyncVP) + "\n";
        DetailedText += "Initial Exposure Delay: " + QString::number(m_PrintSettings.InitialDelay) + " s\n";
    }
    else if (m_PrintSettings.ProjectionMode == VIDEO){

    }

    DetailedText += "Bit Depth: " + QString::number(m_PrintSettings.BitMode) + "\n";
    DetailedText += "Initial Exposure Time: " + QString::number(m_PrintSettings.InitialExposure) + " s\n";
    DetailedText += "Initial Exposure Intensity: " + QString::number(m_PrintSettings.InitialIntensity) + "\n";
    if (m_PrintSettings.PrinterType == CLIP30UM){
        DetailedText += "Starting Position: " + QString::number(m_PrintSettings.StartingPosition) + " mm\n";
        DetailedText += "Max End of Run: " + QString::number(m_PrintSettings.MaxEndOfRun) + " mm\n";
        DetailedText += "Min End of Run: " + QString::number(m_PrintSettings.MinEndOfRun) + " mm\n";
    }
    else if (m_PrintSettings.PrinterType == ICLIP){
        if(m_InjectionSettings.InjectionDelayFlag == PRE){
            DetailedText += "Pre-Injection Delay: "
                            + QString::number(m_InjectionSettings.InjectionDelayParam) + " ms\n";
        }
        else if(m_InjectionSettings.InjectionDelayFlag == POST){
            DetailedText += "Post-Injection Delay: "
                            + QString::number(m_InjectionSettings.InjectionDelayParam) + " ms\n";
        }

        if(m_InjectionSettings.ContinuousInjection == ON){
            DetailedText += "Continuous Injection: enabled\n";
            DetailedText += "Base Injection Rate: " + QString::number(m_InjectionSettings.BaseInjectionRate) + " ul/s\n";
        }
        else if (m_InjectionSettings.SteppedContinuousInjection){
            DetailedText += "Continuous Injection: stepped\n";
            DetailedText += "Base Injection Rate: " + QString::number(m_InjectionSettings.BaseInjectionRate) + " ul/s\n";
        }
        else{
            DetailedText += "Continuous Injection: disabled\n";
        }

        DetailedText += "Initial Injection Volume: " + QString::number(m_InjectionSettings.InfusionVolume) + " ul\n";
    }

    if (AutoModeFlag){
        DetailedText += "Auto Mode Active\n";
        DetailedText += "Print Speed: " + QString::number(PrintSpeed) + " um/s\n";
        DetailedText += "Print Height: " + QString::number(PrintHeight) + " um\n";
    }
    else{
        DetailedText += "Auto Mode Not Active\n";
    }
    if (m_PrintScript.PrintScript == 1)
    {
        DetailedText += "Print Script: enabled\n";
        DetailedText += "Exposure Time: print script\n";
        DetailedText += "UV Intensity: print script\n";
        DetailedText += "Dark Time: print script\n";
        DetailedText += "Layer Thickness: print script\n";
        DetailedText += "Stage Velocity: print script\n";
        if (m_PrintSettings.PrinterType == CLIP30UM){
            DetailedText += "Stage Acceleration: print script\n";
        }
        if (m_PrintSettings.PumpingMode == ON){
            DetailedText += "Pump Height: print script\n";
        }
        if(m_PrintSettings.PrinterType == ICLIP){
            DetailedText += "Injection Volume: print script\n";
            DetailedText += "Injection Rate: print script\n";
        }
    }
    else
    {
        DetailedText += "Print Script: disabled\n";
        DetailedText += "Exposure Time: " + QString::number(m_PrintSettings.ExposureTime/1000) + " ms\n";
        DetailedText += "UV Intensity: " + QString::number(m_PrintSettings.UVIntensity) + "\n";
        DetailedText += "Dark Time: " + QString::number(m_PrintSettings.DarkTime/1000) + " ms\n";
        DetailedText += "Layer Thickness: " + QString::number(m_PrintSettings.LayerThickness*1000) + " um\n";
        DetailedText += "Stage Velocity: " + QString::number(m_PrintSettings.StageVelocity) + " mm/s\n";
        if (m_PrintSettings.PrinterType == CLIP30UM){
            DetailedText += "Stage Acceleration: " + QString::number(m_PrintSettings.StageAcceleration) + " mm/s^2\n";
        }
        if (m_PrintSettings.PumpingMode == ON){
            DetailedText += "Pump Height: " + QString::number(m_PrintSettings.PumpingParameter*1000) + " um\n";
        }
        if(m_PrintSettings.PrinterType == ICLIP){
            DetailedText += "Injection Volume: " + QString::number(m_InjectionSettings.InfusionVolume) + " ul\n";
            DetailedText += "Injection Rate: " + QString::number(m_InjectionSettings.InfusionRate) + " ul/s";
        }
    }

    confScreen.setDetailedText(DetailedText);

    confScreen.exec();

    if (confScreen.clickedButton() == cancelButton){
        PrintToTerminal("Print Parameters Not Confirmed");
        PrintToTerminal("Cancelling Print");
    }
    else if (confScreen.clickedButton() == okButton){
        PrintToTerminal("Print Parameters Confirmed");
        PrintToTerminal(DetailedText);
        PrintToTerminal("Proceding to Stage Movement and Image Upload");
        retVal = true;
    }
    else{
        showError("Confirmation Screen Error");
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
    if (m_PrintSettings.InitialExposure < 0 || m_PrintSettings.InitialExposure > 400){
        showError("Invalid initial exposure time");
        PrintToTerminal("InvalidSliceThickness");
    }

    //Validate m_PrintSettings.LayerThickness
    if (m_PrintSettings.LayerThickness <= 0){
        showError("Invalid Slice Thickness");
        PrintToTerminal("Invalid Slice Thickness");
        return false;
    }
    //Validate StageVelocity
    else if (m_PrintSettings.StageVelocity <= 0 || m_PrintSettings.StageVelocity > 10){
        showError("Invalid Stage Velocity");
        PrintToTerminal("Invalid Stage Velocity");
        return false;
    }
    //Validate StageAcceleration
    else if (m_PrintSettings.StageAcceleration <= 0 || m_PrintSettings.StageAcceleration > 10){
        showError("Invalid Stage Acceleration");
        PrintToTerminal("Invalid Stage Acceleration");
        return false;
    }
    //Validate MaxEndOfRun
    else if (m_PrintSettings.MaxEndOfRun < -5 || m_PrintSettings.MaxEndOfRun > 65 || m_PrintSettings.MinEndOfRun >= m_PrintSettings.MaxEndOfRun){
        showError("Invalid Max End Of Run");
        PrintToTerminal("Invalid Max End Of Run");
        return false;
    }
    //Validate StageMinEndOfRun
    else if (m_PrintSettings.MinEndOfRun < -5
             || m_PrintSettings.MinEndOfRun > 65
             || m_PrintSettings.MinEndOfRun >= m_PrintSettings.MaxEndOfRun){
        showError("Invalid Min End Of Run");
        PrintToTerminal("Invalid Min End Of Run");
        return false;
    }
    //Validate DarkTime
    else if (m_PrintSettings.DarkTime < 0){
        showError("Invalid Dark Time");
        PrintToTerminal("Invalid Dark Time");
        return false;
    }
    //Validate ExposureTime
    else if (m_PrintSettings.ExposureTime <= 0){
        showError("Invalid Exposure Time");
        PrintToTerminal("Invalid Exposure Time");
        return false;
    }
    //Validate UVIntensity
    else if (m_PrintSettings.UVIntensity < 0 || m_PrintSettings.UVIntensity > 255){
        showError("Invalid UV Intensity");
        PrintToTerminal("Invalid UVIntensity");
        return false;
    }
    PrintToTerminal("All Settings Valid");
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

    settings.setValue("PrintSpeed", PrintSpeed);
    settings.setValue("PrintHeight", PrintHeight);
    settings.setValue("DisplayCableIndex", ui->DisplayCableList->currentIndex());

    settings.setValue("LogFileDestination", LogFileDestination);
    settings.setValue("ImageFileDirectory", ImageFileDirectory);
    settings.setValue("LogName", LogName);

    settings.setValue("PrinterType", m_PrintSettings.PrinterType);
    settings.setValue("StageType", m_PrintSettings.StageType);

    settings.setValue("StageCOM", ui->COMPortSelect->currentIndex());
    settings.setValue("PumpCOM", ui->COMPortSelectPump->currentIndex());

    ui->SettingsWidget->savePrintSettings();
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

        PrintSpeed = settings.value("PrintSpeed", 40).toDouble();
        PrintHeight = settings.value("PrintHeight", 5000).toDouble();

        ui->DisplayCableList->setCurrentIndex(settings.value("DisplayCableIndex").toInt());

        LogFileDestination = settings.value("LogFileDestination", "C://").toString();
        ImageFileDirectory = settings.value("ImageFileDirectory", "C://").toString();
        LogName = settings.value("LogName", "CLIPGUITEST").toString();

        m_PrintSettings.PrinterType = settings.value("PrinterType", CLIP30UM).toDouble();

        ui->COMPortSelect->setCurrentIndex(settings.value("StageCOM", 0).toInt());
        ui->COMPortSelectPump->setCurrentIndex(settings.value("PumpCOM", 0).toInt());
        loadSettingsFlag = true;

        ui->SettingsWidget->loadPrintSettings();
    }
}

/**
 * @brief MainWindow::initSettings
 * Updates the UI to reflect the settings
 */
void MainWindow::initSettings()
{

    ui->LogFileLocation->setText(LogFileDestination);
    ui->LogName->setText(LogName);

    if(m_PrintSettings.PrinterType == CLIP30UM){
        ui->CLIPSelect->setChecked(true);
        ui->DICLIPSelect->setChecked(false);
        emit(on_CLIPSelect_clicked());
    }
    else if(m_PrintSettings.PrinterType == ICLIP){
        ui->DICLIPSelect->setChecked(true);
        ui->CLIPSelect->setChecked(false);
        on_DICLIPSelect_clicked();
    }

    ui->SettingsWidget->EnableParameter(VP_RESYNC, OFF); // Always starts in POTF so ResyncVP is disabled from start
    ui->SettingsWidget->EnableParameter(INITIAL_DELAY, OFF); // Always starts in POTF so Initial delay is disabled
    ui->SettingsWidget->initPrintSettings();
}

/*************************************************************
 * ********************Development***************************
 * ***********************************************************/
/**
 * @brief MainWindow::PrintEnd
 */
void MainWindow::PrintComplete()
{
    PrintToTerminal("Print Complete");
    LCR_SetMode(PTN_MODE_OTF);   // set to POTF to turn off any projection
    saveText();
    saveSettings();
    Stage.StageStop(m_PrintSettings.StageType);
    Sleep(50);
    Stage.SetStageVelocity(2, m_PrintSettings.StageType);
    Sleep(50);
    emit(on_GetPosition_clicked());
    Sleep(50);
    if (m_PrintSettings.PrinterType == CLIP30UM){
        if (m_PrintSettings.MinEndOfRun > 0){
            Stage.StageAbsoluteMove(m_PrintSettings.MinEndOfRun, m_PrintSettings.StageType);
        }
        else{
            Stage.StageAbsoluteMove(0, m_PrintSettings.StageType);
        }
    }
    else if(m_PrintSettings.PrinterType == ICLIP){
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
                if (layerCount == 1 && DynamicVar == LED_INTENSITY){
                    LCR_SetLedCurrents(0, 0, Script.at(0).toInt());
                }
            }
            else{
                returnVal = true;
                switch(DynamicVar)
                {
                    case EXPOSURE_TIME:
                        //don't need to account for this atm
                        ui->LiveValue1->setText(QString::number(m_PrintScript.ExposureScriptList.at(layerCount).toInt()));
                        break;
                    case LED_INTENSITY:
                        LCR_SetLedCurrents(0, 0, (Script.at(layerCount).toInt()));
                        ui->LiveValue2->setText(QString::number(Script.at(layerCount).toInt()));
                        PrintToTerminal("LED Intensity set to: " + QString::number(Script.at(layerCount).toInt()));
                        break;
                    case DARK_TIME:
                        PrintToTerminal("Dark Time set to: " + QString::number(Script.at(layerCount).toDouble()));
                        ui->LiveValue3->setText(QString::number(Script.at(layerCount).toInt()));
                        break;
                    case LAYER_THICKNESS:
                        //Currently handled by StageMove() function
                        break;
                    case STAGE_VELOCITY:
                        Stage.SetStageVelocity(Script.at(layerCount).toDouble(), m_PrintSettings.StageType);
                        PrintToTerminal("New stage velocity set to: " + QString::number(Script.at(layerCount).toDouble()) + " mm/s");
                        Sleep(10); //delay for stage com
                        break;
                    case STAGE_ACCELERATION:
                        Stage.SetStageAcceleration(Script.at(layerCount).toDouble(), m_PrintSettings.StageType);
                        PrintToTerminal("New stage acceleration set to: " + QString::number(Script.at(layerCount).toDouble()) + " mm/s");
                        Sleep(10); //delay for stage com
                        break;
                    case PUMP_HEIGHT:
                        //currently handled by StageMove function
                        break;
                    case INJECTION_VOLUME:
                        if (m_InjectionSettings.ContinuousInjection == OFF && m_InjectionSettings.SteppedContinuousInjection == OFF){
                            Pump.SetTargetVolume(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble());
                            ui->LiveValue4->setText(QString::number(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble()));
                            PrintToTerminal("Injection Volume set to : " + QString::number(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble()));
                        }
                        break;
                    case INJECTION_RATE:
                        if (m_InjectionSettings.SteppedContinuousInjection == ON || m_InjectionSettings.ContinuousInjection){
                            m_InjectionSettings.InfusionRate = m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble();
                        }
                        else{
                            Pump.SetInfuseRate(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble());
                            PrintToTerminal("Injection Rate set to: " + QString::number(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble()));
                        }
                        ui->LiveValue5->setText(QString::number(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble()));
                        break;
                    default:
                        break;
                }
            }
        }
    }
    else{ //If layercount = 0, or first layer
        switch(DynamicVar) // TO DO: Fix this mess
        {
            case EXPOSURE_TIME:
                //don't need to account for this atm
                ui->LiveValue1->setText(QString::number(m_PrintScript.ExposureScriptList.at(layerCount).toInt()));
                break;
            case LED_INTENSITY:
                LCR_SetLedCurrents(0, 0, (Script.at(layerCount).toInt()));
                ui->LiveValue2->setText(QString::number(Script.at(layerCount).toInt()));
                PrintToTerminal("LED Intensity set to: " + QString::number(Script.at(layerCount).toInt()));
                break;
            case DARK_TIME:
                PrintToTerminal("Dark Time set to: " + QString::number(Script.at(layerCount).toDouble()));
                ui->LiveValue3->setText(QString::number(Script.at(layerCount).toInt()));
                break;
            case LAYER_THICKNESS:
                //Currently handled by StageMove() function
                break;
            case STAGE_VELOCITY:
                Stage.SetStageVelocity(Script.at(layerCount).toDouble(), m_PrintSettings.StageType);
                PrintToTerminal("New stage velocity set to: " + QString::number(Script.at(layerCount).toDouble()) + " mm/s");
                Sleep(10); //delay for stage com
                break;
            case STAGE_ACCELERATION:
                Stage.SetStageAcceleration(Script.at(layerCount).toDouble(), m_PrintSettings.StageType);
                PrintToTerminal("New stage acceleration set to: " + QString::number(Script.at(layerCount).toDouble()) + " mm/s");
                Sleep(10); //delay for stage com
                break;
            case PUMP_HEIGHT:
                //currently handled by StageMove function
                break;
            case INJECTION_VOLUME:
                if (m_InjectionSettings.ContinuousInjection == OFF && m_InjectionSettings.SteppedContinuousInjection == OFF){
                    Pump.SetTargetVolume(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble());
                    ui->LiveValue4->setText(QString::number(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble()));
                    PrintToTerminal("Injection Volume set to : " + QString::number(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble()));
                }
                break;
            case INJECTION_RATE:
                if (m_InjectionSettings.SteppedContinuousInjection == ON || m_InjectionSettings.ContinuousInjection){
                    m_InjectionSettings.InfusionRate = m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble();
                }
                else{
                    Pump.SetInfuseRate(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble());
                    PrintToTerminal("Injection Rate set to: " + QString::number(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble()));
                }
                ui->LiveValue5->setText(QString::number(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble()));
                break;
            default:
                break;
        }
    }
    return returnVal;
}

/**
 * @brief MainWindow::VP8bitWorkaround
 * Used as a workaround for the exposure clipping
 * seen in Video Pattern mode due to Bit Blanking
 */
void MainWindow::VP8bitWorkaround()
{
    if (m_PrintScript.PrintScript == ON){
        for (uint i = 0; i < m_PrintControls.nSlice; i++)
        {
            double ExpTime = m_PrintScript.ExposureScriptList.at(i).toDouble();
            double DTime = m_PrintScript.ExposureScriptList.at(i).toDouble();
            bool FrameFinished = false;
            while(!FrameFinished){
                if(ExpTime > 33){ //if exposure time > frame time
                    m_PrintScript.VP8_ExpList.append(QString::number(33));
                    m_PrintScript.VP8_DarkList.append(QString::number(0));
                    ExpTime -= 33;
                    PrintToTerminal("ET: 33, DT: 0");
                }
                else{ //frame finished, add remaining exposure time now with dark time
                    m_PrintScript.VP8_ExpList.append(QString::number(ExpTime));
                    m_PrintScript.VP8_DarkList.append(QString::number(DTime));
                    FrameFinished = true;
                    PrintToTerminal("ET: " + QString::number(ExpTime) + ", DT: " + QString::number(DTime));
                }
            }
        }
    }
    else{
        showError("VP-8bit for exp. times above 33ms must use a print script");
    }
}

/*!
 * \brief MainWindow::PrintToTerminal
 * Handler function for printing to terminal.
 * \param StringToPrint
 *
 */
void MainWindow::PrintToTerminal(QString StringToPrint)
{
    ui->ProgramPrints->append(StringToPrint);
}

void MainWindow::PrintScriptHandler(PrintControls m_PrintControls, PrintSettings m_PrintSettings, PrintScripts m_PrintScript){
    PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.ExposureScriptList, EXPOSURE_TIME);
    PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.LEDScriptList, LED_INTENSITY);
    PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.DarkTimeScriptList, DARK_TIME);
    PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.LayerThicknessScriptList, LAYER_THICKNESS);
    PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.StageVelocityScriptList, STAGE_VELOCITY);
    PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.StageAccelerationScriptList, STAGE_ACCELERATION);
    if (m_PrintSettings.PumpingMode == ON){
        PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.PumpHeightScriptList, PUMP_HEIGHT);
    }
    if (m_PrintSettings.PrinterType == ICLIP){
        PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.InjectionVolumeScriptList, INJECTION_VOLUME);
        PrintScriptApply(m_PrintControls.layerCount, m_PrintScript.InjectionRateScriptList, INJECTION_RATE);
    }
}

QStringList MainWindow::GetImageList(PrintControls m_PrintControls, PrintSettings m_PrintSettings)
{
    QStringList ImageList;
    QString item;
    double InitialExposureCount = m_PrintSettings.InitialExposure;
    if (m_PrintControls.InitialExposureFlag == ON
        && m_PrintSettings.ProjectionMode != VIDEO){ //If in initial POTF upload
        m_PrintControls.nSlice = ui->SettingsWidget->FileListCount();
        if (m_PrintControls.nSlice){
            item = ui->SettingsWidget->FileListItem(0);
            while (InitialExposureCount > 0){
                ImageList << item;
                InitialExposureCount -= 5;
            }
            if (m_PrintSettings.ProjectionMode == POTF){
                for (int i = 1; i < ui->SettingsWidget->FileListCount(); i++){
                    item = ui->SettingsWidget->FileListItem(i);
                    ImageList << item;
                    if ((i + m_PrintSettings.InitialExposure) > m_PrintSettings.MaxImageUpload){
                            break;
                        }
                }
            }
        }
    }
    else if (m_PrintSettings.ProjectionMode == POTF){
        //else in re-upload
        for (int i = m_PrintControls.layerCount; i< ui->SettingsWidget->FileListCount(); i++){
            item = ui->SettingsWidget->FileListItem(i);
            ImageList << item;
            if (i > m_PrintControls.layerCount + m_PrintSettings.MaxImageUpload){
                break;
            }
        }
    }
    else if (m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        int j = 0;
        int nUploadFrames = (m_PrintSettings.ResyncVP/24)*m_PrintSettings.BitMode;
        if (m_PrintScript.VP8 == OFF){
            for(int i = m_PrintControls.FrameCount; i < m_PrintControls.FrameCount + nUploadFrames; i++)
            {
                for (int j = 0; j < (24/m_PrintSettings.BitMode); j++)
                {
                    if (i < ui->SettingsWidget->FileListCount()){
                        item = ui->SettingsWidget->FileListItem(i);
                        ImageList << item;
                    }
                    else{
                        ui->ProgramPrints->append("VP Image segmentation fault");
                        break;
                    }
                }
                ui->ProgramPrints->append(QString::number(j) + " patterns uploaded");
            }
        }
        else{ // VP8Bit is live
            for(int i = m_PrintControls.FrameCount; i < m_PrintControls.FrameCount + nUploadFrames; i++){
                int nCount = 0;
                while (nCount < (24/m_PrintSettings.BitMode)){
                    if (i < ui->SettingsWidget->FileListCount()){
                        item = ui->SettingsWidget->FileListItem(0);
                        ImageList << item;
                        if (m_PrintScript.VP8_DarkList.at(m_PrintScript.nPat).toInt() != 0){
                            nCount++;
                        }
                        m_PrintScript.nPat++;
                    }
                    else{
                        nCount = 24/m_PrintSettings.BitMode; // break while loop
                    }
                }
            }
        }
    }
    for (int i = 0; i < ImageList.count(); i++){
        PrintToTerminal(ImageList.at(i));
    }
    return ImageList;
}

void MainWindow::on_LogName_textChanged()
{
    LogName = ui->LogName->toPlainText();
}


void MainWindow::refreshScript()
{
    if (m_PrintScript.ExposureScriptList.size() > 0)
    {
        ui->LiveValueList1->setCurrentIndex(1);
        ui->LiveValue1->setText(QString::number(m_PrintScript.ExposureScriptList.at(0).toInt()));
        ui->LiveValueList2->setCurrentIndex(2);
        ui->LiveValue2->setText(QString::number(m_PrintScript.LEDScriptList.at(0).toInt()));
        ui->LiveValueList3->setCurrentIndex(3);
        ui->LiveValue3->setText(QString::number(m_PrintScript.DarkTimeScriptList.at(0).toInt()));
        if(m_PrintSettings.PrinterType == ICLIP){
            if (m_PrintScript.InjectionVolumeScriptList.size() > 0 && m_PrintScript.InjectionRateScriptList.size() > 0){
                PrintToTerminal("Print List has: " + QString::number(m_PrintScript.InjectionVolumeScriptList.size()) + " injection volume entries");
                PrintToTerminal("Print List has: " + QString::number(m_PrintScript.InjectionRateScriptList.size()) + " injection rate entries");
                ui->LiveValueList4->setCurrentIndex(4);
                ui->LiveValue4->setText(QString::number(m_PrintScript.InjectionVolumeScriptList.at(0).toDouble()));
                ui->LiveValueList5->setCurrentIndex(5);
                ui->LiveValue5->setText(QString::number(m_PrintScript.InjectionRateScriptList.at(0).toDouble()));
            }
        }
    }

    ui->GraphicWindow->initPrintScriptTable(m_PrintSettings, &m_PrintScript);
}

void MainWindow::on_resetButton_clicked()
{
    if (initResetConfirmation() == true){
        saveSettings();
        Sleep(20);

        // Clear all data structures (how do?)
        PrintControls tempControls;
        m_PrintControls = tempControls;
        // Clear terminal
        ui->ProgramPrints->clear();


        // Re initialize features
        CurrentDateTime = QDateTime::currentDateTime();
        loadSettings();
        initSettings();
        ui->GraphicWindow->initPlot(m_PrintControls, m_PrintSettings, m_PrintScript);
        PrintToTerminal("Reset Successful");
    }
}

bool MainWindow::initResetConfirmation()
{
    bool retVal = false;
    QMessageBox confScreen;
    QPushButton *cancelButton = confScreen.addButton(QMessageBox::Cancel);
    QPushButton *okButton = confScreen.addButton(QMessageBox::Ok);
    confScreen.setText("Please Confirm Reset Request");
    confScreen.exec();
    if (confScreen.clickedButton() == cancelButton){
        PrintToTerminal("Reset Cancelled");
    }
    else if (confScreen.clickedButton() == okButton){
        retVal = true;
        PrintToTerminal("Reset Confirmed");
    }
    else{
        PrintToTerminal("Reset Conf. Screen Error");
    }
    return retVal;
}


