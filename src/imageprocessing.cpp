#include "imageprocessing.h"
#include "ui_imageprocessing.h"
#include <QFileDialog>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <time.h>
#include <windows.h>
#include <math.h>

using namespace cv;
using std::cout;
cv::Mat src, src_gray, dst;
cv::Mat src1, src2, src3;
cv::Mat workingImage, workingGray, workingBinary, bit8Image, encodedImage;
static int remainingImages;
static int encode24Count = 1;
static int encodeCount = 0;
int grayEncodeCount = 1;

QString TargetDestination = "C://";
static int rowDim;
static int colDim;


imageprocessing::imageprocessing(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::imageprocessing)
{
    ui->setupUi(this);
}

imageprocessing::~imageprocessing()
{
    delete ui;
}


void imageprocessing::on_SelectImageFiles_clicked()
{
    QStringList file_name = QFileDialog::getOpenFileNames(this,"Open Object Image Files","C://Users/le54t/Documents/Stanford/Research/testImages","*.bmp *.png *.tiff *.tif *.svg");
    if (file_name.count() > 0) //If images were selected
    {
        //QDir ImageDirectory = QFileInfo(file_name.at(0)).absoluteDir();
        //ImageFileDirectory = ImageDirectory.filePath(file_name.at(0));
        for (uint16_t i = 0; i < file_name.count(); i++)
        {
            ui->InputList->addItem(file_name.at(i)); //Add each file name to FileList, will be displayed for user
        }
        //ui->InputList->sortItems();
        remainingImages = ui->InputList->count();
        ui->encodeProgress->setMaximum(remainingImages);

        QListWidgetItem* filename = ui->InputList->item(0);
        QString file = filename->text();
        src = imread( samples::findFile (file.toUtf8().constData()), IMREAD_COLOR);
        colDim = src.cols;
        rowDim = src.rows;
    }
}

void imageprocessing::on_ClearImageFiles_clicked()
{
    ui->InputList->clear();
}

void imageprocessing::on_BrowseTargetDir_clicked()
{
    TargetDestination = QFileDialog::getExistingDirectory(this, "Open Log File Destination");
    ui->TargetDirLabel->setText(TargetDestination);
}


void imageprocessing::on_EncodeTest_clicked()
{
    while (remainingImages > 0)
    {
        bitEncode24();
        ui->encodeProgress->setValue(encodeCount);
        imageprocessing::update();
    }
    encodeCount = 0; //reset encodeCount
}

void imageprocessing::on_GreyscaleEncode_clicked()
{
    while (remainingImages > 0)
    {
        Mat channel[3];
        split(src,channel); //Channels are split from actual image to retain properties of images being  handled
        channel[0], channel[1], channel[2] = Scalar::all(0); //Clear out image channels to remove any potential artifacts
        Mat ImageOut;

        if(remainingImages > 0){
            QListWidgetItem* filename = ui->InputList->item(encodeCount); //Select image
            QString file_name = filename->text(); //convert to string to read by openCV
            Mat bit8read, bit8gray; //Prep working Mats
            bit8read = imread( samples::findFile (file_name.toUtf8().constData()), IMREAD_COLOR); //read image into workingImage mat
            cvtColor(bit8read, bit8gray, COLOR_BGR2GRAY); //convert image to grayscale
            channel[1] = bit8gray.clone();
            encodeCount++;
            remainingImages--;
        }

        if(remainingImages > 0){
            QListWidgetItem* filename = ui->InputList->item(encodeCount); //Select image
            QString file_name = filename->text(); //convert to string to read by openCV
            Mat bit8read, bit8gray; //Prep working Mats
            bit8read = imread( samples::findFile (file_name.toUtf8().constData()), IMREAD_COLOR); //read image into workingImage mat
            cvtColor(bit8read, bit8gray, COLOR_BGR2GRAY); //convert image to grayscale
            channel[2] = bit8gray.clone();
            encodeCount++;
            remainingImages--;
        }

        if(remainingImages > 0){
            QListWidgetItem* filename = ui->InputList->item(encodeCount); //Select image
            QString file_name = filename->text(); //convert to string to read by openCV
            Mat bit8read, bit8gray; //Prep working Mats
            bit8read = imread( samples::findFile (file_name.toUtf8().constData()), IMREAD_COLOR); //read image into workingImage mat
            cvtColor(bit8read, bit8gray, COLOR_BGR2GRAY); //convert image to grayscale
            channel[0] = bit8gray.clone();
            encodeCount++;
            remainingImages--;
        }

        merge(channel,3,ImageOut);
        QPixmap newImage = QPixmap::fromImage(QImage((unsigned char*) ImageOut.data, ImageOut.cols, ImageOut.rows, QImage::Format_BGR888));
        ui->ImageDisplay->setPixmap(newImage.scaled(853,533));

        QString ImageName = TargetDestination + "/" + QString::number(grayEncodeCount) + ".tiff";
        imwrite(ImageName.toUtf8().constData(), ImageOut);
        ui->OutputImageList->addItem(ImageName);
        //imshow("Test", ImageOut);
        grayEncodeCount++;
        imageprocessing::update();
    }
}

