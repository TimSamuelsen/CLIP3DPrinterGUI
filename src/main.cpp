#include "mainwindow.h"
//#include "QtTheme.h"
#include <QApplication>
#include <QFile>
#include <QDir>
//#include <QString>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("MyOrganization");
    a.setOrganizationDomain("MyDomain");
    a.setApplicationName("CLIP3DGUI");
    a.setWindowIcon(QIcon(":/new/pics/testicon.ico"));

    QFile styleSheetFile(":/new/pics/Ubuntu.qss");
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet { QLatin1String(styleSheetFile.readAll()) };
    a.setStyleSheet(styleSheet);

    MainWindow w;
    w.setWindowTitle("CLIP3DGUI");
    w.setWindowIcon(QIcon(":/new/pics/testicon.ico"));
    w.show();
    return a.exec();
}
