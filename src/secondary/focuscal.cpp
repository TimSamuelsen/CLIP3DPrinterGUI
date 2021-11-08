#include "focuscal.h"
#include "ui_focuscal.h"
#include "tl_camera_sdk.h"
#include "tl_camera_sdk_load.h"


FocusCal::FocusCal(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FocusCal)
{
    ui->setupUi(this);
}

FocusCal::~FocusCal()
{
    delete ui;
}
