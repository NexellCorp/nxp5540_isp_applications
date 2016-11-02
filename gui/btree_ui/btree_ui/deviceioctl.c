#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/ioctl.h>
#include <sys/mman.h> /* PROT_READ/PROT_WRITE/MAP_SHARED/mmap/munmap */
#include <linux/types.h>
#include <linux/videodev2.h> /* for V4L2 */
#include <drm/i915_drm.h> /* for intel drm */

#include "deviceioctl.h"

typedef	struct	_BMPTYPEHEADER {		/* for byte aligned read */
    unsigned char	btTYPE[2];					/* "BM" */
} BMPTYPEHEADER;

typedef	struct	_BMPFILEHEADER {
    unsigned int	bfSize;						/* file size */
    unsigned short	bfReserved1;
    unsigned short	bfReserved2;
    unsigned int	bfOffBits;					/* offset value to image data */
} BMPFILEHEADER;

typedef	struct	_BMPINFOHEADER {
    unsigned int	biSize;						/* size of image header structure */
    unsigned int	biWidth;					/* image width */
    unsigned int	biHeight;					/* image height */
    unsigned short	biPlane;					/* always '1' (bit plane) */
    unsigned short	biBitCount;					/* bit per pixel */
    unsigned int	biCompression;				/* compression type */
    unsigned int	biSizeImage;				/* byte size of image */
    unsigned int	biXPelsPerMeter;			/* horizontal resolution*/
    unsigned int	biYPelsPerMeter;			/* vertical resolution */
    unsigned int	biClrUsed;					/* the number of used color */
    unsigned int	biClrImportant;				/* important color index (0:all) */
} BMPINFOHEADER;

/* functions related to image converting */
int		gnCoefY2R[9][256];		/* coeff for yuv-to-rgb conversion */
#define	CAL_DUMMY(width)		((4-(((width)*3)%4))%4)
#define CAL_RGBSIZE(width, height)	(((width*3)+CAL_DUMMY(width))*height);
#define CAL_YUVSIZE(width, height)	(((width*2)+CAL_DUMMY(width))*height);
#define	Y2R_INT_SHIFT	10
#define	Y2R_INT_MUL		(1<<Y2R_INT_SHIFT)
#define	Y2R_INT_RND		(1<<(Y2R_INT_SHIFT-1))
#define CHK_RANGE(a)	((a)<0)?(0):(((a)>255)?(255):(a))
#define	Y2R_R(y,cr)		((gnCoefY2R[0][y]+gnCoefY2R[2][cr]+Y2R_INT_RND)>>Y2R_INT_SHIFT)
#define	Y2R_G(y,cb,cr)	((gnCoefY2R[3][y]+gnCoefY2R[4][cb]+gnCoefY2R[5][cr]+Y2R_INT_RND)>>Y2R_INT_SHIFT)
#define	Y2R_B(y,cb)		((gnCoefY2R[6][y]+gnCoefY2R[7][cb]+Y2R_INT_RND)>>Y2R_INT_SHIFT)

/* function related to make header information for RGB */
static int MakeRGBHeader (int handle, unsigned int m_hsize/*width*/, unsigned int m_vsize/*height*/)
{
    BMPTYPEHEADER	m_bt;
    BMPFILEHEADER	m_bf;
    BMPINFOHEADER	m_bi;
    int ret;
    /* --- make header --- */
    m_bt.btTYPE[0] = 'B';
    m_bt.btTYPE[1] = 'M';

    m_bf.bfSize = CAL_RGBSIZE(m_hsize, m_vsize)
            + sizeof(BMPTYPEHEADER) + sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER);
    m_bf.bfReserved1 = 0;
    m_bf.bfReserved2 = 0;
    m_bf.bfOffBits =
        sizeof(BMPTYPEHEADER) + sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER);

    m_bi.biSize = sizeof(BMPINFOHEADER);
    m_bi.biWidth = m_hsize;
    m_bi.biHeight = m_vsize;
    m_bi.biPlane = 1;
    m_bi.biBitCount = 24;
    m_bi.biCompression = 0;
    m_bi.biSizeImage = CAL_RGBSIZE(m_hsize, m_vsize);
    m_bi.biXPelsPerMeter = 0;
    m_bi.biYPelsPerMeter = 0;
    m_bi.biClrUsed = 0;
    m_bi.biClrImportant = 0;

    ret = write(handle,&m_bt, sizeof(BMPTYPEHEADER));
    if (ret < 0)
        printf("fail to write bmp type header into the file \n");
    ret = write(handle,&m_bf, sizeof(BMPFILEHEADER));
    if (ret < 0)
        printf("fail to write bmp file header into the file \n");
    ret = write(handle,&m_bi, sizeof(BMPINFOHEADER));
    if (ret < 0)
        printf("fail to write bmp info header into the file \n");

    return ret;
}

