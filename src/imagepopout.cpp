#include "imagepopout.h"
#include "ui_imagepopout.h"

QStringList ImageList;

imagepopout::imagepopout(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::imagepopout)
{
    ui->setupUi(this);
}

imagepopout::~imagepopout()
{
    delete ui;
}

void imagepopout::getImageList(QStringList Images)
{
    ImageList = Images;
}

void imagepopout::showImage(QPixmap imagename)
{
    //QPixmap img(imagename);
    ui->Image->setPixmap(imagename);
    printf("ShowingImage");
}
