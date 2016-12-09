/* standard */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/mman.h> /* PROT_READ/PROT_WRITE/MAP_SHARED/mmap/munmap */
/* linux */
#include <linux/types.h>
#include <linux/videodev2.h> /* for V4L2 */
#include <drm/i915_drm.h> /* for intel drm */
/* nexell isp viewer */
#include "nx_device_control.h"

#define NxDebug(...) //printf(__VA_ARGS__)
#define NxDebugErr(...) printf(__VA_ARGS__)

/* functions related to V4L2 */
int NxV4l2Qbuf(int fd, unsigned int index,
                     unsigned int buf_type,
                     unsigned int mem_type,
                     int dma_fd, int length)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[3];
    int ret = 0;
    NxDebug("[%s] %d \n", __func__, index);
    bzero(&v4l2_buf, sizeof(v4l2_buf));
    v4l2_buf.length = 1; /* the number of plane for multi plane*/
    v4l2_buf.type = buf_type;
    v4l2_buf.memory = mem_type;
    v4l2_buf.index = index;
    v4l2_buf.m.planes = planes;
    v4l2_buf.m.planes[0].m.fd = dma_fd;
    v4l2_buf.m.planes[0].length = length;
    ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
    if (ret)
        NxDebugErr("failed to qbuf %d, index %d \n", ret, index);
    else
        NxDebug("index is %d \n", v4l2_buf.index);

    return ret;
}

int Nxv4l2Dqbuf(int fd, unsigned int buf_type,
                      unsigned int mem_type, unsigned int *index)
{
    int ret = 0;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[3];
    NxDebug("[%s] %d \n", __func__, *index);
    bzero(&v4l2_buf, sizeof(v4l2_buf));
    v4l2_buf.type = buf_type;
    v4l2_buf.memory = mem_type;
    v4l2_buf.length = 1;
    v4l2_buf.m.planes = planes;

    ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
    if (ret)
        NxDebugErr("failed to dqbuf %d, index %d \n", ret, *index);
    else {
        NxDebug("[%s] index is %d \n", __func__, v4l2_buf.index);
        *index = v4l2_buf.index;
    }
    return ret;
}

int NxV4l2Reqbuf(int video_fd,
                   int buf_count,
                   unsigned int buf_type)
{
    struct v4l2_requestbuffers req;
    int ret = 0;

    NxDebug("[%s]\n", __func__);

    bzero(&req, sizeof(req));
    req.count = buf_count;
    req.memory = V4L2_MEMORY_DMABUF;
    req.type = buf_type;
    ret = ioctl(video_fd, VIDIOC_REQBUFS, &req);
    if (ret)
        NxDebugErr("failed to reqbuf %d \n", ret);

    return ret;
}

int NxV4l2StreamOn(int video_fd,
                      unsigned int buf_type)
{
    int ret = 0;

    NxDebug("[%s]\n", __func__);

    ret = ioctl(video_fd, VIDIOC_STREAMON, &buf_type);
    if (ret)
        NxDebugErr("failed to start device : %d\n", ret);
    return ret;
}

int NxV4l2StreamOff(int video_fd,
                       unsigned int buf_type)
{
    int ret = 0;

    NxDebug("[%s]\n", __func__);
    ret = ioctl(video_fd, VIDIOC_STREAMOFF, &buf_type);
    if (ret)
        NxDebugErr("failed to stop device : %d\n", ret);
    return ret;
}

int NxV4l2SetRegister(int video_fd,
                         int type,
                         unsigned int addr,
                         unsigned int data)
{
    int ret = 0;
    struct v4l2_dbg_register reg;

    NxDebug("[%s]\n", __func__);
    reg.size = 4;
    reg.reg = addr;
    reg.val = data;

    if (!type) {
        reg.match.type = V4L2_CHIP_MATCH_BRIDGE;
        reg.match.addr = type;
    } else {
        reg.match.type = V4L2_CHIP_MATCH_BRIDGE;
        //reg.match.type = V4L2_CHIP_MATCH_SUBDEV;
        reg.match.addr = type;
    }

    ret = ioctl(video_fd, VIDIOC_DBG_S_REGISTER, &reg);
    if (ret)
        NxDebugErr("failed to set register : %d\n", ret);
    return ret;
}

int NxV4l2GetRegister(int video_fd,
                         int type,
                         unsigned int addr,
                         unsigned int *data)
{
    int ret = 0;
    struct v4l2_dbg_register reg;

    NxDebug("[%s]\n", __func__);
    reg.size = 4;
    reg.reg = addr;
    if (!type)
        reg.match.type = V4L2_CHIP_MATCH_BRIDGE;
    else
        reg.match.type = V4L2_CHIP_MATCH_SUBDEV;

    ret = ioctl(video_fd, VIDIOC_DBG_G_REGISTER, &reg);
    if (ret)
        NxDebugErr("failed to read register : %d\n", ret);
    else {
        *data = reg.val;
        NxDebug("data is 0x%8x \n",*data);
    }
    return ret;
}