/* functions related to image converting */
static void ConvDib(int width, int height, unsigned char *pSrc, unsigned char *pDst)
{
    int     x, y, nDummy;
    int		y1, cb, y2, cr, r, g, b;
    printf("[%s] \n",__func__);
    nDummy = CAL_DUMMY(width);
    printf("width = %d, height = %d \n", width, height);
    for( y=0 ; y < height ; y++ )
    {
        for( x=0 ; x < width ; x+=2 )
        {
            /* get yuv data */
            y1 = pSrc[1];	cb = pSrc[0];
            y2 = pSrc[3];	cr = pSrc[2];

            /* convert rgb */
            b = Y2R_B( y1, cb );		pDst[0] = CHK_RANGE( b );
            g = Y2R_G( y1, cb, cr );	pDst[1] = CHK_RANGE( g );
            r = Y2R_R( y1, cr );		pDst[2] = CHK_RANGE( r );
            b = Y2R_B( y2, cb );		pDst[3] = CHK_RANGE( b );
            g = Y2R_G( y2, cb, cr );	pDst[4] = CHK_RANGE( g );
            r = Y2R_R( y2, cr );		pDst[5] = CHK_RANGE( r );

            /* update buffer pointer */
            pSrc += 4;
            pDst += 6;
        }
        pDst += nDummy;
    }
    printf("converting is finished\n");
}

static void initConvMatrix(void)
{
    int i;

    /* initialize color conversion matrix (YCbCr-to-RGB) */
    /* R = { 1.000f x 1024,	 0.000f x 1024,	 1.371f x 1024	}	{ Y  } */
    /* G = { 1.000f x 1024,	-0.336f x 1024,	-0.698f x 1024	}	{ Cb } */
    /* B = { 1.000f x 1024,	 1.732f x 1024,	 0.000f x 1024	}	{ Cr } */
    for( i=0 ; i<256 ; i++ )
    {
        gnCoefY2R[0][i] = Y2R_INT_MUL*i;
        gnCoefY2R[1][i] = 0;
        gnCoefY2R[2][i] = (int)(1.371*Y2R_INT_MUL*(i-128));
        gnCoefY2R[3][i] = Y2R_INT_MUL*i;
        gnCoefY2R[4][i] = (int)(-0.336*Y2R_INT_MUL*(i-128));
        gnCoefY2R[5][i] = (int)(-0.698*Y2R_INT_MUL*(i-128));
        gnCoefY2R[6][i] = Y2R_INT_MUL*i;
        gnCoefY2R[7][i] = (int)(1.732*Y2R_INT_MUL*(i-128));
        gnCoefY2R[8][i] = 0;
    }
}

int saveBMP
(uint32_t width, uint32_t height, uint8_t *srcbuf)
{
    uint32_t rgb_size, rgb_header = 0;
    uint8_t *rgbbuf;
    int fd = 0;
    initConvMatrix();
    printf("create a file to copy the image data \n");
    fd = creat("./resultImage.bmp", 0644);
    //fd = open("./resultImage.bmp",O_RDWR);
    if (fd < 0) {
        printf("fail to create a file \n");
        return -ENODEV;
    }

    rgb_header = sizeof(BMPINFOHEADER) + sizeof(BMPTYPEHEADER) + sizeof(BMPFILEHEADER);
    rgb_size = CAL_RGBSIZE(width, height);
    rgbbuf = (unsigned char*)malloc(rgb_size);
    if (!rgbbuf) {
        printf("fail to malloc for rgb data \n");
        close(fd);
        return -1;
    }

    printf("rgbbuf = 0x%x , header size = %d, data size = %d\n", rgbbuf, rgb_header, rgb_size);
    /* input header information for RGB */
    printf("input header information for RGB\n");
    MakeRGBHeader(fd, width, height);
    printf("converting yuv data to rgb data \n");
    ConvDib(width, height, srcbuf, rgbbuf);

    if (write(fd,rgbbuf, rgb_size) < 0) {
        printf("fail to write the data into the file \n");
        if(rgbbuf)
            free(rgbbuf);
        if(fd)
            close(fd);
        return -1;
    }
    printf("finish copying \n");

    if(rgbbuf)
        free(rgbbuf);
    if(fd)
        close(fd);
    return 0;
}

