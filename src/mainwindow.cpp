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

#include "API.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "stagecommands.h"
#include "pumpcommands.h"
#include "dlp9000.h"

DLP9000& DLP = DLP9000::Instance();
StageCommands& Stage = StageCommands::Instance();
PumpCommands& Pump = PumpCommands::Instance();

static QStringList ImageList;
//Auto parameter selection mode
static double PrintSpeed;
static double PrintHeight;
static bool AutoModeFlag = false; //true when automode is on, false otherwise

//Settings and log variables
static bool loadSettingsFlag = false; //For ensuring the settings are only loaded once, (May not be needed)
QDateTime CurrentDateTime; //Current time
QString LogFileDestination; //for storing log file destination in settings
QString ImageFileDirectory; //For storing image file directory in settings
QTime PrintStartTime; //Get start time for log
//static double GetPosition; //Holds current position

//For vp8bit workaround
static int VP8Bit = OFF;
QStringList FrameList;

/*!
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

    //
    //ManualStageControl * pStage = new ManualStageControl;
    QObject::connect(&Stage, SIGNAL(StagePrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&Stage, SIGNAL(StageError(QString)), this, SLOT(showError(QString)));

    QObject::connect(&DLP, SIGNAL(DLPPrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&DLP, SIGNAL(DLPError(QString)), this, SLOT(showError(QString)));

    QObject::connect(&Pump, SIGNAL(PumpPrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&Pump, SIGNAL(PumpError(QString)), this, SLOT(showError(QString)));

    QObject::connect(&PrintControl, SIGNAL(ControlPrintSignal(QString)), this, SLOT(PrintToTerminal(QString)));
    QObject::connect(&PrintControl, SIGNAL(ControlError(QString)), this, SLOT(showError(QString)));
    QObject::connect(&PrintControl, SIGNAL(GetPositionSignal()), this, SLOT(on_GetPosition_clicked()));
    QObject::connect(&PrintControl, SIGNAL(UpdatePlotSignal()), this, SLOT(updatePlot()));

    //Initialize features
    CurrentDateTime = QDateTime::currentDateTime(); //get current time for startup time
    loadSettings(); //load settings from settings file
    initSettings(); //initialize settings by updating ui
    initPlot(); //initiallize the plot window
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
    ManualStageUI = new ManualStageControl();
    ManualStageUI->show();

    //Initialize stage for manual stage control
    //Home command is issued to make sure the stage is referenced upon startup
    Stage.StageHome(m_PrintSettings.StageType);
    //Close main window connection to allow manual stage control to take over, ideally this would not be needed and it would stay on the same connection.
    Stage.StageClose(m_PrintSettings.StageType);

    //Update Terminal and Stage Connection Indicator
    PrintToTerminal("Manual Stage Control Entered"); //Print to terminal
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

    PrintToTerminal("Manual Pump Control Entered"); //Print to terminal
    Pump.PumpSerial.closeDevice(); //Closes main window pump connection to allow manual pump control to take over
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
    PrintToTerminal("Opening Image Processing");
}

/**
 * @brief MainWindow::on_GetPosition_clicked
 * Gets position, saves it in module level variable GetPosition,
 * updates slider and prints to terminal window
 */
