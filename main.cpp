#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    int exitCode = 0;

    do{
        QApplication a(argc, argv);
        MainWindow w;
        w.show();
        exitCode = a.exec();
    }while(exitCode == MainWindow::EXIT_CODE_REBOOT);

    return exitCode;
}
