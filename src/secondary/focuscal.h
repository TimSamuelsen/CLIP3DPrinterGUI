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

private slots:
    void on_StageConnectButton_clicked();

private:
    Ui::FocusCal *ui;
    void initCamera();
    int setExposure(long long Exposure);
    int setGain(double Gain, void* camera_handle);
    void SoftwareTrigger();
};

#endif // FOCUSCAL_H
