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

    QString CurrentPath = QDir::currentPath();
    a.setWindowIcon(QIcon(":/new/prefix1//testicon.ico"));

    QFile styleSheetFile(":/new/prefix1/Ubuntu.qss");
    styleSheetFile.open(QFile::ReadOnly);
    QString styleSheet { QLatin1String(styleSheetFile.readAll()) };
    a.setStyleSheet(styleSheet);

    MainWindow w;
    w.setWindowTitle("CLIP3DGUI");
    w.setWindowIcon(QIcon(":/new/prefix1/testicon.ico"));
    w.show();
    return a.exec();
}
