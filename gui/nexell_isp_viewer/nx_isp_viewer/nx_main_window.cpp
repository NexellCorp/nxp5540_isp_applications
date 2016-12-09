/* qt */
#include "qthread.h"
#include "qfiledialog.h"
#include "qfuturewatcher.h"
#include "qimage.h"
#include "qmessagebox.h"
/* nexell isp viewer */
#include "nx_common_enum.h"
#include "nx_debug.h"
#include "nx_yuv_to_rgb.h"
#include "ui_nx_main_window.h"
#include "nx_main_window.h"

NxMainWindow::NxMainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NxMainWindow)
{
    NxDebug("Main Window is created \n");
    ui->setupUi(this);
    initValues();
    updateMenu();
    createActions();
}

NxMainWindow::~NxMainWindow()
{
    NxDebug("Main Window is deleted \n");
    ispRegEdit->close();
    delete ispRegEdit;
    delete ui;
}

void NxMainWindow::updateFrame(unsigned char *src)
{
    NxDebug("[%s] \n", __func__);
    if (!src)
        return;
    if (viewerStatus != nx_viewer_previewing) {
        if (src)
            free(src);
        return;
    }
    memcpy(frame_buffer, src, frame_size);
    free(src);

    QImage *img = new QImage(frame_buffer, frame_width, frame_height, frame_width*3, QImage::Format_RGB888, NULL, NULL);
    QPixmap buffer = QPixmap(img->width(), img->height());
    buffer = QPixmap::fromImage((*img));
    //buffer = buffer.scaled(img->width(), img->height());
    ui->label->setPixmap(buffer);
    //ui->label->resize(buffer.width(), buffer.height());
    ui->label->repaint();
}

int NxMainWindow::checkStatus(int function, int cur_status)
{
    int ret = 0;
    if (function == cur_status)
        ret = 1;
    return ret;
}

void NxMainWindow::openDevice()
{
    struct nx_image_format format;

    NxDebug("%s",__func__);
    if (checkStatus(viewerStatus, nx_viewer_connecting))
        return;

    format.width = frame_width;
    format.height = frame_height;

    viewerStatus = nx_viewer_connecting;
    sendCommand(nx_cmd_open_device, &format);
}

void NxMainWindow::closeDevice()
{
    NxDebug("%s",__func__);
    if (checkStatus(viewerStatus, nx_viewer_disconnecting))
        return;
    if (checkStatus(viewerStatus, nx_viewer_previewing)) {
        previewStop();
        return;
    }
    viewerStatus = nx_viewer_disconnecting;
    sendCommand(nx_cmd_close_device, NULL);
    ispRegEdit->hide();
}

void NxMainWindow::saveImage()
{
    NxDebug("%s",__func__);
}

void NxMainWindow::loadSensorRegSet()
{
    NxDebug("%s",__func__);
    QString fileName = QFileDialog::getOpenFileName(this,
             tr("Open Sensor Tunning File"), "./", tr("Register Setting Files (*.btset)"));
    char *filename = fileName.toLocal8Bit().data();
    if (fileName.isNull())
        return;
    emit userEvent(nx_cmd_load_sensor_regset, filename);
}


void NxMainWindow::loadISPRegSet()
{
    NxDebug("%s",__func__);
    QString fileName = QFileDialog::getOpenFileName(this,
             tr("Open ISP Tunning File"), "./", tr("Register Setting Files (*.btset)"));
    if (fileName.isNull())
        return;
    char *filename = fileName.toLocal8Bit().data();
    emit userEvent(nx_cmd_load_isp_regset, filename);
#if 0
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this,"error",file.errorString());
    }
    QTextStream in(&file);
    while(!in.atEnd()) {

        //QStringList  fields = line.split(",");
        QString line = in.readLine();
        NxDebug() << "data - " << line;
        //model->appendRow(fields);

    }
    file.close();
#endif
}