uint8_t* convertRGB
(uint32_t width, uint32_t height, uint8_t *srcbuf)
{
    uint32_t rgb_size = 0;
    uint8_t *rgbbuf;

    rgb_size = CAL_RGBSIZE(width, height);
    rgbbuf = (unsigned char*)malloc(rgb_size);
    if (!rgbbuf) {
        printf("fail to malloc for rgb data \n");
        return -1;
    }
    initConvMatrix();

    printf("converting yuv data to rgb data \n");
    ConvDib(width, height, srcbuf, rgbbuf);

    return rgbbuf;
}

static int drm_ioctl(int drm_fd, unsigned long request, void *arg)
{
    int ret;

    do {
            ret = ioctl(drm_fd, request, arg);
    } while (ret == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

static int allocGem(int drm_fd, int size)
{
    struct drm_i915_gem_create arg = { 0, };
    int gem_fd = -1;

    arg.size = size;
    gem_fd = drm_ioctl(drm_fd, DRM_IOCTL_I915_GEM_CREATE, &arg);
    if (gem_fd < 0) {
        printf("failed to gem alloc \n");
        return -ENOMEM;
    }
    gem_fd = arg.handle;
    return gem_fd;
}
/* error code */
/* EPERM = 1 - operation not permitted */
/* ENOENT = 2 - No such file or directory */
/* EBUSY = 16    Device or resource busy */
/* EINVAL = 22 - Invalid Argument */
/* ENFILE = 23   File table overflow */
/* ENOSYS = 38 - Function not implemented */
/* ENOMEM = 12 - Out of Memory */
static int getDmafd(int drm_fd, int gem_fd)
{
    struct drm_prime_handle arg = { 0, };
    int dma_fd = -1;

    arg.handle = gem_fd;
    dma_fd = drm_ioctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &arg);
    if (dma_fd < 0) {
        printf("failed to dma handle %d \n", dma_fd);
        return -ENOMEM;
    }
    dma_fd = arg.fd;
    return dma_fd;
}

static int getVaddr(int drm_fd, int gem_fd, int size, void **vaddr)
{
    struct drm_mode_map_dumb arg = { 0, };
    int ret = -1;
    void * map = NULL;

    arg.handle = gem_fd;
    ret = drm_ioctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
    if (ret < 0) {
        printf("failed to virtual address using gem handle %d \n", ret);
        return -ENOMEM;
    }
    map = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd,
                arg.offset);
    if(map == MAP_FAILED) {
        printf("failed to mmap \n");
        return -1;
    }
    *vaddr = map;
    return 0;
}

static int v4l2_qbuf(int fd, int index, uint32_t buf_type, uint32_t mem_type,
        int dma_fd, int length)
{
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[3];

    bzero(&v4l2_buf, sizeof(v4l2_buf));
    v4l2_buf.length = 1; /* the number of plane for multi plane*/
    v4l2_buf.type = buf_type;
    v4l2_buf.memory = mem_type;
    v4l2_buf.index = index;
    v4l2_buf.m.planes = planes;
    v4l2_buf.m.planes[0].m.fd = dma_fd;
    v4l2_buf.m.planes[0].length = length;
    return ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
}

static int v4l2_dqbuf(int fd, uint32_t buf_type, uint32_t mem_type, int *index)
{
    int ret;
    struct v4l2_buffer v4l2_buf;
    struct v4l2_plane planes[3];

    bzero(&v4l2_buf, sizeof(v4l2_buf));
    v4l2_buf.type = buf_type;
    v4l2_buf.memory = mem_type;
    v4l2_buf.length = 1;
    v4l2_buf.m.planes = planes;

    ret = ioctl(fd, VIDIOC_DQBUF, &v4l2_buf);
    if (ret)
        return ret;

    *index = v4l2_buf.index;
    return 0;
}

