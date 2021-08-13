#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QDir>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("MyOrganization");
    a.setOrganizationDomain("MyDomain");
    a.setApplicationName("CLIP3DGUI");

    //QFile styleSheetFile(":/dark/stylesheet.qss");
    QFile styleSheetFile(":/light/stylesheet.qss");
    //QFile styleSheetFile(":/new/pics/Ubuntu.qss");
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet { QLatin1String(styleSheetFile.readAll()) };
    a.setStyleSheet(styleSheet);

    MainWindow w;
    w.setWindowTitle("CLIP3DGUI");
    w.setWindowIcon(QIcon(":/new/pics/testicon.ico"));
    w.show();
    return a.exec();
}
