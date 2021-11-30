#include "graphics.h"
#include "ui_graphics.h"

graphics::graphics(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::graphics)
{
    ui->setupUi(this);
}

graphics::~graphics()
{
    delete ui;
}

/***************************************Plot Handling*********************************************/
double graphics::calcPrintTime(PrintSettings m_PrintSettings, PrintControls m_PrintControls, PrintScripts m_PrintScript)
{
    double printTime = 0;
    printTime += m_PrintSettings.InitialExposure + m_PrintSettings.InitialDelay;
    if (m_PrintScript.PrintScript == ON){
        for (int i = 0; i < m_PrintScript.ExposureScriptList.size(); i++){
            printTime += m_PrintScript.ExposureScriptList.at(i).toDouble()/1000;
            printTime += m_PrintScript.DarkTimeScriptList.at(i).toDouble()/1000;
        }
    }
    else{
        printTime += m_PrintSettings.ExposureTime * m_PrintControls.nSlice / (1000*1000);
        printTime += m_PrintSettings.ExposureTime * m_PrintControls.nSlice / (1000*1000);
    }

    if(m_PrintSettings.ProjectionMode == POTF){
        printTime += m_PrintControls.nSlice * 0.15;
    }
    else if (m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        int nReuploads = 0;
        if (m_PrintScript.PrintScript == ON){
            nReuploads = calcReuploads(m_PrintScript);
        }
        else{
            nReuploads = m_PrintScript.ExposureScriptList.size() / m_PrintSettings.ResyncVP;
        }
        printTime += nReuploads * 1.8;
    }
    return printTime;
}

int graphics::calcReuploads(PrintScripts m_PrintScript)
{
    int nReuploads = 1;     // default to 1
    if (m_PrintScript.PrintScript == ON){
        for(int i = 0; i < m_PrintScript.ExposureScriptList.size()/24; i++){
        }
    }
#if 0
    if (m_PrintControls.layerCount > 1 && (m_PrintControls.layerCount + m_PrintSettings.ResyncVP < m_PrintScript.ExposureScriptList.size()) ){
        returnVal = false; // Default to false if all match
        for (int i = m_PrintControls.layerCount; i < m_PrintControls.layerCount + m_PrintSettings.ResyncVP; i++){
             double Last = m_PrintScript.ExposureScriptList.at(i - m_PrintSettings.ResyncVP).toDouble();
             double Current = m_PrintScript.ExposureScriptList.at(i).toDouble();
             if (Last != Current){
                 returnVal = true;
                 break;
             }
        }
    }
#endif
    return nReuploads;
}


void graphics::initPlot(PrintControls m_PrintControls, PrintSettings m_PrintSettings,
                        PrintScripts m_PrintScript)
{
    ui->LivePlot->addGraph();
    ui->LivePlot->clearItems();     // Clear any previous items in reset case
    ui->LivePlot->graph(0)->setName("Print Progress");
    ui->LivePlot->xAxis->setLabel("Time (s)");
    ui->LivePlot->yAxis->setLabel("Position (mm)");

    double TotalPrintTime = calcPrintTime(m_PrintSettings, m_PrintControls, m_PrintScript);
    ui->LivePlot->xAxis->setRange(0, TotalPrintTime*1.1);
    RemainingPrintTime = TotalPrintTime;
    double upper = 0;
    double lower = 0;
    if (m_PrintSettings.PrinterType == CLIP30UM){
        upper = 1.1 * m_PrintSettings.StartingPosition;
        lower = 0.9 * (m_PrintSettings.StartingPosition -
                       m_PrintSettings.LayerThickness * m_PrintControls.nSlice);
    }
    else if (m_PrintSettings.PrinterType == ICLIP){
        upper = 1.1 * (m_PrintControls.nSlice * m_PrintSettings.LayerThickness);
        lower = 0;
    }
    ui->LivePlot->yAxis->setRange(lower, upper);
    ui->LivePlot->replot();
}

