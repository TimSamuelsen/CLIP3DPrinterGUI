#ifndef FOCUSCAL_H
#define FOCUSCAL_H

#include <QWidget>

namespace Ui {
class FocusCal;
}

class FocusCal : public QWidget
{
    Q_OBJECT

public:
    explicit FocusCal(QWidget *parent = nullptr);
    ~FocusCal();
    void TerminalPrint(QString stringToPrint);

private slots:
    void on_StageConnectButton_clicked();

private:
    Ui::FocusCal *ui;
    void initCamera();
    int setExposure(long long Exposure);
    int setGain(double Gain, void* camera_handle);
    int SoftwareTrigger();

    int initialize_camera_resources();
    int report_error_and_cleanup_resources(const char* error_string);
    //void frame_available_callback(void* sender, unsigned short* image_buffer, int frame_count, unsigned char* metadata, int metadata_size_in_bytes, void* context);
};

#endif // FOCUSCAL_H
