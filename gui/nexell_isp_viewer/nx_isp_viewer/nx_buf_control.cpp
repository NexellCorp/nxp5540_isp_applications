/* linux for v4l2 */
#include <linux/videodev2.h>
/* qt */
#include "qthread.h"
/* nexell isp viewer */
#include "nx_common_enum.h"
#include "nx_debug.h"
#include "nx_device_control.h"
#include "nx_yuv_to_rgb.h"
#include "nx_buf_control.h"

NxBufControl::NxBufControl(nx_device *device)
{
    NxDebug("[%s] - %ld \n", __func__, (long)QThread::currentThreadId());
    dev = device;
    stopFlag = false;
}

void NxBufControl::stop()
{
    NxDebug("NxBufControl[%s] - %ld \n", __func__, (long)QThread::currentThreadId());
    stopFlag = true;
}

void NxBufControl::run()
{
    unsigned int i;
    int ret = -1;
    unsigned char* rgbbuf = NULL;

    NxDebug("NxBufControl[%s] - %ld", __func__, (long)QThread::currentThreadId());

    for (i = 0; i < dev->buffer_count; i++) {
        ret = NxV4l2Qbuf(dev->video_fd, i,
                           dev->buf_type, dev->mem_type,
                           dev->dma_fds[i], dev->size);
        if (ret) {
            NxDebugErr("failed qbuf index %d \n", i);
        }
    }
    dev->dq_index = 0;

    while(1)
    {
        if (stopFlag)
            break;
        NxDebug("dqbuf thread : %d ", dev->dq_index);
        ret = Nxv4l2Dqbuf(dev->video_fd, dev->buf_type, dev->mem_type, &dev->dq_index);
        if (stopFlag)
            break;

        if (!ret) {

            NxDebug("---dqbuf - %d \n", dev->dq_index);
            rgbbuf = NxConvertRGB(dev->width, dev->height, (unsigned char*)dev->pVaddrs[dev->dq_index]);
            if (stopFlag)
                break;
            if(rgbbuf) {
                NxDebug("converting is finished \n");
                emit bufIsReady(rgbbuf);
            } else
                NxDebug("converting is failed \n");

            ret = NxV4l2Qbuf(dev->video_fd, dev->dq_index, dev->buf_type,
                               dev->mem_type, dev->dma_fds[dev->dq_index],
                               dev->size);
            if (!ret)
                NxDebug("====qbuf - %d \n", dev->dq_index);
            else {
                NxDebugErr("failed to queue buf[%d] ", dev->dq_index);
            }
        }
    }
    NxDebug("dqbuf thread is stopped \n");
}
