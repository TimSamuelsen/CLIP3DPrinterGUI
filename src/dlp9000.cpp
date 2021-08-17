#include "dlp9000.h"
#include "API.h"
#include "usb.h"
#include "patternelement.h"
#include "PtnImage.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QTextStream>
#include <QTime>
#include <QTimer>
#include <QProgressDialog>

/*!
 * \brief DLP9000::InitProjector
 * Initializes connection to the projector.
 * \return - Returns true if connection was successful, false otherwise
 */
bool DLP9000::InitProjector(void)
{
    if (USB_Open() == 0)
    {
        LCR_SetMode(PTN_MODE_OTF);
        return true;
    }
    else
    {
        emit DLPError("Light Engine Connection Failed");
        return false;
    }
}

/*!
 * \brief DLP9000::AddPatterns
 * Prepares patterns and stores them in program memory prior to upload.
 * \param fileNames - A list of file locations for images to be uploaded
 * \param m_PrintSettings - Module print settings, using BitMode, ProjectionMode, ExposureTime, and DarkTime
 * \param m_PrintScripts - Module print scripts, using ExposureTimeScript and DarkTimeScript
 * \param m_PrintControls - Module print controls, using layerCount and InitialExposureFlag
 */
void DLP9000::AddPatterns(QStringList fileNames, PrintSettings m_PrintSettings, PrintScripts m_PrintScripts, PrintControls m_PrintControls)
{
    if(m_PrintSettings.BitMode == 0){
        m_PrintSettings.BitMode = 1; //Default bitmode to 1 if it somehow gets passed in undefined
    }
    uint CurrentImage = m_PrintControls.layerCount;
    int i;
    int BitPos = 0;
    int numPatAdded = 0;
    if (m_PrintSettings.ProjectionMode == 1)//Video Pattern Mode
    {

        printf("VP pattern upload");
        for(i = 0; i < fileNames.size(); i++)
        {
            PatternElement pattern;

            if(m_elements.size()==0)
            {
                pattern.bits = m_PrintSettings.BitMode;
                pattern.color = PatternElement::BLUE;
                if (m_PrintScripts.PrintScript == 1)
                {
                    pattern.exposure = m_PrintScripts.ExposureScriptList.at(CurrentImage).toInt() * 1000; //*1000 to get from ms to us
                    pattern.darkPeriod = m_PrintScripts.DarkTimeScriptList.at(CurrentImage).toInt() * 1000;
                    printf("%d, %d \r\n", pattern.exposure, pattern.darkPeriod);
                    CurrentImage++; //Should this be done before?
                }
                else
                {
                    pattern.exposure = m_PrintSettings.ExposureTime;
                    pattern.darkPeriod = m_PrintSettings.DarkTime;
                }
                pattern.trigIn = false;
                pattern.trigOut2 = true;
                pattern.splashImageBitPos = 0;
                pattern.splashImageIndex = 0;
                pattern.clear = true;
                BitPos += m_PrintSettings.BitMode;
            }
            else
            {
                pattern.bits = m_PrintSettings.BitMode;
                pattern.color = PatternElement::BLUE;
                if (m_PrintScripts.PrintScript == ON)
                {
                    pattern.exposure = m_PrintScripts.ExposureScriptList.at(CurrentImage).toInt() * 1000; //*1000 to get from ms to us
                    pattern.darkPeriod = m_PrintScripts.DarkTimeScriptList.at(CurrentImage).toInt() * 1000;
                    printf("%d, %d \r\n", pattern.exposure, pattern.darkPeriod);
                    CurrentImage++; //Should this be done before?
                }
                else
                {
                    pattern.exposure = m_PrintSettings.ExposureTime;
                    pattern.darkPeriod = m_PrintSettings.DarkTime;
                }
                pattern.trigIn = false;
                pattern.trigOut2 = true;
                pattern.clear = true;
                pattern.splashImageIndex = 0;
                if (m_PrintControls.InitialExposureFlag){
                    pattern.splashImageBitPos = 0;
                }
                else{
                    pattern.splashImageBitPos = m_elements[m_elements.size()-1].splashImageBitPos + m_elements[m_elements.size()-1].bits;
                    if (BitPos > 23){
                        BitPos = 0;
                    }
                    printf(",BP: %d,", BitPos);
                    pattern.splashImageBitPos = BitPos;
                    BitPos += m_PrintSettings.BitMode;
                    printf("BitPos: %d \r\n", pattern.splashImageBitPos);
                }
            }
            pattern.selected = true;
            m_elements.append(pattern);
            numPatAdded++;
            //m_patternImageChange = true;
        }
        printf("%d Patterns Added",numPatAdded);
    }
    else //POTF mode
    {
        printf("POTF pattern upload\r\n");
        if(fileNames.isEmpty())
        {
            emit DLPError("No image file found");
            return;
        }

        //fileNames.sort();

        QDir dir = QFileInfo(QFile(fileNames.at(0))).absoluteDir();
        m_ptnImagePath = dir.absolutePath();

        for(i=0;i<m_elements.size();i++)
            m_elements[i].selected=false;

        for(i = 0; i < fileNames.size(); i++)
        {
            PatternElement pattern;

            if(m_elements.size()==0)
            {
                pattern.bits = m_PrintSettings.BitMode;
                pattern.color = PatternElement::BLUE;
                if (m_PrintScripts.PrintScript == ON)
                {
                    pattern.exposure = m_PrintScripts.ExposureScriptList.at(CurrentImage).toInt() * 1000; //*1000 to get from ms to us
                    pattern.darkPeriod = m_PrintScripts.DarkTimeScriptList.at(CurrentImage).toInt() * 1000;
                    printf("%d, %d \r\n", pattern.exposure, pattern.darkPeriod);
                    CurrentImage++; //Should this be done before?
                }
                else
                {
                    pattern.exposure = m_PrintSettings.ExposureTime;
                    pattern.darkPeriod = m_PrintSettings.DarkTime;
                }
                pattern.trigIn = false;
                pattern.trigOut2 = true;
                pattern.clear = true;

            }
            else
            {
                pattern.bits = m_PrintSettings.BitMode;
                pattern.color = PatternElement::BLUE;
                if (m_PrintScripts.PrintScript == ON)
                {
                    pattern.exposure = m_PrintScripts.ExposureScriptList.at(CurrentImage).toInt() * 1000; //*1000 to get from ms to us
                    pattern.darkPeriod = m_PrintScripts.DarkTimeScriptList.at(CurrentImage).toInt() * 1000;
                    printf("%d, %d \r\n", pattern.exposure, pattern.darkPeriod);
                    CurrentImage++; //Should this be done before?
                }
                else
                {
                    pattern.exposure = m_PrintSettings.ExposureTime;
                    pattern.darkPeriod = m_PrintSettings.DarkTime;
                }
                pattern.trigIn = false;
                pattern.trigOut2 = true;
                pattern.clear = true;

                    pattern.splashImageIndex = m_elements[m_elements.size()-1].splashImageIndex;
                    pattern.splashImageBitPos = m_elements[m_elements.size()-1].splashImageBitPos;

            }


            pattern.name = fileNames.at(i);
            printf(pattern.name.toLatin1());
            printf("\r\n");
            pattern.selected = true;

            m_elements.append(pattern);
            numPatAdded++;
            //m_patternImageChange = true;
        }
    }
#if 0
    if (m_elements.size() > 0)
    {
        waveWindow->updatePatternList(m_elements);
        waveWindow->draw();
        ui->zoomSlider->setEnabled(true);
        ui->selectAllButton->setEnabled(true);
        ui->ptnSetting_groupBox->setEnabled(true);
        ui->clearPattern_checkbox->setEnabled(true);
        ui->removePatternsButton->setEnabled(true);
        on_patternSelect(m_elements.size()-1,m_elements);
        isPatternLoaded = TRUE;
        ui->editLUT_Button->setEnabled(TRUE);
    }
#endif

    //updatePtnCheckbox();
    return;
}
/***************Helper Functions**************************/