int startDevice(struct device *dev)
{
    int ret = -1;

    printf("startDevice \n");
    if (!dev)
        return ret;
    ret = ioctl(dev->video_fd, VIDIOC_STREAMON, &dev->buf_type);
    if (ret) {
        printf("failed to start device : %d\n", ret);
    }
    return ret;
}

int stopDevice(struct device *dev)
{
    int ret = -1;

    printf("stop Device \n");
    if (!dev)
        return ret;
    ret = ioctl(dev->video_fd, VIDIOC_STREAMOFF, &dev->buf_type);
    if (ret) {
        printf("failed to stop device : %d\n", ret);
    }
    return ret;
}
int setCropImage(struct device *dev, int x, int y, int w, int h)
{
    int ret = -1;
    struct v4l2_crop crop;

    printf("set crop image \n");
    if (!dev)
        return ret;
    crop.type = dev->buf_type;
    crop.c.width = w;
    crop.c.height = h;
    crop.c.left = x;
    crop.c.top = y;
    ret = ioctl(dev->video_fd, VIDIOC_S_CROP, &crop);
    if (ret) {
        printf("failed to set image crop : %d\n", ret);
    }
    return ret;
}

int dqbuf(struct device *dev)
{
    int ret = -1;

    printf("dqbuf \n");

    if (!dev)
        return ret;
    ret = v4l2_dqbuf(dev->video_fd, dev->buf_type, V4L2_MEMORY_DMABUF,
                     &dev->dq_index);
    if (ret) {
        printf("failed to dqbuf \n");
    }
    printf("dqbuf index %d \n", dev->dq_index);
    return ret;
}

int qbuf(struct device *dev)
{
    int ret = -1;
    printf("qbuf index =  %d \n", dev->dq_index);
    ret = v4l2_qbuf(dev->video_fd, dev->dq_index, dev->buf_type,
                    V4L2_MEMORY_DMABUF, dev->dma_fds[dev->dq_index],
                    dev->size);
    if (ret) {
        printf("failed to qbuf index %d \n", dev->dq_index);
    }
    return ret;
}

int readData(struct device *dev)
{
    int ret = -1;

    printf("readData\n");

    if (!dev)
        return ret;
    ret = v4l2_dqbuf(dev->video_fd, dev->buf_type, V4L2_MEMORY_DMABUF,
                     &dev->dq_index);
    if (ret) {
        printf("failed to dqbuf \n");
    }
    printf("dqbuf index %d \n", dev->dq_index);
    printf("save BMP\n");
    saveBMP(dev->width, dev->height, (uint8_t *)dev->pVaddrs[dev->dq_index]);
    printf("qbuf index =  %d \n", dev->dq_index);
    ret = v4l2_qbuf(dev->video_fd, dev->dq_index, dev->buf_type,
                    V4L2_MEMORY_DMABUF, dev->dma_fds[dev->dq_index],
                    dev->size);
    if (ret) {
        printf("failed to qbuf index %d \n", dev->dq_index);
    }

    return ret;
}