static int v4l2_set_format(int video_fd,
                           unsigned int w,
                           unsigned int h,
                           unsigned int buf_type,
                           unsigned int format)
{
    struct v4l2_format v4l2_fmt;
    int ret = 0;

    NxDebug("[%s]\n", __func__);

    bzero(&v4l2_fmt, sizeof(v4l2_fmt));
    v4l2_fmt.type = buf_type;
    v4l2_fmt.fmt.pix_mp.width = w;
    v4l2_fmt.fmt.pix_mp.height = h;
    v4l2_fmt.fmt.pix_mp.pixelformat = format;
    v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    ret = ioctl(video_fd, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret)
        NxDebugErr("failed to set format %d \n", ret);
    return ret;
}

static int v4l2_set_crop(int video_fd,
                         int x, int y,
                         int w, int h,
                         unsigned int buf_type)
{
    int ret = 0;
    struct v4l2_crop crop;

    NxDebug("[%s]\n", __func__);
    crop.type = buf_type;
    crop.c.width = w;
    crop.c.height = h;
    crop.c.left = x;
    crop.c.top = y;
    ret = ioctl(video_fd, VIDIOC_S_CROP, &crop);
    if (ret)
        NxDebugErr("failed to set image crop : %d\n", ret);
    return ret;
}
/* end functions related to V4L2 */

/* functions related to DRM */
static int drm_ioctl(int drm_fd, unsigned long request, void *arg)
{
    int ret;

    do {
            ret = ioctl(drm_fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}
void NxFreeGem(int drm_fd, int gem_fd)
{
    struct drm_gem_close arg = {0, };
    arg.handle = gem_fd;
    drm_ioctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &arg);
}

int NxAllocGem(int drm_fd, int size)
{
    struct drm_i915_gem_create arg = { 0, };
    int gem_fd = -1;

    arg.size = size;
    gem_fd = drm_ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &arg);
    if (gem_fd < 0) {
        NxDebugErr("failed to gem alloc \n");
        return -ENOMEM;
    }
    gem_fd = arg.handle;
    return gem_fd;
}

int NxGetDmafd(int drm_fd, int gem_fd)
{
    struct drm_prime_handle arg = { 0, };
    int dma_fd = -1;

    arg.handle = gem_fd;
    dma_fd = drm_ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &arg);
    if (dma_fd < 0) {
        NxDebugErr("failed to dma handle %d \n", dma_fd);
        return -ENOMEM;
    }
    dma_fd = arg.fd;
    return dma_fd;
}

int NxGetVaddr(int drm_fd, int gem_fd, int size, void **vaddr)
{
    struct drm_mode_map_dumb arg = { 0, };
    int ret = -1;
    void * map = NULL;

    arg.handle = gem_fd;
    ret = drm_ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
    if (ret < 0) {
        NxDebugErr("failed to virtual address using gem handle %d \n", ret);
        return -ENOMEM;
    }
    map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd,
                arg.offset);
    if(map == MAP_FAILED) {
        NxDebugErr("failed to mmap \n");
        return -1;
    }
    *vaddr = map;
    return 0;
}
/* end  functions related to DRM */

int NxOpenDrmDevice(void)
{
    int drm_fd = -1;
    NxDebug("[%s] \n", __func__);
    drm_fd = open("/dev/dri/card0", O_RDWR);
    if (drm_fd < 0)
        NxDebugErr("failed to open drm device - %d \n", drm_fd);
    return drm_fd;
}

int NxOpenVideoDevice(void)
{
    int video_fd = -1;
    NxDebug("[%s] \n", __func__);
    video_fd = open("/dev/video7", O_RDWR);
    if(video_fd < 0)
        NxDebugErr("failed to open video device : %d \n", video_fd);
    return video_fd;
}

int NxSetImgFormat(int video_fd,
                   unsigned int w, unsigned int h,
                   unsigned int buf_type, unsigned int format)
{
    int ret = -1;

    NxDebug("[%s] \n", __func__);
    NxDebug("width:%d, height:%d, buf_type:%x, format:%x \n",
             w, h, buf_type, format);
    /* set format */
    ret = v4l2_set_format(video_fd, w, h, buf_type, format);
    /* set crop */
    ret = v4l2_set_crop(video_fd, 0, 0, w, h, buf_type);
    //ret = v4l2_set_crop(video_fd, 1, 1, 1600, 1200, buf_type);

    return ret;
}