/*!
 * \brief DLP9000::UpdatePatternMemory
 * Creates Splash images from all the Pattern elements.
 * Converts the splash images to splash blocks and uploads pattern images.
 * \param totalSplashImages  - Total number of Splash images to be uploaded
 * \return - Returns -1 for error and 0 for successful upload
 */
int DLP9000::UpdatePatternMemory(int totalSplashImages)
{
    for(int image = 0; image < totalSplashImages; image++)
    {

        int splashImageCount;
        splashImageCount = totalSplashImages - 1 - image;

        PtnImage merge_image(m_ptnWidth, m_ptnHeight,24, PTN_RGB24);

        merge_image.fill(0);

        int i;
        int es = m_elements.size();
        for(i = 0; i < es; i++)
        {
            int ei = m_elements[i].splashImageIndex;
            if(ei != splashImageCount)
                continue;
            int bitpos = m_elements[i].splashImageBitPos;
            printf("UPbp: %d ", m_elements[i].splashImageBitPos);
            int bitdepth = m_elements[i].bits;
            PtnImage image(m_elements[i].name);
            merge_image.merge(image,bitpos,bitdepth);

        }
        printf("\r\n");
        merge_image.swapColors(PTN_COLOR_RED, PTN_COLOR_BLUE, PTN_COLOR_GREEN);
        uint08* splash_block = NULL;

        PtnImage merge_image_master(m_ptnWidth, m_ptnHeight,24, PTN_RGB24);
        PtnImage merge_image_slave(m_ptnWidth, m_ptnHeight,24, PTN_RGB24);
        merge_image_master = merge_image;
        merge_image_slave = merge_image;


            merge_image_master.crop(0, 0, m_ptnWidth/2, m_ptnHeight);
            merge_image_slave.crop(m_ptnWidth/2, 0, m_ptnWidth/2, m_ptnHeight);


                uint08* splash_block_master = NULL;
                uint08* splash_block_slave = NULL;

                int splashSizeMaster  = merge_image_master.toSplash(&splash_block_master,SPL_COMP_AUTO);
                int splashSizeSlave = merge_image_slave.toSplash(&splash_block_slave,SPL_COMP_AUTO);

                if(splashSizeMaster <= 0 || splashSizeSlave <= 0)
                {
                    emit DLPError("splashSize <= 0");
                    return -1;
                }

                if(uploadPatternToEVM(true, splashImageCount, splashSizeMaster, splash_block_master) == -1)
                {
                    emit DLPError("Master Upload Pattern to EVM failed");
                    return -1;
                }

                if(uploadPatternToEVM(false, splashImageCount, splashSizeSlave, splash_block_slave) == -1)
                {
                    emit DLPError("Slave Upload Pattern to EVM failed");
                    return -1;
                }
    }
    return 0;
}

