#ifndef NX_YUV_TO_RGB_H
#define NX_YUV_TO_RGB_H

int NxSaveBMP (unsigned int width, unsigned int height,
             unsigned char *srcbuf);

unsigned char* NxConvertRGB(unsigned int width,
                          unsigned int height,
                          unsigned char *srcbuf);

unsigned int NxGetSize(unsigned int w,
                     unsigned int h,
                     bool format);
#endif // NX_YUV_TO_RGB_H
