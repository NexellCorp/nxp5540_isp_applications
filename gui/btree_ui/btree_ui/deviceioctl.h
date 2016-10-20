#ifndef V4L2_IOCTL_H
#define V4L2_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BUFFER_COUNT	4

struct device {
    /* drm information */
    int drm_fd;
    int gem_fds[MAX_BUFFER_COUNT];
    int dma_fds[MAX_BUFFER_COUNT];
    void *pVaddrs[MAX_BUFFER_COUNT];

    /* queue */
    unsigned int dq_index;
    /* vidoe frame inforamtion */
    int video_fd;
    unsigned int format;
    unsigned int buf_type;
    unsigned int width;
    unsigned int height;
    unsigned int size;
};

struct device* openDevice(void);

void closeDevice(struct device *dev);


int initDevice(struct device* dev,
               unsigned int w, unsigned int h,
               unsigned int buf_type, unsigned int format);

int startDevice(struct device* dev);
int stopDevice(struct device* dev);

#ifdef __cplusplus
}
#endif

#endif // V4L2_IOCTL_H