/**
 * @brief DLP9000::uploadPatternToEVM
 * Updates the Pattern images into the Splash block on the Firmware image in the EVM on the fly
 * @param master boolean to indicate if it is madetr or slave
 * @param splashImageCount - the Index of the Splash Image to be updated
 * @param splash_size - size of the splash image that is being updated
 * @param splash_block - the updated splash block
 * @return
 */
int DLP9000::uploadPatternToEVM(bool master, int splashImageCount, int splash_size, uint08* splash_block)
{
    int origSize = splash_size;
    LCR_InitPatternMemLoad(master, splashImageCount, splash_size);

    QProgressDialog imgDataDownload("Image data download", "Abort", 0, splash_size);
    imgDataDownload.setWindowTitle(QString("Pattern Data Download.."));
    imgDataDownload.setWindowModality(Qt::WindowModal);
    imgDataDownload.setLabelText(QString("Uploading to EVM"));
    imgDataDownload.setValue(0);
    int imgDataDwld = 0;
    imgDataDownload.setMaximum(origSize);
    imgDataDownload.show();
    //QApplication::processEvents();
    while(splash_size > 0)
    {
        int dnldSize = LCR_pattenMemLoad(master, splash_block + (origSize - splash_size), splash_size);
        if (dnldSize < 0)
        {
            // free(imageBuffer);
            //showCriticalError("Downloading failed");
            //usbPollTimer->start();
            imgDataDownload.close();
            if (master)
            {
               emit DLPError("Master Downloading Failed");
            }
            else
            {
                emit DLPError("Slave Downloading Failed");
            }
            return -1;
        }

        splash_size -= dnldSize;

        if (splash_size < 0)
            splash_size = 0;

        imgDataDwld += dnldSize;
        imgDataDownload.setValue(imgDataDwld);
         //QApplication::processEvents();
        if(imgDataDownload.wasCanceled())
        {
            imgDataDownload.setValue(splash_size);
            imgDataDownload.close();
            emit DLPError("imgDataDownLoad was canceled");
            return -1;
        }
    }

    //QApplication::processEvents();
    imgDataDownload.close();

    return 0;

}

/*!
 * \brief DLP9000::updateLUT
 * Handler function for uploading LUT data to light engine
 * \param ProjectionMode Tells the function whether to format for POTF or video pattern mode
 */