void NxMainWindow::saveCurrentRegSet()
{
    NxDebug("%s",__func__);
}

void NxMainWindow::exit()
{
    NxDebug("%s",__func__);
    if (checkStatus(viewerStatus, nx_viewer_exit))
        return;
    viewerStatus = nx_viewer_exit;
    sendCommand(nx_cmd_exit, NULL);
}

void NxMainWindow::previewStart()
{
    NxDebug("%s",__func__);
    if (checkStatus(viewerStatus, nx_viewer_preview_start))
        return;
    frame_size = NxGetSize(frame_width,frame_height, false);
    frame_buffer = (unsigned char*)malloc(frame_size);
    if (!frame_buffer) {
        NxDebug("failed to malloc %d", frame_size);
        return;
    }
    ui->label->resize(frame_width, frame_height);
    viewerStatus = nx_viewer_preview_start;
    sendCommand(nx_cmd_preview_start, NULL);
}

void NxMainWindow::previewStop()
{
    NxDebug("%s",__func__);
    if (checkStatus(viewerStatus, nx_viewer_preview_stop))
        return;
    viewerStatus = nx_viewer_preview_stop;
    sendCommand(nx_cmd_preview_stop, NULL);
}

void NxMainWindow::sensorRegControl()
{
    NxDebug("%s",__func__);
}

void NxMainWindow::ispRegControl()
{
    NxDebug("%s",__func__);
    ispRegEdit->show();
}

void NxMainWindow::ispBlockOnOff()
{
    NxDebug("%s",__func__);
}

void NxMainWindow::recvResult(int cmd, int ret, void *data)
{
    NxDebug("[%s] cmd: %d, ret: %d ",
             __func__, cmd, ret);
    //NxDebug("[%s] - %d \n", __func__, QThread::currentThreadId());

    NxDebug("previous viewerSatus is %d ", viewerStatus);
   if (ret >= 0) {
       switch(cmd)
       {
            case nx_cmd_preview_stop:
                viewerStatus = nx_viewer_connected;
                if (frame_buffer)
                    free(frame_buffer);
            break;
            case nx_cmd_close_device:
                viewerStatus = nx_viewer_main;
            break;
            case nx_cmd_control_reg:
            {
                struct nx_reg_control reg = *(struct nx_reg_control *)data;
                NxDebug("type is %d \n", reg.type);
                emit updateIspReg(&reg);
            }
            break;
            case nx_cmd_load_sensor_regset:
            case nx_cmd_load_isp_regset:
            break;
            case nx_cmd_exit:
               close();
            break;
            case nx_cmd_open_device:
                QMessageBox::information(this, "succes", "device is connected");
            default:
                viewerStatus = (viewer_status)(viewerStatus + 1);
            break;
       }
   } else {
       switch(cmd)
       {
            case nx_cmd_load_sensor_regset:
            case nx_cmd_load_isp_regset:
                QMessageBox::warning(this, "warning", "failed to load register set");
            break;
            case nx_cmd_control_reg:
                QMessageBox::warning(this, "warning", "failed to control register");
            break;
            case nx_cmd_close_device:
                viewerStatus = nx_viewer_connected;
            break;
            case nx_cmd_open_device:
                QMessageBox::warning(this, "warning", "failed to open device");
            default:
                viewerStatus = (viewer_status)(viewerStatus - 1);
            break;
       }
   }
   NxDebug("after viewerSatus is %d ", viewerStatus);
   updateMenu();
}

int NxMainWindow:: sendCommand(int cmd, void* data)
{
    int ret = -1;

    NxDebug("[%s] cmd is %d \n", __func__, cmd);
    if ((cmd < nx_cmd_open_device) || (cmd > nx_cmd_exit)) {
        QMessageBox::warning(this, "warning", "Invalid Operation");
        return ret;
    }
    emit userEvent(cmd, data);
    return 1;
}

