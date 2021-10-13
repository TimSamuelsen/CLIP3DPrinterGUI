#include "printmonitoring.h"
#include "ui_printmonitoring.h"

printmonitoring::printmonitoring(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::printmonitoring)
{
    ui->setupUi(this);
}

printmonitoring::~printmonitoring()
{
    delete ui;
}
