#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "serialib.h"
#include "SMC100C.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}




void MainWindow::on_ManualStage_clicked()
{

}
