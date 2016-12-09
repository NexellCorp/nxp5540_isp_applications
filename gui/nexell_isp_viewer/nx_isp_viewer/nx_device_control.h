#ifndef NX_DEVICE_CONTROL_H
#define NX_DEVICE_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

int NxOpenDrmDevice(void);
int NxOpenVideoDevice(void);
int NxSetImgFormat(int video_fd,
                   unsigned int w,
                   unsigned int h,
                   unsigned int buf_type,
                   unsigned int format);
int NxGetVaddr(int drm_fd,
                int gem_fd,
                int size,
                void **vaddr);
int NxGetDmafd(int drm_fd, int gem_fd);
int NxAllocGem(int drm_fd, int size);
void NxFreeGem(int drm_fd, int gem_fd);
int NxV4l2Reqbuf(int video_fd,
                   int buf_count,
                   unsigned int buf_type);
int Nxv4l2Dqbuf(int fd,
                  unsigned int buf_type,
                  unsigned int mem_type,
                  unsigned int *index);

int NxV4l2Qbuf(int fd, unsigned int index,
                 unsigned int buf_type,
                 unsigned int mem_type,
                 int dma_fd, int length);
int NxV4l2StreamOn(int video_fd,
                          unsigned int buf_type);
int NxV4l2StreamOff(int video_fd,
                       unsigned int buf_type);
int NxV4l2GetRegister(int video_fd,
                         int type,
                         unsigned int addr,
                         unsigned int *data);
int NxV4l2SetRegister(int video_fd,
                         int type,
                         unsigned int addr,
                         unsigned int data);
#ifdef __cplusplus
}
#endif

#endif // NX_DEVICE_CONTROL_H
