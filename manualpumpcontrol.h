#ifndef MANUALPUMPCONTROL_H
#define MANUALPUMPCONTROL_H

#include <QWidget>
#include "serialib.h"

namespace Ui {
class manualpumpcontrol;
}

class manualpumpcontrol : public QWidget
{
    Q_OBJECT

public:
    explicit manualpumpcontrol(QWidget *parent = nullptr);
    ~manualpumpcontrol();
    serialib PSerial;

private slots:
    void on_ConnectButton_clicked();

    void on_GetInfuseRate_clicked();

    void on_SetInfuseRate_clicked();

private:
    Ui::manualpumpcontrol *ui;
    char* SerialRead();
};

#endif // MANUALPUMPCONTROL_H