int initDevice(struct device* dev,
               unsigned int w, unsigned int h,
               unsigned int buf_type, unsigned int format)
{
    int ret = -1;
    struct v4l2_format v4l2_fmt;
    struct v4l2_requestbuffers req;
    uint32_t size, i = 0;
    int drm_fd = dev->drm_fd;
    int video_fd = dev->video_fd;

    /* set format */
    printf("set format : 0x%x \n", VIDIOC_S_FMT);
    bzero(&v4l2_fmt, sizeof(v4l2_fmt));
    v4l2_fmt.type = buf_type;
    v4l2_fmt.fmt.pix_mp.width = w;
    v4l2_fmt.fmt.pix_mp.height = h;
    v4l2_fmt.fmt.pix_mp.pixelformat = format;
    v4l2_fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
    ret = ioctl(video_fd, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret) {
        printf("failed to set format %d \n", ret);
        goto done;
    }
    dev->buf_type = buf_type;
    dev->format = format;

    setCropImage(dev, 0, 0, w, h);
    printf("request bufs : 0x%x \n", VIDIOC_REQBUFS);
    bzero(&req, sizeof(req));
    req.count = MAX_BUFFER_COUNT;
    req.memory = V4L2_MEMORY_DMABUF;
    req.type = buf_type;
    ret = ioctl(video_fd, VIDIOC_REQBUFS, &req);
    if (ret) {
        printf("failed to reqbuf %d \n", ret);
        goto done;
    }
    printf("create gem, dma handler and get virtual address \n");
    size = CAL_YUVSIZE(w, h);
    if (size <= 0) {
        printf("invalid alloc size %lu \n", size);
        ret = -EINVAL;
        goto done;
    }
    dev->size = size;

    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        ret = allocGem(drm_fd, size);
        if (ret < 0) {
            printf("failed to gem alloc \n");
            ret = -ENOMEM;
            goto done;
        }
        dev->gem_fds[i] = ret;

        ret = getDmafd(drm_fd, dev->gem_fds[i]);
        if (ret < 0) {
            printf("failed to dma handler using gem handle %d\n",ret);
            ret = -ENOMEM;
            goto done;
        }
        dev->dma_fds[i] = ret;
        ret = getVaddr(drm_fd, dev->gem_fds[i], size, &dev->pVaddrs[i]);
        if (ret < 0) {
            printf("failed to get virtual address \n");
            goto done;
        }

    }
    /* qbuf */
    printf("qbuf \n");
    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        ret = v4l2_qbuf(video_fd, i, buf_type, V4L2_MEMORY_DMABUF,
                dev->dma_fds[i], size);
        if (ret) {
            printf("failed qbuf index %d \n", i);
        }
    }

    return ret;
done:
    /* free buffers */
    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        if (dev->dma_fds[i] >= 0)
            close(dev->dma_fds[i]);
        if (dev->gem_fds[i] >= 0)
            close(dev->gem_fds[i]);
    }
    return ret;
}

static void initValue(struct device *dev)
{
    int i;

    dev->drm_fd = -1;
    dev->video_fd = -1;

    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        dev->dma_fds[i] = -1;
        dev->gem_fds[i] = -1;
        dev->pVaddrs[i] = NULL;
    }
    dev->dq_index = 0;
    dev->format = 0;
    dev->buf_type = 0;
    dev->width = 800;
    dev->height = 600;
    dev->size = 0;
}

struct device* openDevice(void)
{
    int drm_fd = -1, video_fd = -1;
    struct device *dev = NULL;
    int ret = -1;

    printf("[%s]\n",__func__);

    dev = malloc(sizeof(struct device));
    if(!dev) {
        printf("failed to malloc for video device");
        return NULL;
    }
    initValue(dev);

    drm_fd = open("/dev/dri/card0", O_RDWR);
    if (drm_fd < 0) {
        goto done;
    }

    video_fd = open("/dev/video7", O_RDWR);
    if(video_fd < 0) {
        printf("failed to open video device : %d \n", video_fd);
        goto done;
    }

    dev->drm_fd = drm_fd;
    dev->video_fd = video_fd;
    dev->dq_index = 0;

    ret = initDevice(dev, dev->width,  dev->height,
                     V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE, V4L2_PIX_FMT_UYVY);
    if(ret < 0)
        goto done;

    return dev;

done:
    printf("failed to open device\n");
    if(video_fd)
        close(video_fd);
    if(drm_fd)
        close(drm_fd);
    if(dev)
        free(dev);
    return NULL;
}

void closeDevice(struct device *dev)
{
    int i;

    printf("[%s] \n",__func__);

    if(!dev)
        return;

    if(dev->video_fd)
        close(dev->video_fd);
    /* free buffers */
    for (i = 0; i < MAX_BUFFER_COUNT; i++) {
        if (dev->dma_fds[i] >= 0)
            close(dev->dma_fds[i]);
        if (dev->gem_fds[i] >= 0)
            close(dev->gem_fds[i]);
    }
    if(dev->drm_fd)
        close(dev->drm_fd);

    if(dev)
        free(dev);
}
