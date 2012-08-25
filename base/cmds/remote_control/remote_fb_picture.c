/** @file remote_fb_picture.c
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work                             
 *  All Rights Reserved                                                                                                                              
 *  - The information contained herein is the confidential property            
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc. 
 *  @author   tellen
 *  @version  1.0        
 *  @date     2011/06/07
 *  @par function description:
 *  - 1 remote control server, save framebuffer data to picture
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#define LOG_TAG	"remote_control"
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <errno.h>

#include "remote_control.h"
#include "remote_fb_picture.h"

static void rc_dump_fb_var_screeninfo(const struct fb_var_screeninfo* vinfo)
{
    LOGI("fb, dump: bpp = %d \n", vinfo->bits_per_pixel);
	LOGI("fb, dump: xoffset = %d \n", vinfo->xoffset);
	LOGI("fb, dump: yoffset = %d \n", vinfo->yoffset);
	LOGI("fb, dump: width = %d \n", vinfo->xres);
	LOGI("fb, dump: height = %d \n", vinfo->yres);
	LOGI("fb, dump: vwidth = %d \n", vinfo->xres_virtual);
	LOGI("fb, dump: vheight = %d \n", vinfo->yres_virtual);
	LOGI("fb, dump: r_offset = %d \n", vinfo->red.offset);
	LOGI("fb, dump: g_offset = %d \n", vinfo->green.offset);
	LOGI("fb, dump: b_offset = %d \n", vinfo->blue.offset);
	LOGI("fb, dump: a_offset = %d \n", vinfo->transp.offset);
	LOGI("fb, dump: r_length = %d \n", vinfo->red.length);
	LOGI("fb, dump: g_length = %d \n", vinfo->green.length);
	LOGI("fb, dump: b_length = %d \n", vinfo->blue.length);
	LOGI("fb, dump: a_length = %d \n", vinfo->transp.length);
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_framebuffer_info
* DESCRIPTION:
*    			get framebuffer info
* ARGUMENTS:
*				const char* path:device path
*				rc_fb_t *fb:framebuffer struct
* Return:
*    			-1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int rc_framebuffer_info(const char* path, rc_fb_t *fb)
{
    int fd;
    struct fb_var_screeninfo vinfo;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
		LOGE("fb, open failed, %s\n", strerror(errno));
		return -1;
	}

    if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        LOGE("fb, ioctl failed, %s\n", strerror(errno));
		close(fd);
        return -1;
    }

	rc_dump_fb_var_screeninfo(&vinfo);
	
    fb->width = vinfo.xres;
    fb->height = vinfo.yres;

    close(fd);
    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_map_framebuffer
* DESCRIPTION:
*    			map framebuffer
* ARGUMENTS:
*				const char* path:device path
*				rc_fb_t *fb:framebuffer struct
* Return:
*    			-1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int rc_map_framebuffer(const char* path, rc_fb_t *fb)
{
    int fd;
    int bytespp;
    int offset;
    struct fb_var_screeninfo vinfo;

    fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    if(ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        LOGE("fb, ioctl failed, %s\n", strerror(errno));
		close(fd);
        return -1;
    }

	//rc_dump_fb_var_screeninfo(&vinfo);
		
    bytespp = vinfo.bits_per_pixel / 8;
    fb->width = vinfo.xres;
    fb->height = vinfo.yres;

    /* TODO: determine image format according to fb_bitfield of 
     *       red/green/blue */
    if (bytespp == 2) {
        fb->format = FB_FORMAT_RGB565;
    } else {
        fb->format = FB_FORMAT_ARGB8888;
    }

	LOGI("fb, fb width = %d, height = %d, format = %d\n", fb->width, fb->height, fb->format);
	
    /* HACK: for several of 3d cores a specific alignment
     * is required so the start of the fb may not be an integer number of lines
     * from the base.  As a result we are storing the additional offset in
     * xoffset. This is not the correct usage for xoffset, it should be added
     * to each line, not just once at the beginning */

    offset = vinfo.xoffset * bytespp;

    /* Android use double-buffer, capture 2nd */
    offset += vinfo.xres * vinfo.yoffset * bytespp * 2;
	
    //fb->data = mmap(0, vinfo.xres * vinfo.yres * bytespp, PROT_READ, MAP_SHARED, fd, offset);
	fb->data = mmap(0, vinfo.xres * vinfo.yres * bytespp, PROT_READ, MAP_SHARED, fd, 0);
    if (fb->data == MAP_FAILED) {
		LOGE("fb, mmap failed, %s\n", strerror(errno));
		close(fd);
		return -1;
	}

    close(fd);
    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_rgb565_to_rgb888