void DLP9000::updateLUT(int ProjectionMode)
{
    int totalSplashImages = 0;
    int ret;
    char errStr[255];

    if(m_elements.size() <= 0)
    {

        emit DLPError("No pattern sequence to send");
        printf("Error: No pattern sequence to send");
        return;
    }

    if (calculateSplashImageDetails(&totalSplashImages,FALSE, ProjectionMode))
        return;

    LCR_ClearPatLut();

    for(int i = 0; i < m_elements.size(); i++)
    {
        printf("Update BitPos: %d", m_elements[i].splashImageBitPos);
        if(LCR_AddToPatLut(i, m_elements[i].exposure, m_elements[i].clear, m_elements[i].bits, m_elements[i].color, m_elements[i].trigIn, m_elements[i].darkPeriod, m_elements[i].trigOut2, m_elements[i].splashImageIndex, m_elements[i].splashImageBitPos)<0)
        {
            sprintf(errStr,"Unable to add pattern number %d to the LUT",i);
            emit DLPError(QString::fromLocal8Bit(errStr));
            break;
        }
        else
        {
            printf("i:%d, exp:%d \r\n",i,m_elements[i].exposure);
        }
    }

    if (LCR_SendPatLut() < 0)
    {
        emit DLPError("Sending pattern LUT failed!");
        //printf("Sending pattern LUT failed");
        return;
    }

    ret = LCR_SetPatternConfig(m_elements.size(), m_elements.size());
    printf("elements size: %d\r\n",m_elements.size());
    if (ret < 0)
    {
        emit DLPError("Sending pattern LUT size failed!");
        return;
    }

    //if (ui->patternMemory_radioButton->isChecked() && m_patternImageChange)
    //{
        if(UpdatePatternMemory(totalSplashImages) == 0)
        {
              printf("Total splash images: %d",totalSplashImages);
      //      m_patternImageChange = false;
        }
        else
        {
            printf("UpdatePatternMemory Failed");
        }
    //}
}

/*!
 * \brief DLP9000::startPatSequence
 * Start pattern display on the light engine, emits an error
 * if unable to start pattern display.
 */
void DLP9000::startPatSequence(void)
{
    if (LCR_PatternDisplay(2) < 0)
    {
        emit DLPError("Unable to start pattern display");
    }
}

/*!
 * \brief DLP9000::clearElements
 * Simple function to clear local program pattern memory.
 */
void DLP9000::clearElements(void)
{
    m_elements.clear();
}

/*!
 * \brief DLP9000::setIT6535Mode
 * Sets the mode of the IT6535 receiver, options include:
 * Disabled, HDMI, and DisplayPort.
 * \param Mode - The mode to set the IT6535 to
 */
void DLP9000::setIT6535Mode(int Mode)
{
    unsigned int dataPort, pixelClock, dataEnable, syncSelect;
    dataPort = 0; //Select data port 1
    pixelClock = 0; //Select pixelClock 1
    dataEnable = 0; //Select dataEnable 1
    syncSelect = 0;  //Select port 1 sync

    if(LCR_SetPortConfig(dataPort,pixelClock,dataEnable,syncSelect)<0)
      emit DLPError("Unable to set port configuration!");
    API_VideoConnector_t ProjectMode;
    switch (Mode){
        case 0:
            ProjectMode = VIDEO_CON_DISABLE;
            break;
        case 1:
            ProjectMode = VIDEO_CON_HDMI;
            break;
        case 2:
            ProjectMode = VIDEO_CON_DP;
            break;
        default:
            ProjectMode = VIDEO_CON_DISABLE; //do nothing
            break;
    }
    LCR_SetIT6535PowerMode(ProjectMode);
}

/*!
 * \brief DLP9000::calculateSplashImageDetails
 * For each of the pattern image on the pattern settings page, calculates the
 * total number of splash images of bit depth 24 based on the bit depth of each image
 * Also calculates the bitposition of each pattern element in the splash Image
 * and the index of the Splash image for each Pattern element.
 * \param totalSplashImages - Total number of splash images to be created from
 *                                the available Pattern images
 * \param ProjectionMode - Determines whether to format for POTF or video pattern mode
 * \return - 0 - success
 *          -1 - failure
 */
