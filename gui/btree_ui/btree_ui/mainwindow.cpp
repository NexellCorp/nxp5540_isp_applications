#include <QMessageBox>
#include <QImage>
#include <QTime>
#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "deviceioctl.h"
#include "readthread.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    printf("MainWindow : %d \n",thread()->currentThreadId());
    ui->setupUi(this);
    dev = NULL;
    createActions();
    ui->label->setVisible(false);
    ui->label->show();
    printf(" end --- MainWindow \n");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fileOpen()
{
    qDebug("File Open \n");

    if(dev)
        QMessageBox::information(this, "Error", "device is already existed");

    dev = openDevice();
    if(!dev)
        QMessageBox::information(this, "Fail", "failed to open device");
    else {
        QMessageBox::information(this, "Success", "device is connected");
        printf("[%s]device = 0x%x \n", __func__, dev);
        emit setOpendedDevice(dev);
    }
}

void MainWindow::fileClose()
{
    int ret = -1;
    qDebug("File Close \n");
    emit runThread(thread_stop);
    ret = stopDevice(dev);
    if (ret < 0)
        qDebug("failed to stream off : %d", ret);
    closeDevice(dev);
    dev = NULL;
}

void MainWindow::fileExit()
{
    qDebug("File Exit\n");
    close();
}

void MainWindow::streamOn()
{
    int ret = -1;

    qDebug("stream On \n");

    ret = startDevice(dev);
    if (ret < 0)
        QMessageBox::information(this, "Fail", "failed to stream on : %d ", ret);
    else {
        emit runThread(thread_run);
        ui->label->setVisible(true);
    }
}

void MainWindow::streamOff()
{
    int ret = -1;

    qDebug("stream Off \n");
    ui->label->setVisible(false);
    emit runThread(thread_wait);
    ret = stopDevice(dev);
    if (ret < 0)
        qDebug("failed to stream off : %d", ret);
}

bool MainWindow::loadFile(unsigned char *file)
{
    printf("load image \n");
    QImage *img = new QImage(file, dev->width, dev->height, dev->width*3, QImage::Format_RGB888, NULL, NULL);
    QPixmap *buffer = new QPixmap();
    *buffer = QPixmap::fromImage((*img));
    *buffer = buffer->scaled(img->width(), img->height());

    ui->label->setPixmap(*buffer);
    ui->label->resize(buffer->width(), buffer->height());
    //ui->label->show();
    ui->label->repaint();

    //qDebug() << "finished updated - " << QDateTime::currentDateTime();
    return true;
}

void MainWindow::imageUpdate(unsigned char* rgbbuf)
{
    //qDebug() << "Update - " << QDateTime::currentDateTime();
    if(!rgbbuf)
        printf("failed to get rgb buf \n");
    else
        loadFile(rgbbuf);
    free(rgbbuf);
    //printf("----- end image update function \n");
}

void MainWindow::createActions()
{
    qDebug("create Actions\n");
    connect(ui->actionOpen, SIGNAL(triggered(bool)), this, SLOT(fileOpen()));
    connect(ui->actionClose_2, SIGNAL(triggered(bool)), this, SLOT(fileClose()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(fileExit()));
    connect(ui->actionStream_On, SIGNAL(triggered(bool)), this, SLOT(streamOn()));
    connect(ui->actionStream_Off, SIGNAL(triggered(bool)), this, SLOT(streamOff()));
}
