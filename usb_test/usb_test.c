#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

/* Control Settup Transfer */
/* cyusb_control_transfer( handle, bmRequestType, bmRequest, wValue, device address, data, data size, timeout) */
/* bmRequest Type */
/* [7] data direction host->device : 0 , device->host : 1 */
/* [5,6] vendor request : 2 */
/* [0,4] receptioninst : device - 0 */
/* bmRequest */
/* 0xD0 - getVendorID, device init */
/* 0xB0 - I2C ID SET */
/* 0xB2 - I2C Read */
/* 0xB1 - I2C Write */
/* 0xB3 - Capture Enable */
/* 0xc4 - I2C Write 16A16D */
/* 0xc5 - I2C Read 16A16D */
#define USB_CMD_DEVICE_INIT	0xD0
#define USB_CMD_MCU_HOLD	0xD1
#define USB_CMD_SET_SENSOR_ID	0xB0
#define USB_CMD_I2C_READ	0xB2
#define USB_CMD_I2C_WRITE	0xB1
#define USB_CMD_CAPTURE	0xB3
#define USB_CMD_I2C_WRITE_16	0xc4
#define USB_CMD_I2C_READ_16	0xc5

#define BTREE_SENSOR_ID 0x82

#define BTREE_USB_RET_SIZE 8

#define INT_MAX		((int)(~0U>>1))

struct usb_btree_io {
	unsigned int address;
	unsigned int data;
	int	result;
	unsigned char *buf;
};

typedef	struct	_BMPTYPEHEADER {		// for byte aligned read
	unsigned char	btTYPE[2];					// "BM"
} BMPTYPEHEADER;

typedef	struct	_BMPFILEHEADER {
	unsigned int	bfSize;						// file size
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned int	bfOffBits;					// offset value to image data
} BMPFILEHEADER;

typedef	struct	_BMPINFOHEADER {
	unsigned int	biSize;						// size of image header structure
	unsigned int	biWidth;					// image width
	unsigned int	biHeight;					// image height
	unsigned short	biPlane;					// always '1' (bit plane)
	unsigned short	biBitCount;					// bit per pixel
	unsigned int	biCompression;				// compression type
	unsigned int	biSizeImage;				// byte size of image
	unsigned int	biXPelsPerMeter;			// horizontal resolution
	unsigned int	biYPelsPerMeter;			// vertical resolution
	unsigned int	biClrUsed;					// the number of used color
	unsigned int	biClrImportant;				// important color index (0:all)
} BMPINFOHEADER;

/* functions related to image converting */
int		gnCoefY2R[9][256];		// coeff for yuv-to-rgb conversion
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

static int usb_ioctl(int usb_fd, unsigned long request, void *arg)
{
	int ret;
	printf("[%s] cmd = 0x%x \n", __func__, request);
	do {
		ret = ioctl(usb_fd, request, arg);
	} while (ret == -1 && (errno == EINTR || errno == EAGAIN));
	return ret;
}

static int usb_get_deviceID(int usb_fd, void *buf)
{
	int ret = -1;
	struct usb_btree_io io_data;

	io_data.address = 0;
	io_data.data = 0;
	io_data.buf = (unsigned char*)buf;
	ret = usb_ioctl(usb_fd, USB_CMD_DEVICE_INIT, &io_data);
	if (ret < 0)
		printf("Set Btree Device Init Fail:%d \n",ret);

	return ret;
}

static int usb_set_sensorID(int usb_fd)
{
	int ret = -1;
	struct usb_btree_io io_data;

	io_data.address = 0;
	io_data.data = BTREE_SENSOR_ID;
	io_data.buf = NULL;
	ret = usb_ioctl(usb_fd, USB_CMD_SET_SENSOR_ID, &io_data);
	if (ret < 0) {
		printf("Set Btree Sensor ID Fail:%d \n",ret);
		return ret;
	}
	ret = io_data.data;
	return ret;
}

static int usb_set_mcuhold(int usb_fd, int mode)
{
	int ret = -1;
	struct usb_btree_io io_data;

	io_data.address = 0;
	io_data.data = mode;
	io_data.buf = NULL;
	ret = usb_ioctl(usb_fd, USB_CMD_MCU_HOLD, &io_data);
	if (ret < 0) {
		printf("Set Btree Sensor ID Fail:%d \n",ret);
		return ret;
	}
	ret = io_data.result;
}

