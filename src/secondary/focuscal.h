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

private:
    Ui::FocusCal *ui;
};

#endif // FOCUSCAL_H
