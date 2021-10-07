#ifndef PRINTSETTINGS_H
#define PRINTSETTINGS_H

#include <QWidget>

namespace Ui {
class printsettings;
}

class printsettings : public QWidget
{
    Q_OBJECT

public:
    explicit printsettings(QWidget *parent = nullptr);
    ~printsettings();

private:
    Ui::printsettings *ui;
};

#endif // PRINTSETTINGS_H