void imageprocessing::bitEncode24()
{
    Mat channel[3];
    split(src,channel); //Channels are split from actual image to retain properties of images being  handled
    channel[0] = Scalar::all(0);
    channel[1] = Scalar::all(0);
    channel[2] = Scalar::all(0); //Clear out image channels to remove any potential artifacts
    Mat ImageOut;

    if (remainingImages > 0)
    {
        QListWidgetItem* filename1 = ui->InputList->item(encodeCount);
        QString file1 = filename1->text();
        src1 = imread( samples::findFile (file1.toUtf8().constData()), IMREAD_COLOR);
        src1 = Scalar::all(0);
        ui->TerminalOut->append("Green");
        bitEncode8(src1, channel[1]);
    }
    if (remainingImages > 0)
    {
        QListWidgetItem* filename2 = ui->InputList->item(encodeCount);
        QString file2 = filename2->text();
        src2 = imread( samples::findFile (file2.toUtf8().constData()), IMREAD_COLOR);
        src2 = Scalar::all(0);
        ui->TerminalOut->append("Red");
        bitEncode8(src2, channel[2]);
    }
    if (remainingImages > 0)
    {
        QListWidgetItem* filename3 = ui->InputList->item(encodeCount);
        QString file3 = filename3->text();
        src3 = imread( samples::findFile (file3.toUtf8().constData()), IMREAD_COLOR);
        src3 = Scalar::all(0);
        ui->TerminalOut->append("Blue");
        bitEncode8(src3, channel[0]);
    }

    merge(channel,3,ImageOut);
    QPixmap newImage = QPixmap::fromImage(QImage((unsigned char*) ImageOut.data, ImageOut.cols, ImageOut.rows, QImage::Format_BGR888));
    ui->ImageDisplay->setPixmap(newImage.scaled(853,533));

    QString ImageName = TargetDestination + "/" + QString::number(encode24Count) + ".tiff";
    imwrite(ImageName.toUtf8().constData(), ImageOut);
    ui->OutputImageList->addItem(ImageName);
    encode24Count++;
    //imshow("Test", ImageOut);

}

//8 bit encoding, called multiple times atm, possible improvements to be made but had weird memory issues
void imageprocessing::bitEncode8(Mat source, Mat& Channel)
{
    cv::Mat workingChannel (source.rows, source.cols, CV_8UC1); //Initialize Channel for return, might not be optimal, but at least it works
    workingChannel = Scalar::all(0);
    bool MaskFlag = true;
    for (uint i = 0; i < 8; i++) //Repeats loop 8 times for 8 bit-layers
    {
        if (remainingImages > 0) //Make sure that there are still images remaining
        {
             QListWidgetItem* filename = ui->InputList->item(encodeCount); //Select image
             QString file_name = filename->text(); //convert to string to read by openCV
             Mat bit8read, bit8gray, bit8binary; //Prep working Mats
             bit8read = imread( samples::findFile (file_name.toUtf8().constData()), IMREAD_COLOR); //read image into workingImage mat
             cvtColor(bit8read, bit8gray, COLOR_BGR2GRAY); //convert image to grayscale
             threshold(bit8gray, bit8binary, 0, 255, 0); //threshold image to binary
             int encodeVal = hexSelect(i); //get correct hexval for current bit layer
             ui->TerminalOut->append("encodeVal: " + QString::number(encodeVal));

             for (int row = 0; row < bit8binary.rows; row++)
             {
                for (int col = 0; col < bit8binary.cols; col++)
                {
                    if (bit8binary.at<uchar>(row,col) > 0)
                    {
                        bit8binary.at<uchar>(row,col) = encodeVal;
                    }
                }
             }

             for (int row = 0; row < workingChannel.rows; row++)
             {
                for (int col = 0; col < workingChannel.cols; col++)
                {
                        workingChannel.at<uchar>(row,col) = workingChannel.at<uchar>(row,col) | bit8binary.at<uchar>(row,col);
                }
             }

             encodeCount++;
             remainingImages--;
        }
        else
        {
            uint Mask;
            if(MaskFlag)
            {
                MaskFlag = false;
                Mask = pow(2, (i)) - 1;
                ui->TerminalOut->append("No more images remaining, Mask: " + QString::number(Mask));
            }
            else
            {
                ui->TerminalOut->append("No more images remaining");
            }
            for (int row = 0; row < workingChannel.rows; row++)
            {
               for (int col = 0; col < workingChannel.cols; col++)
               {
                       workingChannel.at<uchar>(row,col) = workingChannel.at<uchar>(row,col) & Mask;
               }
            }
        }
    }
    Channel = (source.rows, source.cols, CV_8UC1, workingChannel).clone();
#if 0
    uchar testbit = 0xff;
    for (int row = 0; row < workingChannel.rows; row++)
    {
       for (int col = 0; col < workingChannel.cols; col++)
       {
           if ((workingChannel.at<uchar>(row,col) & testbit) > 0)
           {
               workingChannel.at<uchar>(row,col) = 0xff;
           }
           else
           {
               workingChannel.at<uchar>(row,col) = 0;
           }
       }
    }
    imshow("test", workingChannel);
#endif
    workingChannel.release();
}

