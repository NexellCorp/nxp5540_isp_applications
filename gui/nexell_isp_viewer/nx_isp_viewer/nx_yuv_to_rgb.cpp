/* standard */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
/* nexell isp viewer */
#include "nx_debug.h"

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

    m_bf.bfSize = CAL_RGBSIZE(m_hsize, m_vsize) + sizeof(BMPTYPEHEADER) + sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER);
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
        NxDebug("fail to write bmp type header into the file \n");
    ret = write(handle,&m_bf, sizeof(BMPFILEHEADER));
    if (ret < 0)
        NxDebug("fail to write bmp file header into the file \n");
    ret = write(handle,&m_bi, sizeof(BMPINFOHEADER));
    if (ret < 0)
        NxDebug("fail to write bmp info header into the file \n");

    return ret;
}

/* functions related to image converting */
static void ConvDib(int width, int height, unsigned char *pSrc, unsigned char *pDst)
{
    int     x, y, nDummy;
    int		y1, cb, y2, cr, r, g, b;
    NxDebug("[%s] \n",__func__);
    nDummy = CAL_DUMMY(width);
    NxDebug("width = %d, height = %d \n", width, height);

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
    NxDebug("converting is finished\n");
}

static void InitConvMatrix(void)
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

int NxSaveBMP(unsigned int width,
	      unsigned int height,
	      unsigned char *srcbuf)
{
    unsigned int rgb_size, rgb_header = 0;
    unsigned char *rgbbuf;
    int fd = 0;

    InitConvMatrix();

    NxDebug("create a file to copy the image data \n");
    fd = creat("./resultImage.bmp", 0644);
    //fd = open("./resultImage.bmp",O_RDWR);
    if (fd < 0) {
        NxDebug("fail to create a file \n");
        return -ENODEV;
    }

    rgb_header = sizeof(BMPINFOHEADER) + sizeof(BMPTYPEHEADER) + sizeof(BMPFILEHEADER);
    rgb_size = CAL_RGBSIZE(width, height);
    rgbbuf = (unsigned char*)malloc(rgb_size);
    if (!rgbbuf) {
        NxDebug("fail to malloc for rgb data \n");
        close(fd);
        return -1;
    }

    NxDebug("rgbbuf = %p , header size = %d, data size = %d\n", rgbbuf, rgb_header, rgb_size);
    /* input header information for RGB */
    NxDebug("input header information for RGB\n");
    MakeRGBHeader(fd, width, height);
    NxDebug("converting yuv data to rgb data \n");
    ConvDib(width, height, srcbuf, rgbbuf);

    if (write(fd,rgbbuf, rgb_size) < 0) {
        NxDebug("fail to write the data into the file \n");
        if(rgbbuf)
            free(rgbbuf);
        if(fd)
            close(fd);
        return -1;
    }
    NxDebug("finish copying \n");

    if(rgbbuf)
        free(rgbbuf);
    if(fd)
        close(fd);
    return 0;
}

unsigned char* NxConvertRGB(unsigned int width,
			    unsigned int  height,
			    unsigned char *srcbuf)
{
    unsigned int rgb_size = 0;
    unsigned char *rgbbuf = NULL;

    rgb_size = CAL_RGBSIZE(width, height);
    rgbbuf = (unsigned char*)malloc(rgb_size);
    if (!rgbbuf) {
        NxDebug("fail to malloc for rgb data \n");
        return NULL;
    }
    InitConvMatrix();

    NxDebug("converting yuv data to rgb data \n");
    ConvDib(width, height, srcbuf, rgbbuf);

    return rgbbuf;
}

unsigned int NxGetSize(unsigned int w,
		       unsigned int h,
		       bool format)
{
    unsigned int size = 0;
    if (format) {
        size = CAL_YUVSIZE(w,h);
    } else {
        size = CAL_RGBSIZE(w,h);
    }
    return size;
}
