#include "mainwindow.h"
//#include "QtTheme.h"
#include <QApplication>
#include <QFile>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("MyOrganization");
    a.setOrganizationDomain("MyDomain");
    a.setApplicationName("CLIP3DGUI");

    //QFile styleSheetFile("./TCobra.qss");
    //styleSheetFile.open(QFile::ReadOnly);

    MainWindow w;
    w.setWindowTitle("CLIP3DGUI");
    w.show();
    return a.exec();
}