* DESCRIPTION:
*    			RGB565 to RGB888
* ARGUMENTS:
*				const char* src:src data
*				char* dst;dest data
*				size_t pixel:data length
* Return:
*    			-1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int rc_rgb565_to_rgb888(const char* src, char* dst, size_t pixel)
{
    struct rgb565  *from;
    struct rgb888  *to;

    from = (struct rgb565 *) src;
    to = (struct rgb888 *) dst;

    int i = 0;
    /* traverse pixel of the row */
    while(i++ < (int)pixel) {
        to->r = from->r;
        to->g = from->g;
        to->b = from->b;
        /* scale */
        to->r <<= 3;
        to->g <<= 2;
        to->b <<= 3;

        to++;
        from++;
    }

    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_argb8888_to_rgb888
* DESCRIPTION:
*    			RGB8888 to RGB888
* ARGUMENTS:
*				const char* src:src data
*				char* dst;dest data
*				size_t pixel:data length
* Return:
*    			-1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int rc_argb8888_to_rgb888(const char* src, char* dst, size_t pixel)
{
    int i, j;

	//bitmap888 need BGR
	//RGBA -> BGR
    for (i = 0; i < (int)pixel; i++) {
        for (j = 0; j < 3; j++) {
            //dst[3*i+j] = src[4*i+j];
			//dst[3*i+j] = (((src[4*i+j]*src[4*i+3])>>8)&0xff);
			//dst[3*i+j] = (((src[4*i+ 2 - j]*src[4*i+3])>>8)&0xff);
			dst[3*i+j] = src[4*i + 2 - j];
        }
    }

    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_argb8888_to_rgb565
* DESCRIPTION:
*    			RGB8888 to RGB565
* ARGUMENTS:
*				const char* src:src data
*				char* dst;dest data
*				size_t pixel:data length
* Return:
*    			-1:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int rc_argb8888_to_rgb565(const char* src, char* dst, size_t pixel)
{
    int i;

	//BGRA -> RGB
    for (i = 0; i < (int)pixel; i++) {
	#if 1
		int id1 = i<<2;
		int id2 = i<<1;

		int R = src[id1 + 0];
		int G = src[id1 + 1];
		int B = src[id1 + 2];
		
		dst[id2 + 1] = ((R&0xF8) | ((G>>5)&0x07));
		dst[id2] = (((G<<3)&0xE0) | ((B>>3)&0x1F));
	#else
		
		int R = src[4*i + 0];
		int G = src[4*i + 1];
		int B = src[4*i + 2];
		
		dst[2*i + 1] = ((R&0xF8) | ((G>>5)&0x07));
		dst[2*i] = (((G<<3)&0xE0) | ((B>>3)&0x1F));
	#endif
    }

    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_fb2bmp
* DESCRIPTION:
*    			get framebuffer data to bmp file
* ARGUMENTS:
*				void
* Return:
*    			<0:fail 0:success
* Note:
*
*---------------------------------------------------------------*/
int rc_fb2bmp(void)
{
    rc_fb_t fb;
    char *rgb_matrix;
    int i, ret;
	int len;

    ret = rc_map_framebuffer(FB_DEV_PATH, &fb);
    if(ret < 0) 
	{
        LOGE("fb, Cannnot open framebuffer\n");
        return -1;
    }

#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
	len = fb.width * fb.height * 3;
#else
	len = fb.width * fb.height * 2;
#endif
    rgb_matrix = (char *)malloc(len);
    if(!rgb_matrix)
	{
		LOGE("fb, malloc rgb_matrix memory error");
		return -2;
	}

    switch(fb.format) 
	{
	    case FB_FORMAT_RGB565:
	        /* emulator use rgb565 */
			LOGI("fb, format is RGB565\n");
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
			rc_rgb565_to_rgb888(fb.data, rgb_matrix, fb.width * fb.height);
#else
			memcpy(rgb_matrix, fb.data, len);
#endif
	        break;

	    case FB_FORMAT_ARGB8888:
	        /* most devices use argb8888 */
			LOGI("fb, format is ARGB8888\n");
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
	        rc_argb8888_to_rgb888(fb.data, rgb_matrix, fb.width * fb.height);
#else
			rc_argb8888_to_rgb565(fb.data, rgb_matrix, fb.width * fb.height);
#endif
	        break;

	    dafault:
	        LOGE("fb, treat framebuffer as rgb888\n");
    }

#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
	unsigned char *p_bmp_data = NULL;
	int file_header_len, info_header_len;
	RCBmpFileHeader_t bmp_file_header;
	RCBmpInfoHeader_t bmp_info_header;

	file_header_len = sizeof( RCBmpFileHeader_t );
	info_header_len = sizeof( RCBmpInfoHeader_t );

	memset( &bmp_file_header, 0, file_header_len );
	bmp_file_header.bf_type = 'MB';
	bmp_file_header.bf_size = file_header_len + len + info_header_len;
	bmp_file_header.bf_offbits = file_header_len + info_header_len;

	memset( &bmp_info_header, 0, info_header_len );
	bmp_info_header.bi_size = info_header_len;
	bmp_info_header.bi_width = fb.width;
	bmp_info_header.bi_height = -fb.height;
	bmp_info_header.bi_planes = 1;
	bmp_info_header.bi_bitcount = 24; //RGB 888

	p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);
	if( NULL == p_bmp_data )
	{
		LOGE("fb, [error]memory not enough:NULL == p_bmp_data\n");
		return -1;
	}
	memset(p_bmp_data, 0, bmp_file_header.bf_size);
	memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
	memcpy(p_bmp_data + file_header_len, &bmp_info_header, info_header_len);//copy bmp info header
	memcpy(p_bmp_data + file_header_len + info_header_len, rgb_matrix, len);//copy bmp data
	
#else
	unsigned char *p_bmp_data = NULL;
	int file_header_len, info_header_len, color_table_len;
	RCBmpFileHeader_t bmp_file_header;
	RCBmpInfoHeader_t bmp_info_header;
	RCBmpColorTable_t bmp_colors[3];
	
	bmp_colors[0].rgb_blue		=   0;
	bmp_colors[0].rgb_green		=   0xF8;
	bmp_colors[0].rgb_red		=   0;
	bmp_colors[0].rgb_reserved	=   0;
	bmp_colors[1].rgb_blue		=   0xE0;
	bmp_colors[1].rgb_green		=   0x07;
	bmp_colors[1].rgb_red		=   0;
	bmp_colors[1].rgb_reserved	=   0;
	bmp_colors[2].rgb_blue		=   0x1F;
	bmp_colors[2].rgb_green		=   0;
	bmp_colors[2].rgb_red		=   0;
	bmp_colors[2].rgb_reserved	=   0;

	file_header_len = sizeof( RCBmpFileHeader_t );
	info_header_len = sizeof( RCBmpInfoHeader_t );
	color_table_len = sizeof(bmp_colors);

	memset( &bmp_file_header, 0, file_header_len );
	bmp_file_header.bf_type = 'MB';
	bmp_file_header.bf_size = file_header_len + len + info_header_len + color_table_len; //RGB 565
	bmp_file_header.bf_offbits = file_header_len + info_header_len + color_table_len;

	memset( &bmp_info_header, 0, info_header_len );
	bmp_info_header.bi_size = info_header_len;
	bmp_info_header.bi_width = fb.width;
	bmp_info_header.bi_height = -fb.height;
	bmp_info_header.bi_planes = 1;
	bmp_info_header.bi_bitcount = 16; //RGB 565

	//use to RGB565
	bmp_info_header.bi_compression = 3;//BI_BITFIELDS;

	p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);
	if( NULL == p_bmp_data )
	{
		LOGE("fb, [error]memory not enough:NULL == p_bmp_data\n");
		return -1;
	}
	memset(p_bmp_data, 0, bmp_file_header.bf_size);
	memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
	memcpy(p_bmp_data + file_header_len, &bmp_info_header, info_header_len);//copy bmp info header
	memcpy(p_bmp_data + file_header_len + info_header_len, bmp_colors, color_table_len);//copy color table
	memcpy(p_bmp_data + file_header_len + info_header_len + color_table_len, rgb_matrix, len);//copy bmp data

#endif

	#if 1//for test
	{
		FILE *fp;
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
		fp = fopen("/sdcard/fb888.bmp", "wb");
#else
		fp = fopen("/sdcard/fb565.bmp", "wb");
#endif
		if( NULL == fp )
		{
			LOGE("fb, open file error\n");
			return -2;
		}

		fwrite(p_bmp_data, bmp_file_header.bf_size, 1, fp);
		fclose(fp);
		LOGI("fb, write file len = %d\n", (int)bmp_file_header.bf_size);
	}
	#endif

	if( NULL != p_bmp_data )
	{
		free(p_bmp_data);
		p_bmp_data = NULL;
	}
	
    free(rgb_matrix);
    return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_fb2buf
* DESCRIPTION:
*    			get framebuffer data to bufer
* ARGUMENTS:
*				void
* Return:		buffer address
* Note:
*
*---------------------------------------------------------------*/
char * rc_fb2buf(unsigned int* width, unsigned int* height)
{
    rc_fb_t fb;
    char *rgb_matrix;
    int i, ret;
	int len;

    ret = rc_map_framebuffer(FB_DEV_PATH, &fb);
    if(ret < 0) 
	{
        LOGE("fb, Cannnot open framebuffer\n");
        return NULL;
    }

	* width = fb.width;
	* height = fb.height;
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
	len = fb.width * fb.height * 3;
#else
	len = fb.width * fb.height * 2;
#endif
    rgb_matrix = (char *)malloc(len);
    if(!rgb_matrix)
	{
		LOGE("fb, malloc rgb_matrix memory error");
		return NULL;
	}

    switch(fb.format) 
	{
	    case FB_FORMAT_RGB565:
	        /* emulator use rgb565 */
			LOGI("fb, format is RGB565\n");
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
			rc_rgb565_to_rgb888(fb.data, rgb_matrix, fb.width * fb.height);
#else
			memcpy(rgb_matrix, fb.data, len);
#endif
	        break;

	    case FB_FORMAT_ARGB8888:
	        /* most devices use argb8888 */
			LOGI("fb, format is ARGB8888\n");
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
	        rc_argb8888_to_rgb888(fb.data, rgb_matrix, fb.width * fb.height);
#else
			rc_argb8888_to_rgb565(fb.data, rgb_matrix, fb.width * fb.height);
#endif
	        break;

	    dafault:
	        LOGE("fb, treat framebuffer as rgb888\n");
    }

    //free(rgb_matrix);
    return rgb_matrix;
}

