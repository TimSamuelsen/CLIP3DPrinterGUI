#include "prinstscriptdialog.h"
#include "ui_prinstscriptdialog.h"

prinstscriptdialog::prinstscriptdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::prinstscriptdialog)
{
    ui->setupUi(this);
}

prinstscriptdialog::~prinstscriptdialog()
{
    delete ui;
}
