#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // Disable sandbox
    qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");

    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}