//For selecting correct hex value to encode
int imageprocessing::hexSelect(int imageNum)
{
    int returnVal = 0;
    switch(imageNum)
    {
        case 0:
            returnVal = 0x01; //0000 0001 = 0x01
            break;
        case 1:
            returnVal = 0x02; //0000 0010 = 0x02
            break;
        case 2:
            returnVal = 0x04; //0000 0100 = 0x04
            break;
        case 3:
            returnVal = 0x08; //0000 1000 = 0x08
            break;
        case 4:
            returnVal = 0x10; //0001 0000 = 0x10
            break;
        case 5:
            returnVal = 0x20; //0010 0000 = 0x20
            break;
        case 6:
            returnVal = 0x40; //0100 0000 = 0x40
            break;
        case 7:
            returnVal = 0x80; //1000 0000 = 0x80
            break;
        default:
            break;
    }
    return returnVal;
}

#if 0
void imageprocessing::on_DisplayImage_clicked()
{
    QListWidgetItem* filename = ui->InputList->item(0);
    QString file_name = filename->text();
    QPixmap img(file_name);
    QPixmap img2 = img.scaled(1300,800, Qt::KeepAspectRatio);
    ui->ImageDisplay->setPixmap(img2);
}


void imageprocessing::on_pushButton_clicked()
{
    QListWidgetItem* filename = ui->InputList->item(0);
    QString file_name = filename->text();
    src = imread( samples::findFile (file_name.toUtf8().constData()), IMREAD_COLOR);
    cvtColor(src, src_gray, COLOR_BGR2GRAY);
    threshold(src_gray, dst, 0, 250, 0);
    //QPixmap thresholdTest = QPixmap::fromImage(QImage((unsigned char*) dst.data, dst.cols, dst.rows, QImage::Format_Indexed8));
     QPixmap thresholdTest = QPixmap::fromImage(QImage((unsigned char*) src.data, src.cols, src.rows, QImage::Format_BGR888));
    QPixmap thresholdScale = thresholdTest.scaled(1300,800, Qt::KeepAspectRatio);
    ui->ImageDisplay->setPixmap(thresholdScale);
}

void imageprocessing::on_ClearImageFiles_clicked()
{
    ui->InputList->clear();
}

static int SplitCount = 0;
static cv::Mat FullSplit[24];

void imageprocessing::on_Split_clicked()
{
    if (SplitCount == 0)
    {
        static QListWidgetItem* filename = ui->InputList->item(0);
        static QString file_name = filename->text();
        src = imread( samples::findFile (file_name.toUtf8().constData()), IMREAD_COLOR);
        split(src,FullSplit);
    }
    QPixmap SplitTest = QPixmap::fromImage(QImage((unsigned char*) FullSplit[SplitCount].data, FullSplit[SplitCount].cols, FullSplit[SplitCount].rows, QImage::Format_Indexed8));
    ui->ImageDisplay->setPixmap(SplitTest.scaled(1300,800, Qt::KeepAspectRatio));

    SplitCount++;
}

