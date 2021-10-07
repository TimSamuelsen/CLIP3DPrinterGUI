#ifndef PRINSTSCRIPTDIALOG_H
#define PRINSTSCRIPTDIALOG_H

#include <QDialog>

namespace Ui {
class prinstscriptdialog;
}

class prinstscriptdialog : public QDialog
{
    Q_OBJECT

public:
    explicit prinstscriptdialog(QWidget *parent = nullptr);
    ~prinstscriptdialog();

private:
    Ui::prinstscriptdialog *ui;
};

#endif // PRINSTSCRIPTDIALOG_H
