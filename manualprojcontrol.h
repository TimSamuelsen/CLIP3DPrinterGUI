#ifndef MANUALPROJCONTROL_H
#define MANUALPROJCONTROL_H

#include <QWidget>

namespace Ui {
class manualLEcontrol;
}

class manualLEcontrol : public QWidget
{
    Q_OBJECT

public:
    explicit manualLEcontrol(QWidget *parent = nullptr);
    ~manualLEcontrol();

private:
    Ui::manualLEcontrol *ui;
    int StatusCheck();
};

#endif // MANUALPROJCONTROL_H
