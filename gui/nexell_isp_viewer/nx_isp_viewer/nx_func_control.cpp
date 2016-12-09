/* standard */
#include <stdio.h>
#include <stdlib.h> /* malloc */
#include <fcntl.h> /* file io */
#include <unistd.h> /* file io */
/* linux */
#include <linux/errno.h>
#include <linux/videodev2.h> /* for V4L2 */
/* nexell isp viewer */
#include "nx_common_enum.h"
#include "nx_debug.h"
#include "nx_str_common.h"
#include "nx_yuv_to_rgb.h"
#include "nx_device_control.h"
#include "qfuturewatcher.h"
#include "QtConcurrent/QtConcurrent"
dddd
#include "nx_func_control.h"

NxFuncControl::NxFuncControl()
{
    NxDebug("[%s] %ld", __func__, (long)QThread::currentThreadId());
    dev = NULL;
}

void NxFuncControl::initDevValues(struct nx_device *dev,
                            int width, int height)
{
    int i;

    dev->drm_fd = -1;
    dev->video_fd = -1;

    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        dev->dma_fds[i] = -1;
        dev->gem_fds[i] = -1;
        dev->pVaddrs[i] = NULL;
    }
    dev->buffer_count = 4;
    dev->dq_index = 0;
    dev->format = V4L2_PIX_FMT_UYVY;
    dev->buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
    dev->mem_type = V4L2_MEMORY_DMABUF;
    dev->width = width;
    dev->height = height;
    dev->size = NxGetSize(dev->width, dev->height, true);
}

void NxFuncControl::deinitBuf(struct nx_device *device)
{
    unsigned int i;
    NxDebug("[%s] \n", __func__);
    for (i = 0; i < dev->buffer_count; i++) {
        NxDebug("gem:%d, dma:%d", device->gem_fds[i], device->dma_fds[i]);
        if (device->dma_fds[i] >= 0)
            close(device->dma_fds[i]);
        if (device->gem_fds[i] >= 0)
            NxFreeGem(device->drm_fd, device->gem_fds[i]);
    }
}

int NxFuncControl::initBuf(struct nx_device *device)
{
    int ret = -1;
    unsigned int i = 0;

    NxDebug("[%s]\n", __func__);
    for (i = 0; i < dev->buffer_count; i++) {
        ret = NxAllocGem(device->drm_fd, device->size);
        if (ret < 0) {
            NxDebug("failed to gem alloc \n");
            ret = -ENOMEM;
            goto done;
        }
        device->gem_fds[i] = ret;
        NxDebug(" gem_fds is %d ", device->gem_fds[i]);
        ret = NxGetDmafd(device->drm_fd, device->gem_fds[i]);
        if (ret < 0) {
            NxDebug("failed to dma handler using gem handle %d\n",ret);
            ret = -ENOMEM;
            goto done;
        }
        device->dma_fds[i] = ret;
        NxDebug(" dma_fds is %d ", device->dma_fds[i]);
        ret = NxGetVaddr(device->drm_fd, device->gem_fds[i], device->size, &device->pVaddrs[i]);
        if (ret < 0) {
            NxDebug("failed to get virtual address \n");
            goto done;
        }

    }
    NxDebug("reqbuf \n");
    NxV4l2Reqbuf(device->video_fd, dev->buffer_count, device->buf_type);
    return ret;
done:
    deinitBuf(device);
    return ret;
}

int NxFuncControl::openDevice(void *data)
{
    int ret = -1;
    struct nx_device *device = NULL;
    struct nx_image_format *format = (struct nx_image_format*)data;

    NxDebug("[%s] width = %d, height = %d \n",
             __func__,
             format->width,
             format->height);

    device = (struct nx_device *)malloc(sizeof(struct nx_device));
    if(!device) {
        NxDebug("failed to malloc for device");
        return ret;
    }
    dev = device;

    initDevValues(dev, format->width, format->height);

    if ((ret = NxOpenDrmDevice()) < 0)
            goto done;
    dev->drm_fd = ret;

    if ((ret = NxOpenVideoDevice()) < 0)
            goto done;
    dev->video_fd = ret;

    ret = NxSetImgFormat(dev->video_fd,
                            dev->width,
                            dev->height,
                            dev->buf_type,
                            dev->format);
    if (ret < 0)
        goto done;

    ret = initBuf(dev);
    if (ret < 0)
        goto done;

    return ret;

done:
    closeDevice();
    return ret;
}

