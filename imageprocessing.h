#ifndef IMAGEPROCESSING_H
#define IMAGEPROCESSING_H

#include <QWidget>
#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>

namespace Ui {
class imageprocessing;
}

class imageprocessing : public QWidget
{
    Q_OBJECT

public:
    explicit imageprocessing(QWidget *parent = nullptr);
    ~imageprocessing();


private slots:
    void on_SelectImageFiles_clicked();

    void on_DisplayImage_clicked();

    void on_pushButton_clicked();

    void on_ClearImageFiles_clicked();

    void on_Split_clicked();

    void on_BinarySplit_clicked();

    void on_EncodeTest_clicked();

    void on_BrowseTargetDir_clicked();

    void bitEncode8(cv::Mat source, cv::Mat& Channel);

private:
    Ui::imageprocessing *ui;
    cv::Mat mframe;
    void bitEncode24();
    //void bitEncode8(cv::Mat source, cv::Mat& Channel);
    int hexSelect(int imageNum);

};

#endif // IMAGEPROCESSING_H
