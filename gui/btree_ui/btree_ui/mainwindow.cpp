#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "deviceioctl.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    dev = NULL;
    createActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::open()
{
    //ui->label->setText("Open");
    qDebug("Open \n");

    if(dev)
        QMessageBox::information(this, "Error", "device is already existed");

    dev = openDevice();
    if(!dev)
        QMessageBox::information(this, "Error", "failed to open device");
    else
        QMessageBox::information(this, "Error", "device is connected");
}

void MainWindow::close()
{
    qDebug("Close\n");
    closeDevice(dev);
    //exit(false);
}

void MainWindow::streamOn()
{
    qDebug("stream On \n");
    startDevice(dev);
}

void MainWindow::streamOff()
{
    qDebug("stream Off \n");
    stopDevice(dev);
}

void MainWindow::createActions()
{
    qDebug("create Actions\n");
    connect(ui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(open()));
    connect(ui->actionClose, SIGNAL(triggered(bool)), this, SLOT(close()));
    connect(ui->actionStream_On, SIGNAL(triggered(bool)), this, SLOT(streamOn()));
    connect(ui->actionStream_Off, SIGNAL(triggered(bool)), this, SLOT(streamOff()));
}