int DLP9000::calculateSplashImageDetails(int *totalSplashImages, bool firmware, int ProjectionMode)
{
    int maxbits = 400;
    if(ProjectionMode == 1){
        maxbits = 5000;
    }
    int imgCount = 0;
    int bits = 0;
    int totalBits = 0;
    int myBits;
    for(int elemCount = 0; elemCount < m_elements.size(); elemCount++)
    {
        if (ProjectionMode == 1){
            myBits = m_elements[elemCount].splashImageBitPos;
        }
        if (m_elements[elemCount].bits > 16)
        {
            char dispStr[255];
            sprintf(dispStr, "Error:Bit depth not selected for pattern=%d\n", elemCount);
            //showStatus(dispStr);
            return -1;
        }

        totalBits = totalBits + m_elements[elemCount].bits;

        if(firmware == TRUE)
            maxbits = 3984;
        if(totalBits > maxbits)
        {
            char dispStr[255];
            if(firmware == FALSE)
                sprintf(dispStr, "Error:Total Bit Depth cannot exceed 400");
            else
                sprintf(dispStr, "Error:Total Bit Depth cannot exceed 3984");
            emit DLPError(dispStr);
            return -1;
        }

        /* Check if the same pattern is used already */
        int i;
        for(i = 0; i < elemCount; i++)
        {
            /* Only if file name and bit depth matches */
            if(m_elements[i].bits == m_elements[elemCount].bits &&
                    m_elements[i].name == m_elements[elemCount].name)
            {
                break;
            }
        }

        /* Match found. use the same splash image */
        if(i < elemCount)
        {
            m_elements[elemCount].splashImageIndex = m_elements[i].splashImageIndex;
            if (ProjectionMode == 1){
                m_elements[elemCount].splashImageBitPos = myBits;
                printf("myBits: %d\r\n", m_elements[elemCount].splashImageBitPos);
            }
            else{
                m_elements[elemCount].splashImageBitPos = m_elements[i].splashImageBitPos;
            }
            continue;
        }

        /* If it is the last image or cant fit in the current image */
        if(elemCount == m_elements.size() ||
                (bits + m_elements[elemCount].bits) > 24)
        {
            /* Goto next image */
            imgCount++;
            bits = 0;
        }

        m_elements[elemCount].splashImageIndex = imgCount;
        m_elements[elemCount].splashImageBitPos = bits;
        if (ProjectionMode == 1){
            m_elements[elemCount].splashImageBitPos = myBits;
            printf("myBits: %d\r\n", m_elements[elemCount].splashImageBitPos);
        }
        else{
            m_elements[elemCount].splashImageBitPos = bits;
        }
        bits += m_elements[elemCount].bits;
    }

    *totalSplashImages = imgCount + 1;

    return 0;
}

void DLP9000::SetLEDIntensity(int UVIntensity, int PrintScript, QStringList LEDScriptList)
{
    //Set LED currents to 0 red, 0 green, set blue to chosen UVIntensity
    if (PrintScript == ON){ //If printscript is on
        LCR_SetLedCurrents(0,0,(LEDScriptList.at(0).toInt())); //Set LED intensity to first value in printscript
    }
    else{ //if printscript is off
        LCR_SetLedCurrents(0, 0, UVIntensity); //set static LED intensity
    }
}

/*!
 * \brief DLP9000::PatternUpload
 * Handler function for pattern uploads
 * \param ImageList - List of image files to be uploaded
 * \param dlp_PrintControls - Checks nSlice for sanity check, passed along to the AddPatterns function
 * \param dlp_PrintSettings - Passed along to the AddPatterns function
 * \param dlp_PrintScript - Passed along to the AddPatterns function
 * \return
 */
int DLP9000::PatternUpload(QStringList ImageList, PrintControls dlp_PrintControls, PrintSettings dlp_PrintSettings, PrintScripts dlp_PrintScript)
{
    int nPatterns = ImageList.count();
    LCR_PatternDisplay(0);
    if (dlp_PrintControls.nSlice > 0)
    {
        AddPatterns(ImageList, dlp_PrintSettings, dlp_PrintScript, dlp_PrintControls);
        updateLUT(dlp_PrintSettings.ProjectionMode);
        clearElements();
    }
    return nPatterns;
}

/*!
 * \brief DLP9000::PatternDisplay
 * Sets whether to start, stop, or pause pattern display
 * \param DisplaySetting
 */
void DLP9000::PatternDisplay(int DisplaySetting)
{
    LCR_PatternDisplay(DisplaySetting);
}