void graphics::updatePlot(PrintControls m_PrintControls, PrintSettings m_PrintSettings,
                          PrintScripts m_PrintScript)
{
    QTime updateTime = QTime::currentTime();
    int TimeElapsed = m_PrintControls.PrintStartTime.secsTo(updateTime);
    qv_x.append(TimeElapsed);
    qv_y.append(m_PrintControls.StagePosition);
    ui->LivePlot->graph(0)->setData(qv_x, qv_y);
    ui->LivePlot->clearItems();

    // Update Layer label
    QString Layer = " Layer: " + QString::number(m_PrintControls.layerCount) + "/" + QString::number(m_PrintControls.nSlice);
    QCPItemText *textLabel1 = new QCPItemText(ui->LivePlot);
    textLabel1->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    textLabel1->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel1->position->setCoords(0.98, 0.07); // place position at center/top of axis rect
    textLabel1->setText(Layer);
    textLabel1->setFont(QFont(font().family(), 12)); // make font a bit larger
    textLabel1->setPen(QPen(Qt::black)); // show black border around text

    // Update remaining time variable
    if (m_PrintControls.layerCount == 1){
        RemainingPrintTime -= m_PrintSettings.InitialExposure + m_PrintSettings.InitialDelay;
    }

    if (m_PrintScript.PrintScript == ON){
        if (m_PrintControls.layerCount > 0 && m_PrintControls.layerCount < m_PrintScript.ExposureScriptList.count()){
            RemainingPrintTime -= m_PrintScript.ExposureScriptList.at(m_PrintControls.layerCount - 1).toDouble()/1000;
            RemainingPrintTime -= m_PrintScript.DarkTimeScriptList.at(m_PrintControls.layerCount - 1).toDouble()/1000;
        }
    }
    else{
        RemainingPrintTime -= (m_PrintSettings.ExposureTime) / (1000*1000);
        RemainingPrintTime -= (m_PrintSettings.DarkTime) / (1000*1000);
    }

    if (m_PrintSettings.ProjectionMode == POTF){
        RemainingPrintTime -= 0.15;
    }
    else if (m_PrintSettings.ProjectionMode == VIDEOPATTERN){
        //RemainingPrintTime -= 1.8/m_PrintSettings.ResyncVP;
    }
    if (RemainingPrintTime <  0){
        RemainingPrintTime = 0;
    }
    QString RemainingTime = "Est. Remaining Time: " + QString::number(RemainingPrintTime) + "s";

    QCPItemText *textLabel2 = new QCPItemText(ui->LivePlot);
    textLabel2->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    textLabel2->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel2->position->setCoords(0.98, 0.0); // place position at center/top of axis rect
    textLabel2->setText(RemainingTime);
    textLabel2->setFont(QFont(font().family(), 12)); // make font a bit larger
    textLabel2->setPen(QPen(Qt::black)); // show black border around text

    ui->LivePlot->replot(QCustomPlot::rpQueuedReplot);
}

void graphics::addInitialExpLabel()
{
    QCPItemText *textLabel3 = new QCPItemText(ui->LivePlot);
    textLabel3->setPositionAlignment(Qt::AlignTop|Qt::AlignRight);
    textLabel3->position->setType(QCPItemPosition::ptAxisRectRatio);
    textLabel3->position->setCoords(0.98, 0.14); // place position at center/top of axis rect
    textLabel3->setText(" Initial Exposure Active");
    textLabel3->setFont(QFont(font().family(), 12)); // make font a bit larger
    textLabel3->setPen(QPen(Qt::black)); // show black border around tex
    ui->LivePlot->replot(QCustomPlot::rpQueuedReplot);
}

/************************************Print Script Table********************************************/
void graphics::initPrintScriptTable(PrintSettings m_PrintSettings, PrintScripts *pPrintScript)
{
    //Config Header
    ui->PrintScriptTable->setRowCount(pPrintScript->ExposureScriptList.count());
    ui->PrintScriptTable->setColumnCount(7);
    QStringList Labels = {"Exp Time(ms)", "LED Int","Dark Time(ms)",
                          "Layer(um)","Velocity(mm/s)",
                          "Accel(mm/s^2)", "Pump Height(um)",};
    if (m_PrintSettings.PrinterType == ICLIP){
        ui->PrintScriptTable->setColumnCount(9);
        Labels += { "Inj Volume(ul)" , "Inj Rate(ul/s)" };
    }
    ui->PrintScriptTable->setHorizontalHeaderLabels(Labels);
    gpPrintScript = pPrintScript;
    updatePrintScriptTable(m_PrintSettings, *pPrintScript);
    PSTableInitFlag = true;
}

void graphics::PrintScriptTableEntry(QStringList Script, uint ColNum)
{
    for (int i = 0; i < Script.count(); i++){
        QTableWidgetItem *pCell = ui->PrintScriptTable->item(i,ColNum);
        if(!pCell){
            pCell = new QTableWidgetItem;
            ui->PrintScriptTable->setItem(i, ColNum, pCell);
        }
        pCell->setText(Script.at(i));
    }
}