void MainWindow::on_GetPosition_clicked()
{
    //Get position and store it in char*, could store directly in QString but if result is null it can cause issues
    QString CurrentPosition = Stage.StageGetPosition(m_PrintSettings.StageType);

    //If the string length returned from SMC.GetPosition is > 1, then no errors have occurred
    //if (strnlen(ReadPosition,50) > 1)
    //{
        //QString CurrentPosition = QString::fromUtf8(ReadPosition); //convert char* to QString
        if (m_PrintSettings.PrinterType == CLIP30UM){
            CurrentPosition.remove(0,3); //Removes address and command code
            CurrentPosition.chop(2); //To be removed...
        }
        else if(m_PrintSettings.PrinterType == ICLIP){

        }
        ui->CurrentPositionIndicator->setText(CurrentPosition); //Update current position indicator
        PrintToTerminal("Stage is currently at: " + CurrentPosition + " mm"); //Print to terminal
        ui->CurrentStagePos->setSliderPosition(CurrentPosition.toDouble()); //Update slider position
        GetPosition = CurrentPosition.toDouble(); //Update module variable GetPosition (to be accessed from other functions)
    //}
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
        PrintToTerminal("Attempting to Lock to Video Source");

    //if attempt to get status is succesful
    if (LCR_GetStatus(&HWStatus, &SysStatus, &MainStatus) == 0)
    {
        //If BIT3 in main status is set, then video source has already been locked
        if(MainStatus & BIT3){
            PrintToTerminal("External Video Source Locked");
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
            else //System has attempted 15 times to lock to video source, and attempt is determined to be a failure
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
    m_PrintSettings.PrinterType = ICLIP; //Update printer type

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
    m_PrintSettings.PrinterType = CLIP30UM; //Update printer type

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
    PrintToTerminal("Bit-Depth set to: " + QString::number(m_PrintSettings.BitMode));
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
        m_PrintSettings.MotionMode = STEPPED; //Set MotionMode to STEPPED
        ui->ContinuousMotion->setChecked(false); //Updates UI to reflect stepped mode
        PrintToTerminal("Stepped Motion Selected");
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
        m_PrintSettings.MotionMode = CONTINUOUS; //Set MotionMode to Continuous
        ui->SteppedMotion->setChecked(false); //Update UI to reflect continuous mode
        PrintToTerminal("Continuous Motion Selected");

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
        m_PrintSettings.PumpingMode = ON; //Enable pumping mode

        //Update UI
        EnableParameter(PUMP_HEIGHT, ON);
        PrintToTerminal("Pumping Enabled");
    }
    else{
        m_PrintSettings.PumpingMode = OFF; //Disable pumping mode

        //Update UI
        EnableParameter(PUMP_HEIGHT, OFF);
        PrintToTerminal("Pumping Disabled");
    }
}

/**
 * @brief MainWindow::on_setPumping_clicked
 * Sets the pumping depth parameter
 */
void MainWindow::on_setPumping_clicked()
{
    //If pumping is currently enabled
    if(m_PrintSettings.PumpingMode == ON){
        m_PrintSettings.PumpingParameter = ui->pumpingParameter->value()/1000; //Grab value from UI and divide by 1000 to scale units
        PrintToTerminal("Pumping depth set to: " + QString::number(m_PrintSettings.PumpingParameter*1000) + " μm");
    }
    else{ //Pumping is currently disabled
        PrintToTerminal("Please enable pumping before setting pumping parameter");
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
        PrintToTerminal("Please select more images");
    }
    int SliceCount = ui->FileList->count();
    PrintToTerminal(QString::number(SliceCount) + " Images Currently Selected");
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
        m_PrintScript.PrintScript = 1;
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
        m_PrintScript.PrintScript = 0;
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
            m_PrintScript.ExposureScriptList.append(line.split(',').at(0));
            m_PrintScript.LEDScriptList.append(line.split(',').at(1));
            m_PrintScript.DarkTimeScriptList.append(line.split(',').at(2));
            m_PrintScript.LayerThicknessScriptList.append(line.split(',').at(3));
            m_PrintScript.StageVelocityScriptList.append(line.split(',').at(4));
            m_PrintScript.StageAccelerationScriptList.append(line.split(',').at(5));
            m_PrintScript.PumpHeightScriptList.append(line.split(',').at(6));
            if(m_PrintSettings.PrinterType == ICLIP){
                m_PrintScript.InjectionVolumeScriptList.append(line.split(',').at(7));
                m_PrintScript.InjectionRateScriptList.append(line.split(',').at(8));
            }
    }
     //For testing
    for (int i= 0; i < m_PrintScript.ExposureScriptList.size(); i++)
    {
        if(m_PrintSettings.PrinterType == ICLIP){
            PrintToTerminal(m_PrintScript.ExposureScriptList.at(i) + "," + m_PrintScript.LEDScriptList.at(i) + "," + m_PrintScript.DarkTimeScriptList.at(i) + "," + m_PrintScript.LayerThicknessScriptList.at(i) + "," + m_PrintScript.StageVelocityScriptList.at(i)
                                      + "," + m_PrintScript.StageAccelerationScriptList.at(i) + "," + m_PrintScript.PumpHeightScriptList.at(i) + "," + m_PrintScript.InjectionVolumeScriptList.at(i) + "," + m_PrintScript.InjectionRateScriptList.at(i));
        }
        else{
            PrintToTerminal(m_PrintScript.ExposureScriptList.at(i) + "," + m_PrintScript.LEDScriptList.at(i) + "," + m_PrintScript.DarkTimeScriptList.at(i) + "," + m_PrintScript.LayerThicknessScriptList.at(i) + "," + m_PrintScript.StageVelocityScriptList.at(i)
                                      + "," + m_PrintScript.StageAccelerationScriptList.at(i) + "," + m_PrintScript.PumpHeightScriptList.at(i));
        }
    }
    PrintToTerminal("Print List has: " + QString::number(m_PrintScript.ExposureScriptList.size()) + " exposure time entries");
    PrintToTerminal("Print List has: " + QString::number(m_PrintScript.LEDScriptList.size()) + " LED intensity entries");
    PrintToTerminal("Print List has: " + QString::number(m_PrintScript.DarkTimeScriptList.size()) + " dark time entries");
    PrintToTerminal("Print List has: " + QString::number(m_PrintScript.LayerThicknessScriptList.size()) + " layer thickness entries");
    PrintToTerminal("Print List has: " + QString::number(m_PrintScript.StageVelocityScriptList.size()) + " stage velocity entries");
    PrintToTerminal("Print List has: " + QString::number(m_PrintScript.StageAccelerationScriptList.size()) + " stage acceleration entries");
    PrintToTerminal("Print List has: " + QString::number(m_PrintScript.PumpHeightScriptList.size()) + + " pump height entries");

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
}

/**
 * @brief MainWindow::on_ClearPrintScript_clicked
 * Clears selected print list
 */
