#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QTimer>
#include "imageview.h"
#include "readthread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
signals:
     void runThread(enum    thread_status);
     void setOpendedDevice(struct device *);

private slots:
    void fileOpen();
    void fileClose();
    void fileExit();
    void streamOn();
    void streamOff();
    void imageUpdate(unsigned char*);

    void on_openGLWidget_frameSwapped();

private:
    Ui::MainWindow *ui;
    struct device *dev;
    void createActions();
   // bool loadFile(const QString &);
    bool loadFile(unsigned char *);
};

#endif // MAINWINDOW_H