int NxFuncControl::closeDevice()
{
    NxDebug("[%s]\n",__func__);
    NxV4l2Reqbuf(dev->video_fd, 0, dev->buf_type);
    if (dev->video_fd > 0)
        close(dev->video_fd);
    deinitBuf(dev);
    if (dev->drm_fd > 0)
        close(dev->drm_fd);
    if (dev)
        free(dev);
    dev = NULL;
    return 1;
}

int NxFuncControl::exit()
{
    NxDebug("[%s]\n",__func__);
    return 1;
}

void NxFuncControl::callDisplay(unsigned char *dst)
{
    NxDebug("[%s:%ld] \n", __func__, (long)QThread::currentThreadId());
    emit bufIsDone(dst);
}

int NxFuncControl::previewStart()
{
    int ret = -1;

    NxDebug("[%s] - %ld \n", __func__, (long)QThread::currentThreadId());

    dqthread = new QThread;
    bufcontrol = new NxBufControl(dev);
    connect(this, SIGNAL(stopPreview()), bufcontrol, SLOT(stop()), Qt::DirectConnection);
    connect(bufcontrol, SIGNAL(buf_is_ready(unsigned char*)), this, SLOT(callDisplay(unsigned char*)), Qt::QueuedConnection);
    connect(dqthread, SIGNAL(started()), bufcontrol, SLOT(run()), Qt::QueuedConnection);
    connect(dqthread, SIGNAL(finished()), bufcontrol, SLOT(deleteLater()), Qt::QueuedConnection);
    bufcontrol->moveToThread(dqthread);
    dqthread->start();

    ret = NxV4l2StreamOn(dev->video_fd, dev->buf_type);
    if (ret)
        NxDebug("failed to start preview \n");
    return ret;
}

int NxFuncControl::previewStop()
{
    int ret = -1;

    emit stopPreview();
    dqthread->exit();
    dqthread->wait();
    NxDebug("thread-%d",dqthread->isFinished());

    ret = NxV4l2StreamOff(dev->video_fd, dev->buf_type);
    if (ret)
        NxDebug("failed to stop preview \n");

    return ret;
}

static int controlRegister(int fd, struct nx_reg_control* reg)
{
    int ret = -1;
    unsigned int i, j;
    unsigned int address = 0x0000;
    unsigned int data[16][4] = {0,};

    memcpy(data, reg->data, sizeof(data));

    NxDebug("[%s:%ld] \n", __func__,
             (long)QThread::currentThreadId());

    for (i = 0; i < 16; i++) {
        for ( j = 0; j < 4; j++) {
            NxDebug(" i= %d, j= %d \n", i, j);
            address = ((reg->addr) | ((i << 4) & 0x00F0) | (4*j));
            if (reg->flag == nx_reg_flag_read_page)
                ret = NxV4l2GetRegister(fd,
                                     reg->type,
                                     address,
                                     &data[i][j]);
            else
                ret = NxV4l2SetRegister(fd,
                                     reg->type,
                                     address,
                                     data[i][j]);
            if (ret)
                NxDebug("failed to control register:%d \n", ret);
            else
                NxDebug("addr = 0x%4x, data = 0x%8x \n",
                         address, data[i][j]);
        }
    }
    memcpy(reg->data, data, sizeof(data));
    return ret;
}

int NxFuncControl::controlReg(struct nx_reg_control *reg)
{
    int ret = -1;

    NxDebug("[%s] tyep[%s] flag[%d], addr[0x%x], data[%p] \n",
             __func__, (reg->type)?"sensor":"isp",
             reg->flag, reg->addr, reg->data);

    if (reg->flag == nx_reg_flag_read)
        ret = NxV4l2GetRegister(dev->video_fd,
                             reg->type,
                             reg->addr,
                             (unsigned int*)reg->data);
    else if(reg->flag == nx_reg_flag_write)
        ret = NxV4l2SetRegister(dev->video_fd,
                             reg->type,
                             reg->addr,
                             *(unsigned int*)reg->data);
    else {
        QFuture<int> worker = QtConcurrent::run(controlRegister,
                                                dev->video_fd,
                                                reg);
        QFutureWatcher<int> watcher;
        //result.waitForFinished();
        //QObject::connect(&watcher, SIGNAL(finished()), this, SLOT(q))
        watcher.setFuture(worker);
        ret = worker.result();
    }
    if (ret)
        NxDebug("failed to control register - %d\n", ret);
    return ret;
}