void graphics::updatePrintScriptTable(PrintSettings m_PrintSettings, PrintScripts m_PrintScript)
{
    PrintScriptTableEntry(m_PrintScript.ExposureScriptList, 0);
    PrintScriptTableEntry(m_PrintScript.LEDScriptList, 1);
    PrintScriptTableEntry(m_PrintScript.DarkTimeScriptList, 2);
    PrintScriptTableEntry(m_PrintScript.LayerThicknessScriptList, 3);
    PrintScriptTableEntry(m_PrintScript.StageVelocityScriptList, 4);
    PrintScriptTableEntry(m_PrintScript.StageAccelerationScriptList, 5);
    PrintScriptTableEntry(m_PrintScript.PumpHeightScriptList, 6);
    if (m_PrintSettings.PrinterType == ICLIP){
        PrintScriptTableEntry(m_PrintScript.InjectionVolumeScriptList, 7);
        PrintScriptTableEntry(m_PrintScript.InjectionRateScriptList, 8);
    }
}

void graphics::on_PrintScriptTable_cellChanged(int row, int column)
{
    if(PSTableInitFlag){ //Only activate after print script table is init
        QString CurrentCell = ui->PrintScriptTable->item(row, column)->text();
        QString ScriptType;
        switch (column)
        {
            case 0:
                gpPrintScript->ExposureScriptList[row] = CurrentCell;
                ScriptType = "Exposure layer ";
                break;
            case 1:
                gpPrintScript->LEDScriptList[row] = CurrentCell;
                ScriptType = "LED Intensity layer ";
                break;
            case 2:
                gpPrintScript->DarkTimeScriptList[row] = CurrentCell;
                ScriptType = "Dark Time layer ";
                break;
            case 3:
                gpPrintScript->LayerThicknessScriptList[row] = CurrentCell;
                ScriptType = "Layer Thickness layer ";
                break;
            case 4:
                gpPrintScript->StageVelocityScriptList[row] = CurrentCell;
                ScriptType = "Velocity layer ";
                break;
            case 5:
                gpPrintScript->StageAccelerationScriptList[row] = CurrentCell;
                ScriptType = "Acceleration layer ";
                break;
            case 6:
                gpPrintScript->PumpHeightScriptList[row] = CurrentCell;
                ScriptType = "Pump Height layer ";
                break;
            case 7:
                gpPrintScript->InjectionVolumeScriptList[row] = CurrentCell;
                ScriptType = "Injection Volume layer ";
                break;
            case 8:
                gpPrintScript->InjectionRateScriptList[row] = CurrentCell;
                ScriptType = "Injection Rate layer ";
                break;
            default:
                break;
        }
        emit GraphicsPrint(ScriptType + QString::number(row+1) + " set to " + CurrentCell);
    }
}

static bool EditsActive = true;
void graphics::on_PrintScriptTable_cellPressed(int row, int column)
{
    if(EditsActive){
        QMessageBox confScreen;
        confScreen.setText("Are you sure you want to manually edit the print script?");
        QPushButton *YesButton = confScreen.addButton(QMessageBox::Yes);
        QPushButton *AbortButton = confScreen.addButton(QMessageBox::Abort);
        confScreen.exec();
        if (confScreen.clickedButton() == YesButton){
            EditsActive = false;
        }
        else{
            ui->PrintScriptTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
            //EditsActive = false;
        }
    }
}

#if 0

/*!
 * \brief MainWindow::on_AutoCheckBox_stateChanged
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

/*!
 * \brief MainWindow::on_AutoCheckBox_clicked
 * Guard on edge case that sets the checkbox while it's actual state is unchecked
 */
void MainWindow::on_AutoCheckBox_clicked()
{
    int SliceCount = ui->FileList->count();
    if (SliceCount < 1){
        ui->AutoCheckBox->setChecked(false);
    }
}

/*!
 * \brief MainWindow::on_setPrintSpeed_clicked
 * Sets PrintSpeed variable from value inputted by user, also triggers automode
 */
void MainWindow::on_setPrintSpeed_clicked()
{
    PrintSpeed = (ui->PrintSpeedParam->value());
    PrintToTerminal("Set Print Speed to: " + QString::number(PrintSpeed) + " um/s");
    AutoMode();
}

/*!
 * \brief MainWindow::on_SetPrintHeight_clicked
 * Sets PrintHeight variable from value inputted by user, also triggers automode
 */
void MainWindow::on_SetPrintHeight_clicked()
{
    PrintHeight = (ui->PrintHeightParam->value());
    PrintToTerminal("Set Print Speed to: " + QString::number(PrintHeight) + "");
    AutoMode();
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
#endif