static int usb_i2c_write
	(int usb_fd, unsigned long address, unsigned long idata, struct usb_btree_io *io_data)
{
	unsigned int addr_h, addr_l;
	unsigned int data;
	int ret = -1;

	addr_h = ((address >> 7) & 0x01FE);
	addr_l = ((address << 1) & 0x01FE) | 0x0200;
	data = idata;
	printf("addr = 0x%4x, addr_l = 0x%2x, addr_h = 0x%2x, data = 0x%8x \n",
			address, addr_l, addr_h, data);
	
	if(!usb_set_mcuhold(usb_fd,1))
		goto done;
	
	//if (usb_set_sensorID(usb_fd) < 0)
	//	goto done;
	
	memset(io_data, 0x0, sizeof(struct usb_btree_io));
	io_data->address = addr_h;
	io_data->data = ((data>>16)&0xFFFF);
	ret = usb_ioctl(usb_fd, USB_CMD_I2C_WRITE_16, io_data);
	if (ret < 0) {
		printf("Read Register Fail:%d, address : 0x%2x, data = 0x%4x \n"
				,ret, io_data->address, io_data->data);
		goto done;
	}
	printf("Register address = 0x%2x, data = 0x%4x, result = %d \n",
			io_data->address, io_data->data, io_data->result);
	
	io_data->address = addr_l;
	io_data->data = (data&0xFFFF);
	ret = usb_ioctl(usb_fd, USB_CMD_I2C_WRITE_16, io_data);
	if (ret < 0) {
		printf("Read Register Fail:%d, address : 0x%2x, data = 0x%4x \n"
				,ret, io_data->address, io_data->data);
		goto done;
	}
	printf("Register address = 0x%2x, data = 0x%4x, result = %d \n",
			io_data->address, io_data->data, io_data->result);

	ret = io_data->result;
	
	if(!usb_set_mcuhold(usb_fd,0))
		ret = -1;
done:
	return ret;
}

static int usb_i2c_read
	(int usb_fd, unsigned int address, struct usb_btree_io *io_data)
{
	int addr_h, addr_l;
	int ret = -1;
	unsigned int	data_h, data_l = 0;

	addr_h = ((address >> 7) & 0x01FE);
	addr_l = ((address << 1) & 0x01FE) | 0x0200;
	printf("addr = 0x%4x, addr_l = 0x%2x, addr_h = 0x%2x\n",
			address, addr_l, addr_h);

	if (!usb_set_mcuhold(usb_fd,1))
		goto done;

	//if (usb_set_sensorID(usb_fd) < 0)
	//	goto done;

	printf("Register Read : High address \n");
	memset(io_data, 0x0, sizeof(struct usb_btree_io));
	io_data->address = addr_h;
	io_data->data = 0;
	ret = usb_ioctl(usb_fd, USB_CMD_I2C_READ_16, io_data);
	if (ret<0)
		goto done;
 	data_h = io_data->data;
	printf("Register address = 0x%2x, data = 0x%4x, result = %d \n",
			io_data->address, data_h, io_data->result);

	printf("Register Read : low address \n");
	memset(io_data, 0x0, sizeof(struct usb_btree_io));
	io_data->address = addr_l;
	io_data->data = 0;
	ret = usb_ioctl(usb_fd, USB_CMD_I2C_READ_16, io_data);
	if (ret<0)
		goto done;
	data_h =io_data->data;
	printf("Register address = 0x%2x, data = 0x%4x, result = %d \n",
			io_data->address, data_h, io_data->result);
	
	printf("Register Read : low address \n");
	memset(io_data, 0x0, sizeof(struct usb_btree_io));
	io_data->address = addr_l;
	io_data->data = 0;
	ret = usb_ioctl(usb_fd, USB_CMD_I2C_READ_16, io_data);
	if (ret<0)
		goto done;
	data_l = io_data->data;
	printf("Register address = 0x%2x, data = 0x%4x, result = %d \n",
			io_data->address, data_l, io_data->result);

	ret = io_data->result;
	io_data->data = ((data_h << 16) | data_l);
	
	if (!usb_set_mcuhold(usb_fd,0))
		ret = -1;	
done:
	return ret;
}

int open_usb_device(char *dev)
{
	int fd;
	
	printf("dev path is %s \n", dev);
	fd = open(dev, O_RDWR);
	if ( fd < 0)
		perror("open fail \n");
	return fd;
}
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

	m_bf.bfSize = CAL_RGBSIZE(m_hsize, m_vsize); //(((m_hsize*3) + m_dummy) * m_vsize)
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
	m_bi.biSizeImage = CAL_RGBSIZE(m_hsize, m_vsize); //((m_hsize*3) + m_dummy) * m_vsize;
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
			// get yuv data
			y1 = pSrc[1];	cb = pSrc[0];
			y2 = pSrc[3];	cr = pSrc[2];

			// convert rgb
			b = Y2R_B( y1, cb );		pDst[0] = CHK_RANGE( b );
			g = Y2R_G( y1, cb, cr );	pDst[1] = CHK_RANGE( g );
			r = Y2R_R( y1, cr );		pDst[2] = CHK_RANGE( r );
			b = Y2R_B( y2, cb );		pDst[3] = CHK_RANGE( b );
			g = Y2R_G( y2, cb, cr );	pDst[4] = CHK_RANGE( g );
			r = Y2R_R( y2, cr );		pDst[5] = CHK_RANGE( r );

			// update buffer pointer
			pSrc += 4;
			pDst += 6;
		}
		//printf("[%d] pSrc=%d, pDst=%d\n",y, pSrc[0], pDst[0]);
		pDst += nDummy;
	}
	printf("converting is finished\n");
}