int loadRegisterSet(int type, char *filename, int video_fd)
{
    int ret = -1, page, addr, isp_addr, data, addr_inc;
    char buff[256] = {0,};
    char *pbuf = NULL, *p = NULL;
    FILE *fp = NULL;

    NxDebug("[%s:%ld] \n", __func__,
             (long)QThread::currentThreadId());

    if ( !(fp = fopen(filename, "rb"))) {
        NxDebug("fail to open the file \n");
        return ret;
    }
    if (type == nx_reg_type_isp)
        addr_inc = 4;
    else
        addr_inc = 2;

    while (!feof(fp))
    {
        fget_line(fp, buff, sizeof(buff));
        pbuf = str_strip( buff, " \t\r\n");

        // check comment
        if ( ((pbuf[0] == '/') && (pbuf[1] == '/')) || (pbuf[0] == 0))
            continue;
        p = str_token(pbuf, " \t\r\n");
        if (strncmp(p, "bt", 2))    continue;

        // sleep
        if ( !strncmp(p, "bts", 3))
            sleep(atoh(&(p[3])));

        if (!strncmp(p, "btp", 3))
            page = atoh(&(p[3]));
        else if (!strncmp(p, "bta", 3))
            addr = atoh(&(p[3]));
        else if (!strncmp(p, "btd", 3)) {
                isp_addr = ((page << 12) & 0xF000) | addr;
                data = atoh(&(p[3]));
                ret = NxV4l2SetRegister(video_fd,
                                           type,
                                           isp_addr,
                                           data);
                addr += addr_inc;
        }
    }
    fclose(fp);
    return ret;
}

int NxFuncControl::loadRegSet(int type, char *data)
{
    int ret = -1;
    NxDebug("filename is %s", data);

    QFuture<int> worker = QtConcurrent::run(loadRegisterSet,
                                            type,
                                            data,
                                            dev->video_fd);
    QFutureWatcher<int> watcher;
    watcher.setFuture(worker);
    ret = worker.result();
    return ret;
}

void NxFuncControl::cmdProcessing(int cmd, void *data)
{
    int ret = -1;
    void *retData = NULL;

    NxDebug("[%s:%ld] cmd is %d \n",
             __func__, (long)QThread::currentThreadId(), cmd);
    switch(cmd)
    {
        case nx_cmd_open_device:
            ret = openDevice(data);
        break;
        case nx_cmd_close_device:
            ret = closeDevice();
        break;
        case nx_cmd_preview_start:
            ret = previewStart();
        break;
        case nx_cmd_preview_stop:
            ret = previewStop();
        break;
        case nx_cmd_control_reg:
        {
            struct nx_reg_control reg = *(struct nx_reg_control*)data;
            ret = controlReg(&reg);
            retData = &reg;
        }
        break;
        case nx_cmd_load_sensor_regset:
            ret = loadRegSet(nx_reg_type_sensor, (char*)data);
        break;
        case nx_cmd_load_isp_regset:
            ret = loadRegSet(nx_reg_type_isp, (char*)data);
        break;
        case nx_cmd_exit:
            ret = true;
        break;
        default:
        ret = -1;
        break;
    }

    emit uiNeedUpdate(cmd, ret, retData);
    if (cmd == nx_cmd_exit) {
        exit();
        wait();
    }
}

void NxFuncControl::recvCommand(int command, void *data)
{
    NxDebug("[%s] - %ld \n", __func__, (long)QThread::currentThreadId());
    NxDebug("[%s] cmd is %d, data is %p ", __func__, command, data);

    if ((command < 0 ) || (command > nx_cmd_exit)) {
        NxDebugErr("invalid command \n");
        return;
    }
    cmdProcessing(command, data);
}
