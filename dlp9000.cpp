#include "dlp9000.h"
#include "API.h"
#include "mainwindow.h"
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

//static MainWindow Main;

bool DLP9000::InitProjector(void)
{
    if (USB_Open() == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}



/**
 * @brief MainWindow::on_addPatternsButton_clicked
 */
//Make sure that input comes from
void DLP9000::AddPatterns(QStringList fileNames, double ExposureTime, double DarkTime, int UVIntensity)
{
    int i;
    int numPatAdded = 0;


        if(fileNames.isEmpty())
            return;

        fileNames.sort();

        QDir dir = QFileInfo(QFile(fileNames.at(0))).absoluteDir();
        m_ptnImagePath = dir.absolutePath();
    //    settings.setValue("PtnImagePath",m_ptnImagePath);

        for(i=0;i<m_elements.size();i++)
            m_elements[i].selected=false;

        for(i = 0; i < fileNames.size(); i++)
        {
            PatternElement pattern;

            if(m_elements.size()==0)
            {
                pattern.bits = 1;
                pattern.color = PatternElement::RED;
                pattern.exposure = ExposureTime;
                pattern.darkPeriod = DarkTime;
                pattern.trigIn = false;
                pattern.trigOut2 = true;
                pattern.clear = true;

            }
            else
            {
                pattern.bits = m_elements[m_elements.size()-1].bits;
                pattern.color = m_elements[m_elements.size()-1].color;
                pattern.exposure = m_elements[m_elements.size()-1].exposure;
                pattern.darkPeriod = m_elements[m_elements.size()-1].darkPeriod;
                pattern.trigIn = m_elements[m_elements.size()-1].trigIn;
                pattern.trigOut2 = m_elements[m_elements.size()-1].trigOut2;
                pattern.clear = m_elements[m_elements.size()-1].clear;

                    pattern.splashImageIndex = m_elements[m_elements.size()-1].splashImageIndex;
                    pattern.splashImageBitPos = m_elements[m_elements.size()-1].splashImageBitPos;

            }


            pattern.name = fileNames.at(i);
            pattern.selected = true;

            m_elements.append(pattern);
            numPatAdded++;
            //m_patternImageChange = true;
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

/**
 * @brief MainWindow::UpdatePatternMemory
 * Creates Splash images from all the Pattern elements
 * If it is Firmware update, adds the splash images to the firmware
 * If on the fly, converts the splash images to splash blocks and updates on teh fly
 * @param totalSplashImages - I - total number of Splash images to be updated in Firmware
 * @param firmware - I - boolean to determine if it is to update firmware or On the Fly mode
 * @return
 */
int DLP9000::UpdatePatternMemory(int totalSplashImages, bool firmware )
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
            int bitdepth = m_elements[i].bits;
            PtnImage image(m_elements[i].name);
            merge_image.merge(image,bitpos,bitdepth);
        }

        merge_image.swapColors(PTN_COLOR_RED, PTN_COLOR_BLUE, PTN_COLOR_GREEN);
        uint08* splash_block = NULL;

        PtnImage merge_image_master(m_ptnWidth, m_ptnHeight,24, PTN_RGB24);
        PtnImage merge_image_slave(m_ptnWidth, m_ptnHeight,24, PTN_RGB24);
        merge_image_master = merge_image;
        merge_image_slave = merge_image;
        if (m_dualAsic)
        {

            merge_image_master.crop(0, 0, m_ptnWidth/2, m_ptnHeight);
            merge_image_slave.crop(m_ptnWidth/2, 0, m_ptnWidth/2, m_ptnHeight);


                uint08* splash_block_master = NULL;
                uint08* splash_block_slave = NULL;

                int splashSizeMaster  = merge_image_master.toSplash(&splash_block_master,SPL_COMP_AUTO);
                int splashSizeSlave = merge_image_slave.toSplash(&splash_block_slave,SPL_COMP_AUTO);

                if(splashSizeMaster <= 0 || splashSizeSlave <= 0)
                    return -1;

                if(uploadPatternToEVM(true, splashImageCount, splashSizeMaster, splash_block_master) == -1)
                    return -1;

                if(uploadPatternToEVM(false, splashImageCount, splashSizeSlave, splash_block_slave) == -1)
                    return -1;
        }
        else
        {
                int splashSize = merge_image.toSplash(&splash_block,SPL_COMP_AUTO);
                if(splashSize <= 0)
                    return -1;
                if(uploadPatternToEVM(true, splashImageCount, splashSize, splash_block) < 0)
                    return -1;
        }
    }


    return 0;
}

/**
 * @brief MainWindow::UpdateLUTOnTheFly
 * Updates the Pattern images into the Splash block on the Firmware image in the EVM on the fly
 * @param master - I - boolean to indicate if it is madetr or slave
 * @param splashImageCount - I - the Index of the Splash Image to be updated
 * @param splash_size - I - size of the splash image that is being updated
 * @param splash_block - I - the updated splash block
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
            return -1;
        }
    }

    //QApplication::processEvents();
    imgDataDownload.close();

    return 0;

}




/**
 * @brief MainWindow::on_updateLUT_Button_clicked
 */
void DLP9000::updateLUT()
{
    int totalSplashImages = 0;
    int ret;
    QTime waitEndTime;
    char errStr[255];
    MainWindow Main;

    if(m_elements.size() <= 0)
    {

        Main.showError("No pattern sequence to send");
        printf("Error: No pattern sequence to send");
        return;
    }

    LCR_ClearPatLut();

    for(int i = 0; i < m_elements.size(); i++)
    {
        if(LCR_AddToPatLut(i, m_elements[i].exposure, m_elements[i].clear, m_elements[i].bits, m_elements[i].color, m_elements[i].trigIn, m_elements[i].darkPeriod, m_elements[i].trigOut2, m_elements[i].splashImageIndex, m_elements[i].splashImageBitPos)<0)
        {
            sprintf(errStr,"Unable to add pattern number %d to the LUT",i);
            Main.showError(QString::fromLocal8Bit(errStr));
            break;
        }
        else
        {
            printf("i:%d\r\n exp:%d",i,m_elements[i].exposure);
        }
    }

    if (LCR_SendPatLut() < 0)
    {
        Main.showError("Sending pattern LUT failed!");
        printf("Sending pattern LUT failed");
        return;
    }

    ret = LCR_SetPatternConfig(m_elements.size(), m_elements.size());

    if (ret < 0)
    {
        Main.showError("Sending pattern LUT size failed!");
        return;
    }

    //if (ui->patternMemory_radioButton->isChecked() && m_patternImageChange)
    //{
        if(UpdatePatternMemory(totalSplashImages, false) == 0)
        {
      //      m_patternImageChange = false;
        }
        else
        {
            printf("UpdatePatternMemory Failed");
        }
    //}
}

/**
 * @brief MainWindow::on_startPatSequence_Button_clicked
 */
void startPatSequence()
{
    MainWindow Main;

    if (LCR_PatternDisplay(0x2) < 0)
        Main.showError("Unable to start pattern display");
}