void MainWindow::on_ClearPrintScript_clicked()
{
    ui->PrintScriptFile->clear();
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
        PrintToTerminal("Light Engine Connected");
        ui->LightEngineIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->LightEngineIndicator->setText("Connected");
        uint Code;
        //Their GUI does not call reader Errorcode
        if (LCR_ReadErrorCode(&Code) >= 0)
        {
            PrintToTerminal("Last Error Code: " + QString::number(Code));
        }
        else
        {
            PrintToTerminal("Failed to get last error code");
        }
    }
    else
    {
        PrintToTerminal("Light Engine Connection Failed");
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
    Stage.StageClose(m_PrintSettings.StageType);
    QString COMSelect = ui->COMPortSelect->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (Stage.StageInit(COM, m_PrintSettings.StageType) == true && Stage.StageHome(m_PrintSettings.StageType) == true)
    {
        PrintToTerminal("Stage Connected");
        ui->StageConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->StageConnectionIndicator->setText("Connected");
        Sleep(10);
        emit(on_GetPosition_clicked());
    }
    else
    {
        PrintToTerminal("Stage Connection Failed");
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
    Pump.PumpSerial.closeDevice();
    QString COMSelect = ui->COMPortSelectPump->currentText();
    QByteArray array = COMSelect.toLocal8Bit();
    char* COM = array.data();
    if (Pump.PumpSerial.openDevice(COM,9600) == 1)
    {
        PrintToTerminal("Pump Connected");
        ui->PumpConnectionIndicator->setStyleSheet("background:rgb(0, 255, 0); border: 1px solid black;");
        ui->PumpConnectionIndicator->setText("Connected");
    }
    else
    {
        PrintToTerminal("Pump Connection Failed");
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
    PrintToTerminal(arg1 + " Selected");
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
            PrintToTerminal("No Object Image Files Detected, Please Select Image Files First");
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
    if (SliceCount < 1){
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
    else if (MaxImageVal > 395) //Verifies that max image upload is not greater than the storage limit on light engine
    {
        MaxImageVal = 395;
    }
    else //If MaxImageVal is is an acceptable range
    {
        m_PrintSettings.MaxImageUpload = MaxImageVal; //This line might not be needed
    }
    m_PrintSettings.MaxImageUpload = MaxImageVal;
    QString MaxImageUploadString = "Set Max Image Upload to: " + QString::number(m_PrintSettings.MaxImageUpload);
    PrintToTerminal(MaxImageUploadString);
}


/**
 * @brief MainWindow::on_setPrintSpeed_clicked
 * Sets PrintSpeed variable from value inputted by user, also triggers automode
 */
void MainWindow::on_setPrintSpeed_clicked()
{
    PrintSpeed = (ui->PrintSpeedParam->value());
    PrintToTerminal("Set Print Speed to: " + QString::number(PrintSpeed) + " um/s");
    AutoMode();
}

/**
 * @brief MainWindow::on_SetPrintHeight_clicked
 * Sets PrintHeight variable from value inputted by user, also triggers automode
 */
void MainWindow::on_SetPrintHeight_clicked()
{
    PrintHeight = (ui->PrintHeightParam->value());
    PrintToTerminal("Set Print Speed to: " + QString::number(PrintHeight) + "");
    AutoMode();
}

/**
 * @brief MainWindow::on_SetIntialAdhesionTimeButton_clicked
 * Sets InitialExposure variable from the value inputted by the user
 */
void MainWindow::on_SetIntialAdhesionTimeButton_clicked()
{
    m_PrintSettings.InitialExposure = (ui->InitialAdhesionParameter->value());
    PrintToTerminal("Set Initial Exposure to: " + QString::number(m_PrintSettings.InitialExposure) + " s");
}

/**
 * @brief MainWindow::on_SetStartingPosButton_clicked
 * Sets Starting Position variable from the value inputted by the user
 * (also doubles as debug tool for GetPosition(), will be removed)
 */
void MainWindow::on_SetStartingPosButton_clicked()
{
    m_PrintSettings.StartingPosition = (ui->StartingPositionParam->value());
    PrintToTerminal("Set Starting Position to: " + QString::number(m_PrintSettings.StartingPosition) + " mm");
    QString CurrentPosition = Stage.StageGetPosition(m_PrintSettings.StageType);
    CurrentPosition = CurrentPosition.remove(0,3);
    PrintToTerminal("Stage is currently at: " + CurrentPosition + "mm");
    ui->CurrentPositionIndicator->setText(CurrentPosition);
}

/**
 * @brief MainWindow::on_SetSliceThickness_clicked
 * Sets m_PrintSettings.LayerThickness variable from the value inputted by the user
 */
void MainWindow::on_SetSliceThickness_clicked()
{
    m_PrintSettings.LayerThickness = (ui->SliceThicknessParam->value()/1000);
    PrintToTerminal("Set Slice Thickness to: " + QString::number(m_PrintSettings.LayerThickness*1000) + " μm");
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
    Stage.SetStageVelocity(m_PrintSettings.StageVelocity, m_PrintSettings.StageType);
    PrintToTerminal("Set Stage Velocity to: " + QString::number(m_PrintSettings.StageVelocity) +" mm/s");
}

/**
 * @brief MainWindow::on_SetStageAcceleration_clicked
 * Sets StageAcceleration variable from the value inputted by the user,
 * also directly sends command to stage to set acceleration
 */
void MainWindow::on_SetStageAcceleration_clicked()
{
    m_PrintSettings.StageAcceleration = (ui->StageAccelParam->value());
    Stage.SetStageAcceleration(m_PrintSettings.StageAcceleration, m_PrintSettings.StageType);
    PrintToTerminal("Set Stage Acceleration to: " + QString::number(m_PrintSettings.StageAcceleration) + " mm/s");
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
    PrintToTerminal("Set Max End Of Run to: " + QString::number(m_PrintSettings.MaxEndOfRun) + " mm");
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
    PrintToTerminal("Set Min End Of Run to: " + QString::number(m_PrintSettings.MinEndOfRun) + " mm");
}

/******************************************Light Engine Parameters********************************************/
/**
 * @brief MainWindow::on_SetDarkTime_clicked
 * Sets DarkTime variable from the value selected by the user
 */
void MainWindow::on_SetDarkTime_clicked()
{
    m_PrintSettings.DarkTime = (ui->DarkTimeParam->value()*1000);
    PrintToTerminal("Set Dark Time to: " + QString::number(m_PrintSettings.DarkTime/1000) + " ms");
}

/**
 * @brief MainWindow::on_SetExposureTime_clicked
 * Sets ExposureTime variable from the value selected by the user
 */
void MainWindow::on_SetExposureTime_clicked()
{
    m_PrintSettings.ExposureTime = (ui->ExposureTimeParam->value()*1000);
    PrintToTerminal("Set Exposure Time to: " + QString::number(m_PrintSettings.ExposureTime/1000) + " ms");
}

/**
 * @brief MainWindow::on_SetUVIntensity_clicked
 * Sets UVIntensity from the value selected by the user
 */
void MainWindow::on_SetUVIntensity_clicked()
{
    m_PrintSettings.UVIntensity = (ui->UVIntensityParam->value());
    PrintToTerminal("Set UV Intensity to: " + QString::number(m_PrintSettings.UVIntensity));
}
/*******************************************Pump Parameters********************************************/
/**
 * @brief MainWindow::on_ContinuousInjection_clicked
 */
void MainWindow::on_ContinuousInjection_clicked()
{
    if (ui->ContinuousInjection->isChecked() == true){
        Pump.SetTargetVolume(0);
        m_InjectionSettings.ContinuousInjection = true;
        PrintToTerminal("Continuous Injection Selected");
    }
    else{
        m_InjectionSettings.ContinuousInjection = false;
        PrintToTerminal("Continuous Injection Disabled");
    }
}

/**
 * @brief MainWindow::on_SetInfuseRate_clicked
 */
void MainWindow::on_SetInfuseRate_clicked()
{
    m_InjectionSettings.InfusionRate = ui->InfuseRateParam->value();
    Pump.SetInfuseRate(m_InjectionSettings.InfusionRate);
    PrintToTerminal("Set Infusion Rate to: " + QString::number(m_InjectionSettings.InfusionRate) + " μl/s");
}

/**
 * @brief MainWindow::on_SetVolPerLayer_clicked
 */
void MainWindow::on_SetVolPerLayer_clicked()
{
    m_InjectionSettings.InfusionVolume = ui->VolPerLayerParam->value();
    Pump.SetTargetVolume(m_InjectionSettings.InfusionVolume);
    PrintToTerminal("Set Infusion Volume per layer to: " + QString::number(m_InjectionSettings.InfusionVolume) + " ul");
}

void MainWindow::on_SetInitialVolume_clicked()
{
    m_InjectionSettings.InitialVolume = ui->InitialVolumeParam->value();
    PrintToTerminal("Intial injection volume set to: QString::number(m_InjectionSettings.InitialVolume)");
}

void MainWindow::on_PreMovementCheckbox_clicked()
{
    if (ui->PreMovementCheckbox->isChecked() == true){
        ui->PostMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = PRE;
        PrintToTerminal("Pre-Injection delay enabled");
    }
    else{
        ui->PostMovementCheckbox->setChecked(false);
        ui->PreMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = OFF;
        PrintToTerminal("Injection delay disabled");
    }
}

void MainWindow::on_PostMovementCheckbox_clicked()
{
    if (ui->PostMovementCheckbox->isChecked() == true){
        ui->PreMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = POST;
        PrintToTerminal("Post-Injection delay enabled");
    }
    else{
        ui->PostMovementCheckbox->setChecked(false);
        ui->PreMovementCheckbox->setChecked(false);
        m_InjectionSettings.InjectionDelayFlag = OFF;
        PrintToTerminal("Injection delay disabled");
    }
}

void MainWindow::on_SetInjectionDelay_clicked()
{
    m_InjectionSettings.InjectionDelayParam = ui->InjectionDelayParam->value();
    PrintToTerminal("Injection delay set to: " + QString::number(m_InjectionSettings.InjectionDelayParam));
}

/*******************************************Helper Functions********************************************/
/**
 * @brief MainWindow::showError
 * helper function to show the appropriate API Error message
 * @param errMsg - I - error messgae to be shown
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
     QString LogTitle = LogFileDestination + "/CLIPGUITEST_" + LogDate + "_" + LogTime + ".txt";
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
            PrintToTerminal("WARNING: Auto Mode is not accurate if you have not selected all your object image files");
            int TotalPrintTime = PrintHeight / PrintSpeed;  //  um/(um/s) = s
            //PrintToTerminal("PrintHeight: " + QString::number(PrintHeight) + " PrintSpeed: " + QString::number(PrintSpeed) + "  TotalPrintTime: " + QString::number(TotalPrintTime));
            m_PrintSettings.ExposureTime = (TotalPrintTime*1000) / SliceCount;
            //PrintToTerminal("Total Print Time: " + QString::number(TotalPrintTime) + " Slice Count" + QString::number(SliceCount) + "  ExposureTime: " + QString::number(ExposureTime));
            ui->ExposureTimeParam->setValue(m_PrintSettings.ExposureTime);
            PrintToTerminal("Calculated Exposure Time: " + QString::number(m_PrintSettings.ExposureTime) + " ms");

            m_PrintSettings.LayerThickness = PrintHeight / SliceCount;
            ui->SliceThicknessParam->setValue(m_PrintSettings.LayerThickness);
            PrintToTerminal("Calculated Slice Thickness: " + QString::number(m_PrintSettings.LayerThickness) + " um");
        }
        else
        {
            PrintToTerminal("No Object Image Files Detected, Please Select Image Files First");
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
    if(m_PrintSettings.PrinterType == CLIP30UM){
        DetailedText += "Printer set to CLIP 30um\n";
    }
    else if(m_PrintSettings.PrinterType == ICLIP){
        DetailedText += "Printer set to iCLIP\n";
    }

    if(m_PrintSettings.ProjectionMode == POTF){
        DetailedText += "POTF projection mode selected\n";
    }
    else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        DetailedText += "Video Pattern projection mode selected\n";
    }

    if(m_PrintSettings.MotionMode == STEPPED){
        DetailedText += "Motion mode set to stepped\n";
    }
    else if(m_PrintSettings.MotionMode == CONTINUOUS){
        DetailedText += "Motion mode set to continuous\n";
    }

    if(m_PrintSettings.PumpingMode == OFF){
        DetailedText += "Pumping disabled\n";
    }
    else if(m_PrintSettings.PumpingMode == ON){
        DetailedText += "Pumping Enabled\n";
    }

    DetailedText += "Max Image Upload: " + QString::number(m_PrintSettings.MaxImageUpload) + "images\n";
    DetailedText += "Bit Depth set to: " + QString::number(m_PrintSettings.BitMode) + "\n";
    DetailedText += "Initial Exposure Time: " + QString::number(m_PrintSettings.InitialExposure) + "s\n";
    if (m_PrintSettings.PrinterType == CLIP30UM){
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
    if (m_PrintScript.PrintScript == 1)
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
    if (m_PrintSettings.PrinterType == CLIP30UM){
        DetailedText += "Stage Acceleration: " + QString::number(m_PrintSettings.StageAcceleration) + " mm/s^2\n";
        DetailedText += "Max End Of Run: " + QString::number(m_PrintSettings.MaxEndOfRun) + " mm\n";
        DetailedText += "Min End Of Run: " + QString::number(m_PrintSettings.MinEndOfRun) + " mm\n";
    }
    else if (m_PrintSettings.PrinterType == ICLIP){
        if(m_InjectionSettings.ContinuousInjection == ON){
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
        PrintToTerminal("Print Parameters Not Confirmed");
        PrintToTerminal("Cancelling Print");
    }
    else if (confScreen.clickedButton() == okButton)
    {
        PrintToTerminal("Print Parameters Confirmed");
        PrintToTerminal(DetailedText);
        PrintToTerminal("Proceding to Stage Movement and Image Upload");
        retVal = true;
    }
    else
    {
        PrintToTerminal("ConfScreen Error");
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
    if (m_PrintSettings.InitialExposure < 0 || m_PrintSettings.InitialExposure > 400)
    {
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
    else if (m_PrintSettings.MinEndOfRun < -5 || m_PrintSettings.MinEndOfRun > 65 || m_PrintSettings.MinEndOfRun >= m_PrintSettings.MaxEndOfRun){
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
    else if (m_PrintSettings.UVIntensity < 1 || m_PrintSettings.UVIntensity > 255){
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

    settings.setValue("PrinterType", m_PrintSettings.PrinterType);
    settings.setValue("StageType", m_PrintSettings.StageType);

    settings.setValue("MotionMode", m_PrintSettings.MotionMode);
    settings.setValue("PumpingMode",m_PrintSettings.PumpingMode);
    settings.setValue("PumpingParameter", m_PrintSettings.PumpingParameter);
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

        m_PrintSettings.PrinterType = settings.value("PrinterType", CLIP30UM).toDouble();

        m_PrintSettings.MotionMode = settings.value("MotionMode", STEPPED).toDouble();
        m_PrintSettings.PumpingMode = settings.value("PumpingMode", 0).toDouble();
        m_PrintSettings.PumpingParameter = settings.value("PumpingParameter", 0).toDouble();
        m_PrintSettings.BitMode = settings.value("BitMode", 1).toDouble();
        m_InjectionSettings.InfusionRate = settings.value("InfusionRate", 5).toDouble();
        m_InjectionSettings.InfusionVolume = settings.value("InfusionVolume", 5).toDouble();

        m_InjectionSettings.ContinuousInjection = settings.value("ContinuousInjection", OFF).toInt();
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

    if(m_PrintSettings.PrinterType == CLIP30UM){
        ui->CLIPSelect->setChecked(true);
        ui->DICLIPSelect->setChecked(false);
        emit(on_CLIPSelect_clicked());
    }
    else if(m_PrintSettings.PrinterType == ICLIP){
        ui->DICLIPSelect->setChecked(true);
        ui->CLIPSelect->setChecked(false);
        emit(on_DICLIPSelect_clicked());
        if(m_InjectionSettings.ContinuousInjection == ON){
            ui->ContinuousInjection->setChecked(true);
            emit(on_ContinuousInjection_clicked());
        }
    }

    if(m_PrintSettings.MotionMode == STEPPED){
        ui->SteppedMotion->setChecked(true);
        ui->ContinuousMotion->setChecked(false);
        emit(on_SteppedMotion_clicked());
    }
    else if(m_PrintSettings.MotionMode == CONTINUOUS){
        ui->ContinuousMotion->setChecked(true);
        ui->SteppedMotion->setChecked(false);
        emit(on_ContinuousMotion_clicked());
    }

    if(m_PrintSettings.PumpingMode == ON){
        ui->pumpingCheckBox->setChecked(true);
        emit(on_pumpingCheckBox_clicked());
    }
    else if(m_PrintSettings.PumpingMode == OFF){
        ui->pumpingCheckBox->setChecked(false);
    }

    ui->pumpingParameter->setValue(m_PrintSettings.PumpingParameter);
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
    if (m_PrintScript.PrintScript ==1)
    {
        m_PrintControls.TotalPrintTime = 0;
        for (int i= 0; i < m_PrintScript.ExposureScriptList.size(); i++)
            {
                //PrintToTerminal("Exp. Time: " + QString::number(ExposureScriptList.at(i).toDouble()/1000));
                m_PrintControls.TotalPrintTime += m_PrintScript.ExposureScriptList.at(i).toDouble()/1000;
                //PrintToTerminal("Current Total :" + QString::number(TotalPrintTimeS));
            }
        m_PrintControls.TotalPrintTime += m_PrintControls.nSlice * (m_PrintSettings.DarkTime/(1000*1000));
        m_PrintControls.TotalPrintTime += m_PrintSettings.InitialExposure + 5;
        m_PrintControls.TotalPrintTime += m_PrintControls.nSlice * 0.1;
        m_PrintControls.RemainingPrintTime = m_PrintControls.TotalPrintTime;
        ui->LivePlot->xAxis->setRange(0, m_PrintControls.TotalPrintTime*1.1);
    }
    else
    {
        ui->LivePlot->xAxis->setRange(0, m_PrintSettings.InitialExposure+5+0.1*m_PrintControls.nSlice+(1.5*(m_PrintControls.nSlice*(m_PrintSettings.ExposureTime+m_PrintSettings.DarkTime))/(1000*1000)));
    }
    ui->LivePlot->yAxis->setRange(0.9*(m_PrintSettings.StartingPosition - m_PrintControls.nSlice*m_PrintSettings.LayerThickness),1.1*m_PrintSettings.StartingPosition);
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
    QString Layer = " Layer: " + QString::number(m_PrintControls.layerCount) + "/" + QString::number(m_PrintControls.nSlice);
    QCPItemText *textLabel1 = new QCPItemText(ui->LivePlot);
    textLabel1->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    textLabel1->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel1->position->setCoords(0.98, 0.07); // place position at center/top of axis rect
    textLabel1->setText(Layer);
    textLabel1->setFont(QFont(font().family(), 12)); // make font a bit larger
    textLabel1->setPen(QPen(Qt::black)); // show black border around text


    //Update Remaining Time Label
    QString RemainingTime;
    if (m_PrintScript.PrintScript == 1)
    {
        if (m_PrintControls.layerCount > 0)
        {
            m_PrintControls.RemainingPrintTime -= m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount - 1).toDouble()/1000;
        }
        RemainingTime = "Est. Remaining Time: " + QString::number(m_PrintControls.RemainingPrintTime) + "s";
    }
    else
    {
        RemainingTime = "Est. Remaining Time: " + QString::number(((m_PrintSettings.ExposureTime+m_PrintSettings.DarkTime)/(1000*1000))*(m_PrintControls.nSlice-m_PrintControls.layerCount)+m_PrintSettings.InitialExposure) + "s";
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
    PrintToTerminal("Print Complete");
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
                        Pump.SetTargetVolume(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble());
                        ui->LiveValue4->setText(QString::number(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble()));
                        PrintToTerminal("Injection Volume set to : " + QString::number(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble()));
                        break;
                    case INJECTION_RATE:
                        Pump.SetInfuseRate(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble());
                        ui->LiveValue5->setText(QString::number(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble()));
                        PrintToTerminal("Injection Rate set to: " + QString::number(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble()));
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
                Pump.SetInfuseRate(m_PrintScript.InjectionRateScriptList.at(layerCount).toDouble());
                break;
            case INJECTION_RATE:
                Pump.SetTargetVolume(m_PrintScript.InjectionVolumeScriptList.at(layerCount).toDouble());
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
        if(m_PrintScript.PrintScript == ON){
            ExpTime = m_PrintScript.ExposureScriptList.at(i).toDouble();
            DTime = m_PrintScript.DarkTimeScriptList.at(i).toDouble();
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
                LEDlist.append(m_PrintScript.LEDScriptList.at(i));
                FrameList.append(QString::number(0));
                ExpTime -= 33;
                imageList << item->text();
                LayerCount++;
                PrintToTerminal("ET: 33, DT: 0, LED: " + m_PrintScript.LEDScriptList.at(i));
            }
            else{ //frame finished, add remaining exposure time now with dark time
                ExposureTimeList.append(QString::number(ExpTime));
                DarkTimeList.append(QString::number(DTime));
                LEDlist.append(m_PrintScript.LEDScriptList.at(i));
                FrameList.append(QString::number(1));
                LayerCount++;
                imageList << item->text();
                FrameFinished = true;
                PrintToTerminal("ET: " + QString::number(ExpTime) + ", DT: " + QString::number(DTime) + ", LED: " + m_PrintScript.LEDScriptList.at(i));
            }
        }
        //update nSlice
    }
    DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
    PrintToTerminal("Etime: " + QString::number(ExposureTimeList.count()) + ", Dtime: " + QString::number(DarkTimeList.count()) + ", LEDlist: " + QString::number(LEDlist.count()) + ", Images: " + QString::number(imageList.count()));
}

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
    QListWidgetItem * item;
    static uint InitialExposureCount = m_PrintSettings.InitialExposure;
    if (m_PrintControls.InitialExposureFlag == ON){ //If in initial POTF upload
        m_PrintControls.nSlice = ui->FileList->count();
            item = ui->FileList->item(0);
        for (uint i = 1; i < (m_PrintSettings.InitialExposure); i++){
            ImageList << item->text();
        }
        if (m_PrintSettings.ProjectionMode == POTF){
            for (int i = 1; i < ui->FileList->count(); i++){
                item = ui->FileList->item(i);
                ImageList << item->text();
                if ((i + m_PrintSettings.InitialExposure) > m_PrintSettings.MaxImageUpload){
                        break;
                    }
            }
        }
    }
    else if (m_PrintSettings.ProjectionMode == POTF){
        //else in re-upload
        for (int i = m_PrintControls.layerCount; i< ui->FileList->count(); i++){
            item = ui->FileList->item(i);
            ImageList << item->text();
            if (i > m_PrintControls.layerCount + m_PrintSettings.MaxImageUpload){
                break;
            }
        }
    }
    else if (m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        int j = 0;
        for(int i = m_PrintControls.FrameCount; i < m_PrintControls.FrameCount + (5*m_PrintSettings.BitMode); i++)
        {
            for (j = 0; j < (24/m_PrintSettings.BitMode); j++)
            {
                if (i < ui->FileList->count()){
                    item = ui->FileList->item(i);
                    ImageList << item->text();
                }
                else{
                    ui->ProgramPrints->append("VP Image segmentation fault");
                    break;
                }
            }
            ui->ProgramPrints->append(QString::number(j) + " patterns uploaded");
        }
    }
    return ImageList;
}
/*******************************************Live Value Monitoring********************************************/
/**
 * @brief MainWindow::on_LiveValueList1_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList1_activated(const QString &arg1)
{
    PrintToTerminal("LV1: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList2_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList2_activated(const QString &arg1)
{
    PrintToTerminal("LV2: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList3_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList3_activated(const QString &arg1)
{
    PrintToTerminal("LV3: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList4_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList4_activated(const QString &arg1)
{
    PrintToTerminal("LV4: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList5_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList5_activated(const QString &arg1)
{
    PrintToTerminal("LV5: " + arg1);
}

/**
 * @brief MainWindow::on_LiveValueList6_activated
 * @param arg1
 * Currently Not In Use
 */
void MainWindow::on_LiveValueList6_activated(const QString &arg1)
{
    PrintToTerminal("LV6: " + arg1);
}
/*************************************************************
 * ********************Graveyard***************************
 * ***********************************************************/
//Snippets that may be useful in the future but the overall functionality has been deprecated
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
        if(m_PrintSettings.ProjectionMode == POTF){
            m_PrintControls.nSlice = ui->FileList->count();
            m_PrintControls.remainingImages = m_PrintSettings.MaxImageUpload - m_PrintSettings.InitialExposure;
        }
        else if(m_PrintSettings.ProjectionMode == VIDEOPATTERN){
            m_PrintControls.nSlice = (24/m_PrintSettings.BitMode)*ui->FileList->count();
            QString filename = ui->FileList->item(m_PrintControls.layerCount)->text();
            QPixmap img(filename);
            ImagePopoutUI->showImage(img);
        }
        QStringList ImageList = GetImageList(m_PrintControls, m_PrintSettings);
        PrintControl.InitializeSystem(ImageList, m_PrintSettings, &m_PrintControls, m_PrintScript);

        emit(on_GetPosition_clicked());
        initPlot();
        updatePlot();
        //If in continuous motion mode, calculates the print end position
        ui->StartPrint->setEnabled(true);
        printf("%f",m_PrintControls.PrintEnd);
    }
}

/**
 * @brief MainWindow::on_StartPrint_clicked
 * Starts print process, set LED current calculates speed for continuous motion mode
 */
void MainWindow::on_StartPrint_clicked()
{
    //If settings are validated successfully and Initialization has been completed
    if (ValidateSettings() == true){
        PrintToTerminal("Entering Printing Procedure");
        PrintStartTime = QTime::currentTime(); //Get print start time from current time
        ImageList = GetImageList(m_PrintControls, m_PrintSettings);

        PrintControl.StartPrint(m_PrintSettings, m_PrintScript, m_InjectionSettings);
        PrintProcess();
    }
}

/**
 * @brief MainWindow::on_AbortPrint_clicked
 * Aborts print and acts as e-stop. Stops light engine projection,
 * stage movement and print process
 */
void MainWindow::on_AbortPrint_clicked()
{
    PrintControl.AbortPrint(m_PrintSettings, &m_PrintControls);
    ui->ProgramPrints->append("PRINT ABORTED");
}

void MainWindow::PrintProcess()
{
    if(m_PrintControls.layerCount + 1 <= m_PrintControls.nSlice){
        if (m_PrintControls.remainingImages <= 0){
            QStringList ImageList =  GetImageList(m_PrintControls, m_PrintSettings);
            m_PrintControls.remainingImages = PrintControl.ReuploadHandler(ImageList, m_PrintControls, m_PrintSettings, m_PrintScript);
        }
        SetExposureTimer();
        PrintControl.PrintProcessHandler(&m_PrintControls, m_PrintSettings);
    }
    else{
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
    PrintControl.StagePumpingHandler(m_PrintControls, m_PrintSettings, m_PrintScript);
}

/**
 * @brief MainWindow::ExposureTimeSlot
 * Handles all dark time actions required
 */
void MainWindow::ExposureTimeSlot(void)
{
    //Set dark timer first for most accurate timing
    SetDarkTimer(); //Set dark timer first for best timing
    //Record current time in terminal
    ui->ProgramPrints->append(QTime::currentTime().toString("hh.mm.ss.zzz"));
    updatePlot(); //update plot early in dark time

    //Video pattern mode handling
    if (m_PrintSettings.ProjectionMode == VIDEOPATTERN){ //If in video pattern mode
        if (PrintControl.VPFrameUpdate(&m_PrintControls, m_PrintSettings) == true){
            if (m_PrintControls.FrameCount < ui->FileList->count()){ //if not at end of file list
                QPixmap img(ui->FileList->item(m_PrintControls.FrameCount)->text()); //select next image
                ImagePopoutUI->showImage(img); //display next image
            }
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
    PrintProcess();
}

void MainWindow::SetExposureTimer()
{
    if (m_PrintControls.InitialExposureFlag == 1){
        QTimer::singleShot(m_PrintSettings.InitialExposure*1000, Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
    }
    else{
        switch (m_PrintControls.ExposureType){
            case EXPOSURE_NORM:
                QTimer::singleShot(m_PrintSettings.ExposureTime/1000, Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
                break;
            case EXPOSURE_PUMP:
                QTimer::singleShot(m_PrintSettings.ExposureTime/1000, Qt::PreciseTimer, this, SLOT(pumpingSlot()));
                break;
            case EXPOSURE_PS:
                QTimer::singleShot(m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
                break;
            case EXPOSURE_PS_PUMP:
                QTimer::singleShot(m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(pumpingSlot()));
                break;
            default:
                break;
        }
    }
}

void MainWindow::SetDarkTimer()
{
    double DarkTimeSelect = m_PrintSettings.DarkTime;
    if (m_PrintSettings.MotionMode == STEPPED){
        if (m_PrintScript.PrintScript == ON){
            if(m_PrintControls.layerCount < m_PrintScript.DarkTimeScriptList.size()){
                DarkTimeSelect = m_PrintScript.DarkTimeScriptList.at(m_PrintControls.layerCount).toDouble();
            }
            else{
                DarkTimeSelect = m_PrintScript.DarkTimeScriptList.at(m_PrintControls.layerCount-2).toDouble();
            }
        }
        QTimer::singleShot(DarkTimeSelect/1000,  Qt::PreciseTimer, this, SLOT(DarkTimeSlot()));
        ui->ProgramPrints->append("Dark time: " + QString::number(DarkTimeSelect));
    }
    else{
        if (m_PrintControls.inMotion == false){
            Stage.StageAbsoluteMove(m_PrintControls.PrintEnd, m_PrintSettings.StageType);
        }
        PrintProcess();
    }
}
