#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "API.h"
#include "dlp9000.h"

DLP9000& p_DLP = DLP9000::Instance();

/***************************************Print Functionality*********************************************/
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
    m_PrintControls.layerCount = 0xFFFFFF; //Set layer count high to stop print process
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
        if (m_PrintScript.PrintScript == ON){ //If printscript is on
            LCR_SetLedCurrents(0,0,(m_PrintScript.LEDScriptList.at(0).toInt())); //Set LED intensity to first value in printscript
        }
        else{ //if printscript is off
            LCR_SetLedCurrents(0, 0, m_PrintSettings.UVIntensity); //set static LED intensity
        }

        if (m_PrintSettings.MotionMode == CONTINUOUS){ //continuous motion mode
            double ContStageVelocity = (m_PrintSettings.StageVelocity)/(m_PrintSettings.ExposureTime/(1e6)); //Multiply Exposure time by 1e6 to convert from us to s to get to proper units
            ui->ProgramPrints->append("Continuous Stage Velocity set to " + QString::number(m_PrintSettings.StageVelocity) + "/" + QString::number(m_PrintSettings.ExposureTime) + " = " + QString::number(ContStageVelocity) + " mm/s");
            Stage.SetStageVelocity(ContStageVelocity, m_PrintSettings.StageType);
        }

        PrintStartTime = QTime::currentTime(); //Get print start time from current time
        p_DLP.startPatSequence(); //Start projection

        //Based on projection mode enter correct print process
        if (m_PrintSettings.ProjectionMode == POTF){ //if in POTF mode
            PrintProcess(); //Start print process
            ui->ProgramPrints->append("Entering POTF print process");
        }
        else{ //else in Video Pattern Mode
            p_DLP.clearElements();
            PrintProcessVP(); //Start video pattern print process
            ui->ProgramPrints->append("Entering Video Pattern print process");
        }

        //If continuous injection is on then start pump infusion
        if (m_InjectionSettings.ContinuousInjection == ON){
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
    if (m_PrintControls.layerCount +1 <= m_PrintControls.nSlice)
    {
        //If there are no remaining images, reupload
        if (m_PrintControls.remainingImages <= 0)
        {
            //If motion mode is set to continuous, pause stage movement during re-upload
            if (m_PrintSettings.MotionMode == CONTINUOUS){
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
            for(int i = (m_PrintControls.layerCount); (i < ui->FileList->count()); i++)
            {
                    item = ui->FileList->item(i);
                    imageList << item->text();
                    count++;
                    if (i > m_PrintControls.layerCount + MaxImageReupload)
                    {
                        break;
                    }
            }

            //Add patterns to local memory, upload to light engine, then clear local memory
            p_DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
            p_DLP.updateLUT(m_PrintSettings.ProjectionMode);
            p_DLP.clearElements();

            m_PrintControls.remainingImages = count - 1; //Set the remaining images to the # of images uploaded - 1
            m_PrintControls.layerCount++; //Update layer count
            Sleep(50); //Add delay to allow for light engine process to run
            p_DLP.startPatSequence(); //Start projection
            if(m_PrintSettings.PrinterType == ICLIP){ //delay is needed for iCLIP printer only
                Sleep(500);
            }
            SetExposureTimer(0, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode); //Set the exposure time

            //if in continuous motion mode restart movement
            if(m_PrintSettings.MotionMode == CONTINUOUS){
                Stage.StageAbsoluteMove(m_PrintControls.PrintEnd, m_PrintSettings.StageType);
                m_PrintControls.inMotion = true;
            }
            ui->ProgramPrints->append("Reupload succesful, current layer: " + QString::number(m_PrintControls.layerCount));
            ui->ProgramPrints->append(QString::number(m_PrintControls.remainingImages + 1) + " images uploaded");
        }
        //If in intial exposure mode
        else if (m_PrintControls.InitialExposureFlag == true)
        {
            SetExposureTimer(m_PrintControls.InitialExposureFlag, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode); //Set exposure timer with InitialExposureFlag high
            updatePlot();
            m_PrintControls.InitialExposureFlag = false; //Set InitialExposureFlag low
            m_PrintControls.inMotion = false; //Stage is currently not in motion, used for continuos stage movement
            ui->ProgramPrints->append("POTF Exposing Initial Layer " + QString::number(m_PrintSettings.InitialExposure) + "s");
        }
        else
        {
            SetExposureTimer(0, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode); //set exposure time
            ui->ProgramPrints->append("Layer: " + QString::number(m_PrintControls.layerCount));

            //Grab the current image file name and print to
            QString filename =ui->FileList->item(m_PrintControls.layerCount)->text();
            ui->ProgramPrints->append("Image File: " + filename);
            m_PrintControls.layerCount++; //Update layerCount for new layer
            m_PrintControls.remainingImages--; //Decrement remaining image counter
            if(m_PrintSettings.MotionMode == STEPPED){
                updatePlot();
            }
            else if (m_PrintSettings.MotionMode == CONTINUOUS){
                if(m_PrintSettings.PrinterType == ICLIP){
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
    if (m_PrintControls.layerCount +1 <= m_PrintControls.nSlice)
    {
        if (m_PrintControls.ReSyncFlag == 1) //if resync
        {
            m_PrintControls.ReSyncFlag = 0;
            if (LCR_PatternDisplay(0) < 0)
                showError("Unable to stop pattern display");
            Sleep(10);
            QStringList imageList = GetImageList(m_PrintControls, m_PrintSettings);
            //Add pattern data to buffer to prepare for pattern upload
            if (m_PrintSettings.ProjectionMode == VIDEOPATTERN && m_PrintSettings.BitMode == 8){
                //VP8bitWorkaround();
                //ui->ProgramPrints->append("Using VP 8-bit workaround");
                p_DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
            }
            else{
                p_DLP.AddPatterns(imageList, m_PrintSettings, m_PrintScript, m_PrintControls);
            }
            p_DLP.updateLUT(m_PrintSettings.ProjectionMode); //update LUT on light engine to upload pattern data
            p_DLP.clearElements(); //clear pattern data buffer
            Sleep(50); //small delay to ensure that the new patterns are uploaded
            PrintToTerminal("Resync succesful, frame: " + QString::number(m_PrintControls.FrameCount) + " layer: " + QString::number(m_PrintControls.layerCount) + "New Patterns: " + QString::number(imageList.count()));
            p_DLP.startPatSequence();
            Sleep(5);
            SetExposureTimer(0, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode);
            m_PrintControls.layerCount++; //increment layer counter
            m_PrintControls.remainingImages--; //decrement remaining images
            m_PrintControls.BitLayer += m_PrintSettings.BitMode;
        }
        else if (m_PrintControls.InitialExposureFlag == true) //Initial Exposure
        {
            SetExposureTimer(m_PrintControls.InitialExposureFlag, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode);
            m_PrintControls.BitLayer = 1; //set to first bitlayer
            m_PrintControls.ReSyncFlag = 1; //resync after intial exposure
            ui->ProgramPrints->append("VP Exposing Initial Layer " + QString::number(m_PrintSettings.InitialExposure) + "s");
            m_PrintControls.InitialExposureFlag = 0;
        }
        else //Normal print process
        {
            //Start exposuretime timers
            SetExposureTimer(0, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode);
            //Print information to log
            ui->ProgramPrints->append("VP Layer: " + QString::number(m_PrintControls.layerCount));
            QString filename =ui->FileList->item(m_PrintControls.FrameCount)->text();
            ui->ProgramPrints->append("Image File: " + filename);

            m_PrintControls.layerCount++; //increment layer counter
            m_PrintControls.remainingImages--; //decrement remaining images

            //updatePlot();
            m_PrintControls.BitLayer += m_PrintSettings.BitMode;
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
    if(m_PrintScript.PrintScript == ON){
        if (m_PrintControls.layerCount < m_PrintScript.PumpHeightScriptList.size()){
            Stage.StageRelativeMove(-m_PrintScript.PumpHeightScriptList.at(m_PrintControls.layerCount).toDouble(), m_PrintSettings.StageType);
            ui->ProgramPrints->append("Pumping " + QString::number(m_PrintScript.PumpHeightScriptList.at(m_PrintControls.layerCount).toDouble()*1000) +" um");
        }
    }
    else{
        Stage.StageRelativeMove(-m_PrintSettings.PumpingParameter, m_PrintSettings.StageType);
        ui->ProgramPrints->append("Pumping " + QString::number(m_PrintSettings.PumpingParameter*1000) +" um");
    }
}

/**
 * @brief MainWindow::ExposureTimeSlot
 * Handles all dark time actions required
 */
void MainWindow::ExposureTimeSlot(void)
{
    //Set dark timer first for most accurate timing
    SetDarkTimer(m_PrintScript.PrintScript, m_PrintSettings.MotionMode); //Set dark timer first for best timing
    //Record current time in terminal
    ui->ProgramPrints->append(QTime::currentTime().toString("hh.mm.ss.zzz"));
    updatePlot(); //update plot early in dark time

    //Video pattern mode handling
    if (m_PrintSettings.ProjectionMode == VIDEOPATTERN) //If in video pattern mode
    {
        if (m_PrintControls.BitLayer > 24) //If end of frame has been reached
        {
            m_PrintControls.FrameCount++; //increment frame counter
            ui->ProgramPrints->append("New Frame: " + QString::number(m_PrintControls.FrameCount));
            if (m_PrintControls.FrameCount < ui->FileList->count()){ //if not at end of file list
                QPixmap img(ui->FileList->item(m_PrintControls.FrameCount)->text()); //select next image
                ImagePopoutUI->showImage(img); //display next image
            }
            m_PrintControls.BitLayer = 1; //Reset BitLayer to first layer
            m_PrintControls.ReSyncCount++; //Add to the resync count
            //If 120 frames have been reached, prepare for resync
            if (m_PrintControls.ReSyncCount > (120-24)/(24/m_PrintSettings.BitMode)){
                m_PrintControls.ReSyncFlag = 1;
                m_PrintControls.ReSyncCount = 0;
            }
        }
    }

    //Print Script handling
    if(m_PrintScript.PrintScript == ON)
    {
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

    //Injection handling
    if (m_PrintSettings.PrinterType == ICLIP)
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
                QTimer::singleShot(m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(pumpingSlot()));
            }
            else{
                QTimer::singleShot(m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount).toDouble(), Qt::PreciseTimer, this, SLOT(ExposureTimeSlot()));
            }
            ui->ProgramPrints->append("Exposing: " + QString::number(m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount).toDouble()) + " ms");
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

void MainWindow::SetDarkTimer(int PrintScript, int DarkMotionMode)
{
    double DarkTimeSelect = m_PrintSettings.DarkTime;
    if (DarkMotionMode == STEPPED)
    {
        if (PrintScript == ON){
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

/**
 * @brief MainWindow::PrintInfuse
 * Function for handling injection for iCLIP
 */
void MainWindow::PrintInfuse()
{
    if (m_InjectionSettings.ContinuousInjection == OFF){
        Pump.ClearVolume();
        Sleep(15);
        Pump.StartInfusion();
        ui->ProgramPrints->append("PrintInfuse");
    }
}


double MainWindow::CalcContinuousVelocity(PrintSettings m_PrintSettings)
{
    double ContStageVelocity = (m_PrintSettings.StageVelocity)/(m_PrintSettings.ExposureTime/(1e6)); //Multiply Exposure time by 1e6 to convert from us to s to get to proper units
    ui->ProgramPrints->append("Continuous Stage Velocity set to " + QString::number(m_PrintSettings.StageVelocity) + "/" + QString::number(m_PrintSettings.ExposureTime) + " = " + QString::number(ContStageVelocity) + " mm/s");
    Stage.SetStageVelocity(ContStageVelocity, m_PrintSettings.StageType);

    return ContStageVelocity;
}

void MainWindow::InitializeMW()
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

    Stage.initStagePosition(m_PrintSettings);
    QStringList ImageList = GetImageList(m_PrintControls, m_PrintSettings);
    p_DLP.PatternUpload(ImageList, m_PrintControls, m_PrintSettings, m_PrintScript);
    emit(on_GetPosition_clicked()); //get stage position
    initPlot(); //Initialize plot
    updatePlot(); //Update plot
    ui->StartPrint->setEnabled(true);
}

void MainWindow::StartPrint()
{
    if (ValidateSettings() == true)
    {
        Stage.initStageStart(m_PrintSettings);
        PrintToTerminal("Entering print procedure");
        p_DLP.SetLEDIntensity(m_PrintSettings, m_PrintScript);
        if (m_PrintSettings.MotionMode == CONTINUOUS){ //continuous motion mode
            Stage.SetStageVelocity( CalcContinuousVelocity(m_PrintSettings), m_PrintSettings.StageType);
        }

        PrintStartTime = QTime::currentTime();
        p_DLP.startPatSequence();

        //Based on projection mode enter correct print process
        if (m_PrintSettings.ProjectionMode == POTF){ //if in POTF mode
            PrintProcess(); //Start print process
            ui->ProgramPrints->append("Entering POTF print process");
        }
        else{ //else in Video Pattern Mode
            p_DLP.clearElements();
            PrintProcessVP(); //Start video pattern print process
            ui->ProgramPrints->append("Entering Video Pattern print process");
        }

        //If continuous injection is on then start pump infusion
        if (m_InjectionSettings.ContinuousInjection == ON){
            Pump.StartInfusion();
        }
    }
}

void MainWindow::PrintProcess2()
{
    if (m_PrintControls.layerCount + 1 <= m_PrintControls.nSlice)
    {
        if (m_PrintControls.remainingImages <= 0)
        {
            if(m_PrintSettings.MotionMode == CONTINUOUS){
                Stage.StageStop(m_PrintSettings.StageType);
            }
            QStringList ImageList = GetImageList(m_PrintControls, m_PrintSettings);
            m_PrintControls.remainingImages = p_DLP.PatternUpload(ImageList, m_PrintControls, m_PrintSettings, m_PrintScript);
        }

        if (m_PrintControls.InitialExposureFlag == true){
            SetExposureTimer(m_PrintControls.InitialExposureFlag, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode); //Set exposure timer with InitialExposureFlag high
            updatePlot();
            m_PrintControls.InitialExposureFlag = false; //Set InitialExposureFlag low
            m_PrintControls.inMotion = false; //Stage is currently not in motion, used for continuos stage movement
            ui->ProgramPrints->append("POTF Exposing Initial Layer " + QString::number(m_PrintSettings.InitialExposure) + "s");
        }
        else{
            SetExposureTimer(0, m_PrintScript.PrintScript, m_PrintSettings.PumpingMode);


            m_PrintControls.layerCount++;
            m_PrintControls.remainingImages--;
        }
    }
}


