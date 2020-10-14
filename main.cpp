#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("IRLToolkit");
    QCoreApplication::setOrganizationDomain("irltoolkit.com");
    QCoreApplication::setApplicationName("obsws-rproxy-client");
    MainWindow w;
    w.show();

    return a.exec();
}