void imageprocessing::on_BinarySplit_clicked()
{
    emit(on_pushButton_clicked());

    uchar * array = dst.isContinuous()? dst.data: dst.clone().data;
    uint length = dst.total()*dst.channels();
    for (uint i = 0; i < length; i++)
    {
        if (array[i] > 0)
        {
            //int intToPrint = array[i];
            array[i] = 0xFF;
            //ui->TerminalOut->append("data is " + QString::number(intToPrint) + " at " + QString::number(i));
            //break;
        }
    }
    cv::Mat A(dst.rows, dst.cols, CV_8U);
    A.data = array;
    //std::memcpy(A.data, array, )
    QPixmap EncodeTest = QPixmap::fromImage(QImage((unsigned char*) A.data, A.cols, A.rows, QImage::Format_Indexed8));
    ui->ImageDisplay->setPixmap(EncodeTest.scaled(1300,800, Qt::KeepAspectRatio));
}

void imageprocessing::bitEncode24()
{
    bool firstLayer = true;
    QListWidgetItem* filename = ui->InputList->item(encodeCount);
    QString file = filename->text();
    src = imread( samples::findFile (file.toUtf8().constData()), IMREAD_COLOR);
    //std::vector<Mat> rgbChannels[3];
    std::vector<Mat> rgbChannels;
    //uchar* bit8array = src.isContinuous()? src.data: src.clone().data;
    //uchar * bit8array[8] = {0};
    for (uint j = 0; j<2; j++)
    {
        for (uint i = 0; i < 8; i++)
        {
            if (remainingImages > 0)
            {
                uchar* bit8array = src.isContinuous()? src.data: src.clone().data;
                QListWidgetItem* filename = ui->InputList->item(encodeCount); //Select image
                QString file_name = filename->text(); //convert to string to read by openCV
                workingImage = imread( samples::findFile (file_name.toUtf8().constData()), IMREAD_COLOR); //read image into workingImage mat
                cvtColor(workingImage, workingGray, COLOR_BGR2GRAY); //convert image to grayscale
                threshold(workingGray, workingBinary, 50, 250, 0); //threshold image to binary
                uchar * array = workingBinary.isContinuous()? workingBinary.data: workingBinary.clone().data; //Copy binary mat to an array for operations
                uint length = workingBinary.total()*workingBinary.channels(); //Get length of array for operations
                printf("size of array: %d",sizeof(array));
                int encodeVal = hexSelect(i); //get correct hexval for current bit layer
                ui->TerminalOut->append("encodeVal: " + QString::number(encodeVal));
                for (uint k = 0; k < length; k++) //iterates through every element in array
                {
                    if (array[k] > 10)
                    {
                        array[k] = encodeVal;
                    }
                }
                if (firstLayer) //If bit layer 0, copy into the empty bit8array
                {
                    //uchar * bit8array = workingBinary.isContinuous()? workingBinary.data: workingBinary.clone().data; //Copy binary mat to an array for operations
                    //uchar * bit8array = src.isContinuous()? src.data: src.clone().data; //Copy binary mat to an array for operations
                    bit8array = array;
                    ui->TerminalOut->append("first layer " + file_name);
                    firstLayer = false;
                }
                else
                {
                    //std::transform(bit8array.begin(), bit8array.end(), )
                    int count = 0;
                    //cv::Mat Channel2 (rowDim, colDim, CV_8U);
                    //Channel2.data = array;
                    //QPixmap EncodeTest2 = QPixmap::fromImage(QImage((unsigned char*) Channel2.data, Channel2.cols, Channel2.rows, QImage::Format_Indexed8));
                    //ui->ImageDisplay_2->setPixmap(EncodeTest2.scaled(500,300, Qt::KeepAspectRatio));
                    for (uint w = 0; w < length; w++) //Iterates through every element in arrays, ARRAYS MUST BE EQUAL DIMENSIONS
                    {
                        bit8array[w] = array[w] | bit8array[w]; //| array[i]; //Bitwise OR operator
                        count++;
                    }
                    //cv::Mat Channel3 (rowDim, colDim, CV_8U);
                    //Channel3.data = bit8array;
                    //QPixmap EncodeTest3 = QPixmap::fromImage(QImage((unsigned char*) Channel3.data, Channel3.cols, Channel3.rows, QImage::Format_Indexed8));
                    //ui->ImageDisplay_3->setPixmap(EncodeTest3.scaled(500,300, Qt::KeepAspectRatio));
                    printf("bit8array2: %d\n",sizeof(bit8array));
                    ui->TerminalOut->append("testtest: " + QString::number(count));
                    if (i ==7)
                    {
                        if (j ==0)
                        {
                            cv::Mat Channel (rowDim, colDim, CV_8U);
                            Channel.data = bit8array;
                            rgbChannels.push_back(Channel);
                        }
                        else if (j == 1)
                        {
                            cv::Mat Channel2(rowDim, colDim, CV_8U);
                            Channel2.data = bit8array;
                            rgbChannels.push_back(Channel2);
                        }
                        else if (j == 2)
                        {
                            cv::Mat Channel3(rowDim, colDim, CV_8U);
                            Channel3.data = bit8array;
                            rgbChannels.push_back(Channel3);
                        }
                        //rgbChannels[j] = Channel;
                    }
                }
                encodeCount++;
                remainingImages--;
                bit8array = 0;
                array = 0;
            }
            else
            {
                ui->TerminalOut->append("No more remaining images");
                return;
            }
        }
        firstLayer = true;
        //cv::Mat Channel(workingImage.rows, workingImage.cols, CV_8UC1);
        //cv::Mat Channel (rowDim, colDim, CV_8U);
        //Channel.data = bit8array;
        //encodedImage = Channel;
        //rgbChannels[j] = Channel;
    }
    QPixmap EncodeTest1 = QPixmap::fromImage(QImage((unsigned char*) rgbChannels[0].data, rgbChannels[0].cols, rgbChannels[0].rows, QImage::Format_Indexed8));
    QPixmap EncodeTest2 = QPixmap::fromImage(QImage((unsigned char*) rgbChannels[1].data, rgbChannels[1].cols, rgbChannels[1].rows, QImage::Format_Indexed8));
    QPixmap EncodeTest3 = QPixmap::fromImage(QImage((unsigned char*) rgbChannels[2].data, rgbChannels[2].cols, rgbChannels[2].rows, QImage::Format_Indexed8));
    ui->ImageDisplay->setPixmap(EncodeTest1.scaled(500,300, Qt::KeepAspectRatio));
    ui->ImageDisplay_2->setPixmap(EncodeTest2.scaled(500,300, Qt::KeepAspectRatio));
    ui->ImageDisplay_3->setPixmap(EncodeTest3.scaled(500,300, Qt::KeepAspectRatio));
    //merge(rgbChannels,encodedImage);
    //cvtColor(rgbChannels, encodedImage, COLOR_GRAY2BGR); //convert image to grayscale

}