void NxMainWindow::controlReg(struct nx_reg_control *reg)
{
    NxDebug("[%s] tyep[%s] flag[%d], addr[0x%x], data[%p] \n",
             __func__, (reg->type)?"sensor":"isp",
             reg->flag, reg->addr, reg->data);
    emit userEvent(nx_cmd_control_reg, reg);
}

void NxMainWindow::updateMenu()
{
    bool enable = ((viewerStatus > nx_viewer_connecting)
                   && (viewerStatus < nx_viewer_exit))
                   ? true : false;

    ui->actionClose_Device->setEnabled(enable);
    ui->actionSave_Image->setEnabled(enable);
    ui->actionLoad_Sensor_Reg_Set->setEnabled(enable);
    ui->actionLoad_ISP_Reg_Set->setEnabled(enable);
    ui->actionSave_Current_Reg_Set->setEnabled(enable);
    ui->actionPlay->setEnabled(enable);
    ui->actionSensor_Control->setEnabled(enable);
    ui->actionISP_Reg_Control->setEnabled(enable);
    ui->actionBlock_On_Off->setEnabled(enable);
    ui->actionOpen_Device->setEnabled(!enable);
    enable = (viewerStatus == nx_viewer_previewing) ? true : false;
    ui->actionSave_Image->setEnabled(enable);
    ui->actionStop->setEnabled(enable);
    ui->label->setEnabled(enable);
    if (viewerStatus == nx_viewer_previewing)
        ui->actionPlay->setEnabled(false);
}

void NxMainWindow::initValues()
{
    NxDebug("%s", __func__);

    ispRegEdit = new NxRegEditDialog;
    ispRegEdit->setType(nx_reg_type_isp);
    ispRegEdit->hide();
    viewerStatus  = nx_viewer_main;
    frame_width = 800;
    frame_height = 600;
}

void NxMainWindow::createActions()
{
    NxDebug("%s",__func__);

    /* actions for Menu 'File' */
    connect(ui->actionOpen_Device, SIGNAL(triggered(bool)), this, SLOT(openDevice()));
    connect(ui->actionClose_Device, SIGNAL(triggered(bool)), this, SLOT(closeDevice()));
    connect(ui->actionSave_Image, SIGNAL(triggered(bool)), this, SLOT(saveImage()));
    connect(ui->actionLoad_Sensor_Reg_Set, SIGNAL(triggered(bool)), this, SLOT(loadSensorRegSet()));
    connect(ui->actionLoad_ISP_Reg_Set, SIGNAL(triggered(bool)), this, SLOT(loadISPRegSet()));
    connect(ui->actionSave_Current_Reg_Set, SIGNAL(triggered(bool)), this, SLOT(saveCurrentRegSet()));
    connect(ui->actionExit, SIGNAL(triggered(bool)), this, SLOT(exit()));

    /* actions for Menu 'View' */
    connect(ui->actionPlay, SIGNAL(triggered(bool)), this, SLOT(previewStart()));
    connect(ui->actionStop, SIGNAL(triggered(bool)), this, SLOT(previewStop()));
    /* actions for Menu 'Register Control' */
    connect(ui->actionSensor_Control, SIGNAL(triggered(bool)), this, SLOT(sensorRegControl()));
    connect(ui->actionISP_Reg_Control, SIGNAL(triggered(bool)), this, SLOT(ispRegControl()));
    /* actions for Menu 'ISP' */
    connect(ui->actionBlock_On_Off, SIGNAL(triggered(bool)), this, SLOT(ispBlockOnOff()));
    /* action for ISP Reg Control */
    connect(ispRegEdit, SIGNAL(controlReg(struct nx_reg_control *)), this, SLOT(controlReg(struct nx_reg_control *)));
    connect(this, SIGNAL(updateIspReg(struct nx_reg_control *)), ispRegEdit, SLOT(updateReg(struct nx_reg_control *)));
}