static void initConvMatrix(void)
{
	int i;

	// initialize color conversion matrix (YCbCr-to-RGB)
	// R = { 1.000f x 1024,	 0.000f x 1024,	 1.371f x 1024	}	{ Y  }
	// G = { 1.000f x 1024,	-0.336f x 1024,	-0.698f x 1024	}	{ Cb }
	// B = { 1.000f x 1024,	 1.732f x 1024,	 0.000f x 1024	}	{ Cr }
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


int main(int argc, char **argv)
{
	int ret = 0;
	int usb_fd, fd = -1;
	unsigned char buf[BTREE_USB_RET_SIZE];
	unsigned char *dbuf, *rgbbuf; 
	struct usb_btree_io io_data;
	int addr, width, height = 0;
	int i, j, count, pSize, yuv_size, rgb_size, rgb_header = 0;

	printf("Btree USB TEST Application \n");
	printf("Open Btree USB Device : %s \n", argv[1]);
	usb_fd = open_usb_device(argv[1]);
	if (usb_fd < 0) {
		printf("Failed to open btree usb device \n");
		ret = usb_fd;
		goto error;
	}
	printf("Btree USB Device is ready for a test \n");
	printf("Set Btree Device Init and getVendorString \n");
	memset(buf, 0x0, BTREE_USB_RET_SIZE);
	ret = usb_get_deviceID(usb_fd, buf);
	if (ret < 0) {
		printf("Set Btree Device Init Fail:%d \n",ret);
		goto error;
	}
	printf("Vendor String = %s \n", buf);

	printf("Set Btree Sensor ID : 0x%x \n", BTREE_SENSOR_ID);
	ret = usb_set_sensorID(usb_fd);
	if (ret < 0) {
		printf("Set Btree Sensor ID Fail:%d \n",ret);
		goto error;
	}
	printf("Sensor ID = 0x%x \n", ret);
	// register read/write
	addr = 0x0000;
	printf("Register Read \n");
	ret = usb_i2c_read(usb_fd, addr, &io_data);
	if (ret < 0) {
		printf("Read Register Fail:%d, address : 0x%8x \n",ret, addr);
	}
	printf("Register address = 0x%8x, data = 0x%8x, result = %d \n", 
			addr, io_data.data, io_data.result);
	width = io_data.data >> 16;
	height = io_data.data & 0x0000FFFF;
	printf("width = %d, height = %d \n", width, height);

	printf("Set Capture Enable\n");
	memset(buf, 0x0, BTREE_USB_RET_SIZE);
	io_data.address = 0;
	io_data.data = 1;
	io_data.buf = buf;
	ret = usb_ioctl(usb_fd, USB_CMD_CAPTURE, &io_data);
	if (ret < 0) {
		printf("fail to start capture :%d \n",ret);
		goto error;
	}
	printf("ret = %d \n", io_data.result);

	printf("create a file to copy the image data \n");
	fd = creat("./resultImage.bmp", 0644);
	if (fd < 0) {
		printf("fail to create a file \n");
		goto error;
	}
	initConvMatrix();

	i = 0;
	// it should be chanded to get the max packet size from class driver
	pSize = 1024;
	yuv_size = CAL_YUVSIZE(width, height);
	count = yuv_size/pSize;
	dbuf = (unsigned char*)malloc(yuv_size);
	if (!dbuf) {
		printf("fail to malloc for yuv data \n");
		goto error;
	}
	printf("dbuf = 0x%x, size = %d \n",dbuf, yuv_size);

	rgb_header = sizeof(BMPINFOHEADER) + sizeof(BMPTYPEHEADER) + sizeof(BMPFILEHEADER);
	rgb_size = CAL_RGBSIZE(width, height);
	rgbbuf = (unsigned char*)malloc(rgb_size);
	if (!rgbbuf) {
		printf("fail to malloc for rgb data \n");
		goto error;
	}
	printf("rgbbuf = 0x%x , header size = %d, data size = %d\n", rgbbuf, rgb_header, rgb_size);
	
#if 0
	printf(" read frame from the camera : i = %d, count = %d, max packet size = %d \n", i, count, pSize);
	while(i<count)
	{
		ret = read(usb_fd, dbuf+(i*pSize), pSize);
		if (ret < 0) {
				printf("fail to read image data \n");
				goto error;
		}
		i++;
	}
#else
	if(yuv_size < INT_MAX) {
		ret = read(usb_fd, dbuf, yuv_size);
		if (ret < 0) {
				printf("fail to read image data \n");
				goto error;
		}
	} else {
		printf(" the buf size is too big to be read at once ");
		printf(" buf size is %d, max buf size is %d \n",
				yuv_size, INT_MAX);
	}
#endif
	// input header information for RGB
	printf("input header information for RGB\n");
	MakeRGBHeader(fd, width, height);
	printf("converting yuv data to rgb data \n");
	ConvDib(width, height, dbuf, rgbbuf);
	ret = write(fd,rgbbuf, rgb_size);
	if (ret < 0)
		printf("fail to write the data into the file \n");
	printf("finish copying \n");

error:
	if(dbuf)
		free(dbuf);
	if(rgbbuf)
		free(rgbbuf);
	if(fd)
		close(fd);
	if(usb_fd)
		close(usb_fd);
	return ret;
}
