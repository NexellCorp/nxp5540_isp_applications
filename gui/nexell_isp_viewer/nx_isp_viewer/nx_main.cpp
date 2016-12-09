/* qt */
#include "qapplication.h"
#include "qthread.h"
/* nexell isp viewer */
#include "nx_func_control.h"
#include "nx_debug.h"
#include "nx_main_window.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NxMainWindow w;
    NxFuncControl funcControl;
    QThread thread;

    NxDebug("main thread - %ld", (long)QThread::currentThreadId());

    QObject::connect(&w, SIGNAL(userEvent(int,void*)), &funcControl, SLOT(recvCommand(int,void*)), Qt::QueuedConnection);
    QObject::connect(&funcControl, SIGNAL(uiNeedUpdate(int,int,void*)), &w, SLOT(recvResult(int,int,void*)), Qt::QueuedConnection);
    QObject::connect(&funcControl, SIGNAL(bufIsDone(unsigned char*)), &w, SLOT(updateFrame(unsigned char*)), Qt::QueuedConnection);
    QObject::connect(&thread, SIGNAL(finished()), &funcControl, SLOT(deleteLater()), Qt::QueuedConnection);
    funcControl.moveToThread(&thread);
    thread.start();
    w.show();
    a.exec();
    thread.exit();
    NxDebug("func thread-%d",thread.isFinished());
    thread.wait();
    return 0;
}
