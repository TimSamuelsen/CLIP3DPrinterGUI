#include "printsettings.h"
#include "ui_printsettings.h"

printsettings::printsettings(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::printsettings)
{
    ui->setupUi(this);
}

printsettings::~printsettings()
{
    delete ui;
}



#if 0

#endif