//bitEncode24();
//QPixmap EncodeTest = QPixmap::fromImage(QImage((unsigned char*) encodedImage.data, encodedImage.cols, encodedImage.rows, QImage::Format_BGR888));
//ui->ImageDisplay->setPixmap(EncodeTest.scaled(500,300, Qt::KeepAspectRatio));

QListWidgetItem* filename1 = ui->InputList->item(encodeCount);
QString file1 = filename1->text();
src1 = imread( samples::findFile (file1.toUtf8().constData()), IMREAD_COLOR);

QListWidgetItem* filename2 = ui->InputList->item(encodeCount+8);
QString file2 = filename2->text();
src2 = imread( samples::findFile (file2.toUtf8().constData()), IMREAD_COLOR);

QListWidgetItem* filename3 = ui->InputList->item(encodeCount+16);
QString file3 = filename3->text();
src3 = imread( samples::findFile (file3.toUtf8().constData()), IMREAD_COLOR);


std::vector<Mat> rgbChannels;
Mat channel[3];
split(src,channel);
Mat ImageOut;

//uchar* bit8array1 = src1.isContinuous()? src1.data: src1.clone().data;
//uchar* bit8array2 = src2.isContinuous()? src2.data: src2.clone().data;
//uchar* bit8array3 = src3.isContinuous()? src3.data: src3.clone().data;

//Mat Chan1(src.rows, src.cols, CV_8UC1);
bitEncode8(src1, channel[0]);
//Mat Chan2(src.rows, src.cols, CV_8UC1);
bitEncode8(src2, channel[1]);
//Mat Chan3(src.rows, src.cols, CV_8UC1);
bitEncode8(src3, channel[2]);

merge(channel,3,ImageOut);
imshow("Test", ImageOut);
#endif

