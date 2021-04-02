#ifndef MANUALPUMPCONTROL_H
#define MANUALPUMPCONTROL_H

#include <QWidget>

namespace Ui {
class manualpumpcontrol;
}

class manualpumpcontrol : public QWidget
{
    Q_OBJECT

public:
    explicit manualpumpcontrol(QWidget *parent = nullptr);
    ~manualpumpcontrol();

private slots:
    void on_ConnectButton_clicked();

private:
    Ui::manualpumpcontrol *ui;
};

#endif // MANUALPUMPCONTROL_H
