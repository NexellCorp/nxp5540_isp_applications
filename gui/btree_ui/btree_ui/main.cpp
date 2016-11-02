#include "mainwindow.h"
#include <QApplication>

#include "readthread.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    ReadThread thread;

    QObject::connect(&thread, SIGNAL(frameUpdated(unsigned char*)), &w, SLOT(imageUpdate(unsigned char*)));
    QObject::connect(&w, SIGNAL(runThread(thread_status)), &thread, SLOT(startRead(thread_status)));
    QObject::connect(&w, SIGNAL(setOpendedDevice(struct device *)), &thread, SLOT(setDevice(struct device *)));
    thread.start();
    w.show();
    printf("main : %d \n", a.thread()->currentThreadId());
    return a.exec();
}
