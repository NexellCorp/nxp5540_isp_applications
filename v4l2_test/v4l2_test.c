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

#define MAX_BUFFER_COUNT	4

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

static int saveBMP
(uint32_t width, uint32_t height, uint8_t *srcbuf)
{
	uint32_t rgb_size, rgb_header = 0;
	uint8_t *rgbbuf;
	int fd = 0;
	initConvMatrix();
	printf("create a file to copy the image data \n");
	fd = creat("./resultImage.bmp", 0644);
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
/* EBUSY = 16      /* Device or resource busy */
/* EINVAL = 22 - Invalid Argument */
/* ENFILE = 23      /* File table overflow */
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

static int capture_test(int drm_fd, char *video,
		uint32_t w, uint32_t h, uint32_t format)
{
	int video_fd, ret = -1;
	struct v4l2_format v4l2_fmt;
	struct v4l2_requestbuffers req;
	uint32_t size, i = 0;
	uint32_t buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	int gem_fds[MAX_BUFFER_COUNT] = {-1, };
	int dma_fds[MAX_BUFFER_COUNT] = {-1, };
	void *pVaddrs[MAX_BUFFER_COUNT] = {NULL, };
	int loop_count = 100;

	printf("Capture Test \n");
	video_fd = open(video, O_RDWR);
	if(video_fd < 0) {
		printf("failed to open video device \n");
		ret = -ENODEV;
		goto done;
	}
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
	printf("request bufs : 0x%x \n", VIDIOC_REQBUFS);
	bzero(&req, sizeof(req));
	req.count = MAX_BUFFER_COUNT;
	req.memory = V4L2_MEMORY_DMABUF;
	req.type = buf_type;
	ret = ioctl(video_fd, VIDIOC_REQBUFS, &req);
	if (ret) {
		printf("failed to eeqbuf %d \n", ret);
		goto done;
	}
	printf("create gem, dma handler and get virtual address \n");
	size = CAL_YUVSIZE(w, h);
	if (size <= 0) {
		printf("invalid alloc size %lu \n", size);
		ret = -EINVAL;
		goto done;
	}

	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = allocGem(drm_fd, size);
		if (ret < 0) {
			printf("failed to gem alloc \n");
			ret = -ENOMEM;
			goto done;
		}
		gem_fds[i] = ret;

		ret = getDmafd(drm_fd, gem_fds[i]);
		if (ret < 0) {
			printf("failed to dma handler using gem handle %d\n",ret);
			ret = -ENOMEM;
			goto done;
		}
		dma_fds[i] = ret;
		ret = getVaddr(drm_fd, gem_fds[i], size, &pVaddrs[i]);
		if (ret < 0) {
			printf("failed to get virtual address \n");
		}
		printf("[%d] gemfd = %d, dmafd = %d, vaddr = 0x%x \n",
				i, gem_fds[i], dma_fds[i], pVaddrs[i]);
	}
	/* qbuf */
	printf("qbuf \n");
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		ret = v4l2_qbuf(video_fd, i, buf_type, V4L2_MEMORY_DMABUF,
				dma_fds[i], size);
		if (ret) {
			printf("failed qbuf index %d \n", i);
			goto done;
		}
	}
	printf("stream on \n");
	ret = ioctl(video_fd, VIDIOC_STREAMON, &buf_type);
	if (ret) {
		printf("failed to stream on : %d\n", ret);
		goto done;
	}

	while (loop_count--) {
		int dq_index;
		printf("loop_count is %d \n", loop_count);

		ret = v4l2_dqbuf(video_fd, buf_type, V4L2_MEMORY_DMABUF,
				&dq_index);
		if (ret) {
			printf("failed to dqbuf \n");
			goto done;
		}
		printf("dqbuf index %d \n", dq_index);
		printf("save BMP\n");
		saveBMP(w, h, (uint8_t *)pVaddrs[dq_index]);
		printf("qbuf index =  %d \n", dq_index);
		ret = v4l2_qbuf(video_fd, dq_index, buf_type,
				V4L2_MEMORY_DMABUF, dma_fds[dq_index],
				size);
		if (ret) {
			printf("failed to qbuf index %d \n", dq_index);
			goto done;
		}
	}
	printf("stream off \n");
	ioctl(video_fd, VIDIOC_STREAMOFF, &buf_type);

done:
	printf("done ---- \n");

	close(video_fd);

	/* free buffers */
	for (i = 0; i < MAX_BUFFER_COUNT; i++) {
		if (dma_fds[i] >= 0)
			close(dma_fds[i]);
		if (gem_fds[i] >= 0)
			close(gem_fds[i]);
	}

	return ret;
}

int main(int argc, char **argv)
{
	int ret, drm_fd;
	uint32_t width, height, format = 0;

	printf("V4L2 TEST Application \n");

	width = 1600;
	height = 1200;
	format = V4L2_PIX_FMT_UYVY;
	drm_fd = open("/dev/dri/card0", O_RDWR);
	if (drm_fd < 0) {
		printf("failed to open drm_device \n");
		return -1;
	}

	ret = capture_test(drm_fd, argv[1], width, height, format);
	if (ret < 0) {
		printf("failed to do capture test \n");
		goto done;
	}

done:
	if(drm_fd)
		close(drm_fd);

	return ret;
}
