/** @file remote_screen_cap.cpp
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work                             
 *  All Rights Reserved                                                                                                                              
 *  - The information contained herein is the confidential property            
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc. 
 *  @author   tellen
 *  @version  1.0        
 *  @date     2011/11/16
 *  @par function description:
 *  - 1 remote control server, screen capture for android 2.3
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#define LOG_TAG	"remote_control"

#include <utils/Log.h>
#include <cutils/properties.h>
#include <string.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <binder/IMemory.h>
#include <surfaceflinger/ISurfaceComposer.h>

#include <SkImageEncoder.h>
#include <SkBitmap.h>
#include "SkCanvas.h"
#include "SkDevice.h"
#include "SkStream.h"

#include "remote_control.h"
#include "remote_screen_cap.h"
#include "remote_fb_picture.h"

using namespace android;

class RemoteStream : public SkWStream {
public:
    RemoteStream(){
        file_size = 0;
        fp = fopen("/sdcard/test.png", "wb");
		if( NULL == fp ){
			LOGE("RemoteStream, create file error");
		}
    }
    
	virtual bool write(const void* buffer, size_t size) {
        LOGI("RemoteStream, write file size:%d", size);
        
        //while (size > 0) {
            fwrite(buffer, size, 1, fp);
            file_size += size;
        //}
        return true;
    }
    
    virtual void flush() {
        fclose(fp);
        LOGI("RemoteStream, save file ok, file size:%d", file_size);
    }
    
private:
    FILE        *fp;
    size_t      file_size;
};

/*---------------------------------------------------------------
* FUNCTION NAME: rc_system_time
* DESCRIPTION:
*    			get system time
* ARGUMENTS:
*				void
* Return:
*    			int64_t
* Note:
*
*---------------------------------------------------------------*/
int64_t rc_system_time(void)
{
    return systemTime(SYSTEM_TIME_MONOTONIC);
}

/*---------------------------------------------------------------
* FUNCTION NAME: remote_screen_info
* DESCRIPTION:
*    			get screen info width and height
* ARGUMENTS:
*				unsigned int *width:
*               unsigned int *height:
* Return:
*    			void
* Note:
*
*---------------------------------------------------------------*/
void remote_screen_info(unsigned int *width, unsigned int *height)
{
    const String16 name("SurfaceFlinger");
    sp<ISurfaceComposer> composer;
    getService(name, &composer);

    sp<IMemoryHeap> heap;
    uint32_t w, h;
    PixelFormat f;
    status_t err = composer->captureScreen(0, &heap, &w, &h, &f, 0, 0,0,-1UL);
    if (err != NO_ERROR) {
        LOGE("get screen info ,screen capture failed: %s", strerror(-err));
        *width = 0;
        *height = 0;
    }

    LOGI("get screen info: w=%u, h=%u, format:%d", w, h, f);
    *width = w;
    *height = h;
}

/*---------------------------------------------------------------
* FUNCTION NAME: remote_screen_cap_file
* DESCRIPTION:
*               only capture graphic layer
*    			screen capture to a file
* ARGUMENTS:
*				char *fname:file name
* Return:
*    			void
* Note:
*
*---------------------------------------------------------------*/
void remote_screen_cap_file(char *fname)
{
    const String16 name("SurfaceFlinger");
    sp<ISurfaceComposer> composer;
    getService(name, &composer);

    sp<IMemoryHeap> heap;
    uint32_t w, h;
    PixelFormat f;
    status_t err = composer->captureScreen(0, &heap, &w, &h, &f, 0, 0,0,-1UL);
    if (err != NO_ERROR) {
        LOGE("screen capture to file failed: %s", strerror(-err));
        return;
    }

    if( NULL != fname )
    {
        SkBitmap b;
        b.setConfig(SkBitmap::kARGB_8888_Config, w, h);
        b.setPixels(heap->getBase());
        SkImageEncoder::EncodeFile(fname, b,
                SkImageEncoder::kPNG_Type, SkImageEncoder::kDefaultQuality);

        /* encode to stream
        SkWStream* strm = new RemoteStream();
        if (NULL != strm) {
            SkImageEncoder::EncodeStream(strm, b, SkImageEncoder::kPNG_Type, SkImageEncoder::kDefaultQuality);
            delete strm;
        }*/
    }
}

/*---------------------------------------------------------------
* FUNCTION NAME: remote_screen_jpeg
* DESCRIPTION:
*               only capture graphic layer
*    			screen capture data to jpeg
* ARGUMENTS:
*				unsigned long *buf_len
* Return:
*    			unsigned char *:buffer address
* Note:
*
*---------------------------------------------------------------*/
unsigned char * remote_screen_jpeg(unsigned long *buf_len)
{
    const String16 name("SurfaceFlinger");
    sp<ISurfaceComposer> composer;
    getService(name, &composer);

    sp<IMemoryHeap> heap;
    uint32_t w, h;
    PixelFormat f;
    status_t err = composer->captureScreen(0, &heap, &w, &h, &f, 0, 0,0,-1UL);
    if (err != NO_ERROR) {
        LOGE("screen capture to file failed: %s", strerror(-err));
        return NULL;
    }

    SkBitmap b;
    b.setConfig(SkBitmap::kARGB_8888_Config, w, h);
    b.setPixels(heap->getBase());

    SkDynamicMemoryWStream strm;
    nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
    SkImageEncoder::EncodeStream(&strm, b, SkImageEncoder::kJPEG_Type, SkImageEncoder::kDefaultQuality);
    *buf_len=strm.getOffset();
    unsigned char * p_data=(unsigned char*)malloc((*buf_len)*sizeof(char));
    if(p_data==NULL)
    {
        LOGE("[error]memory not enough:NULL == p_bmp_data");
        return NULL;
    }
    memset(p_data,0,(*buf_len));
    memcpy(p_data,strm.getStream(),(*buf_len));
    LOGI("get jpeg data use time = %d ms\n", (int)((systemTime(SYSTEM_TIME_MONOTONIC) - startTime)/(1000*1000)));        
    return p_data;
}

#if 1
/*---------------------------------------------------------------
* FUNCTION NAME: remote_screen_cap_buf
* DESCRIPTION:
*               only capture graphic layer
*    			screen capture to a buffer, after usering need free this pointer
* ARGUMENTS:
*				unsigned long *buf_len:buffer len
*               unsigned int pic_w:request picture width
*               unsigned int pic_h:request picture height
* Return:
*    			char *:unsigned char pointer
* Note:
*
*---------------------------------------------------------------*/
unsigned char * remote_screen_cap_buf(unsigned long *buf_len, unsigned int pic_w, unsigned int pic_h)
{
    const String16 name("SurfaceFlinger");
    sp<ISurfaceComposer> composer;
    getService(name, &composer);

    sp<IMemoryHeap> heap;
    uint32_t w, h;
    PixelFormat f;

    char *rgbMatrix = NULL;
    unsigned char *p_bmp_data = NULL;

#ifdef RC_DEBUG_TIME
    nsecs_t capTime = systemTime(SYSTEM_TIME_MONOTONIC);
#endif
    
    status_t err = composer->captureScreen(0, &heap, &w, &h, &f, 0, 0, 0,-1UL);
    if (err != NO_ERROR) 
    {
        LOGE("screen capture to buffer failed: %s", strerror(-err));
        return NULL;
    }

#ifdef RC_DEBUG_TIME
    LOGI("screen capture use time = %d ms\n", (int)((systemTime(SYSTEM_TIME_MONOTONIC) - capTime)/(1000*1000)));
#endif

    LOGI("screen capture: screen_w=%u, screen_h=%u, format:%d, pic_w:%d, pic_h:%d", w, h, f, pic_w, pic_h);

    if( (PIXEL_FORMAT_RGBA_8888 != f) || ( 0 == w ) || ( 0 == h ) )
    {
        LOGE("framebuffer data format is not RGBA8888");
		return NULL ;
    }
    
    if( NULL == buf_len )
    {
        LOGE("buffer length pointer is NULL");
        return NULL;
    }

#ifdef RC_DEBUG_TIME
    nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
#endif
    
    int degree = 0;
    char property[100];
    if (property_get("ro.sf.hwrotation", property, NULL) > 0) 
    {
        switch (atoi(property)) 
        {
            case 270:
                LOGI("hardware has rotation, need rotate 180 degree");
                degree = 180;
                break;

            case 90:
            default:
                break;
        }
    }
    
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
	int len = pic_w * pic_h * 3;
    rgbMatrix = (char *)malloc(len);
    if(!rgbMatrix) 
	{
		LOGE("malloc rgbMatrix memory error");
		return NULL ;
	}
    
    //scale and rotate the picture of capture screen
    {
        SkBitmap b;
        b.setConfig(SkBitmap::kARGB_8888_Config, w, h);
        b.setPixels(heap->getBase());
        
        SkPaint paint;
        paint.setFilterBitmap(true);
        
        SkBitmap ScaleBmp;
        ScaleBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
        bool ret = ScaleBmp.allocPixels(b.getColorTable());
        if( !ret )
        {
            LOGE("alloc scale bitmap [error]memory not enough");
            goto MEM_ERR;
        }
        SkMatrix matrixScale;
        matrixScale.setScale(pic_w/(float)w, pic_h/(float)h);
        SkCanvas canvasScale(new SkDevice(NULL,ScaleBmp,false));
        canvasScale.drawBitmapMatrix(b, matrixScale, &paint);

        if( 0 == degree )
        {
            rc_argb8888_to_rgb888((char *)ScaleBmp.getPixels(), rgbMatrix, pic_w * pic_h);
        }
        else
        {
            SkBitmap RotateBmp;
            RotateBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
            ret = RotateBmp.allocPixels(ScaleBmp.getColorTable());
            if( !ret )
            {
                LOGE("alloc rotate bitmap [error]memory not enough");
                goto MEM_ERR;
            }
            SkMatrix matrixRotate;
            matrixRotate.setRotate(degree, pic_w/2, pic_h/2);
            SkCanvas canvasRotate(new SkDevice(NULL,RotateBmp,false));
            canvasRotate.drawBitmapMatrix(ScaleBmp, matrixRotate, &paint);

            rc_argb8888_to_rgb888((char *)RotateBmp.getPixels(), rgbMatrix, pic_w * pic_h);
        }   
    }

    //RGB888 bmp file
	//unsigned char *p_bmp_data = NULL;
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
	bmp_info_header.bi_width = pic_w;
	bmp_info_header.bi_height = -pic_h;
	bmp_info_header.bi_planes = 1;
	bmp_info_header.bi_bitcount = 24; //RGB 888

	p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);
	if( NULL == p_bmp_data )
	{
		LOGE("[error]memory not enough:NULL == p_bmp_data");
		goto MEM_ERR;
	}
	memset(p_bmp_data, 0, bmp_file_header.bf_size);
	memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
	memcpy(p_bmp_data + file_header_len, &bmp_info_header, info_header_len);//copy bmp info header
	memcpy(p_bmp_data + file_header_len + info_header_len, rgbMatrix, len);//copy bmp data
#else
	int len = pic_w * pic_h * 2;
    rgbMatrix = (char *)malloc(len);
    if(!rgbMatrix) 
	{
		LOGE("malloc rgbMatrix memory error");
		return NULL ;
	}
    
    //scale and rotate the picture of capture screen
    {
        SkBitmap b;
        b.setConfig(SkBitmap::kARGB_8888_Config, w, h);
        b.setPixels(heap->getBase());
        
        SkPaint paint;
        paint.setFilterBitmap(true);
        
        SkBitmap ScaleBmp;
        ScaleBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
        bool ret = ScaleBmp.allocPixels(b.getColorTable());
        if( !ret )
        {
            LOGE("alloc scale bitmap [error]memory not enough");
            goto MEM_ERR;
        }
        SkMatrix matrixScale;
        matrixScale.setScale(pic_w/(float)w, pic_h/(float)h);
        SkCanvas canvasScale(new SkDevice(NULL,ScaleBmp,false));
        canvasScale.drawBitmapMatrix(b, matrixScale, &paint);

        if( 0 == degree )
        {
            rc_argb8888_to_rgb565((char *)ScaleBmp.getPixels(), rgbMatrix, pic_w * pic_h);
        }
        else
        {
            SkBitmap RotateBmp;
            RotateBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
            ret = RotateBmp.allocPixels(ScaleBmp.getColorTable());
            if( !ret )
            {
                LOGE("alloc rotate bitmap [error]memory not enough");
                goto MEM_ERR;
            }
            SkMatrix matrixRotate;
            matrixRotate.setRotate(degree, pic_w/2, pic_h/2);
            SkCanvas canvasRotate(new SkDevice(NULL,RotateBmp,false));
            canvasRotate.drawBitmapMatrix(ScaleBmp, matrixRotate, &paint);

            rc_argb8888_to_rgb565((char *)RotateBmp.getPixels(), rgbMatrix, pic_w * pic_h);
        }   
    }

	//unsigned char *p_bmp_data = NULL;
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
	bmp_info_header.bi_width = pic_w;
	bmp_info_header.bi_height = -pic_h;
	bmp_info_header.bi_planes = 1;
	bmp_info_header.bi_bitcount = 16; //RGB 565

	//use to RGB565
	bmp_info_header.bi_compression = 3;//BI_BITFIELDS;

	p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);
	if( NULL == p_bmp_data )
	{
		LOGE("[error]memory not enough:NULL == p_bmp_data");
		goto MEM_ERR;
	}
	memset(p_bmp_data, 0, bmp_file_header.bf_size);
	memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
	memcpy(p_bmp_data + file_header_len, &bmp_info_header, info_header_len);//copy bmp info header
	memcpy(p_bmp_data + file_header_len + info_header_len, bmp_colors, color_table_len);//copy color table
	memcpy(p_bmp_data + file_header_len + info_header_len + color_table_len, rgbMatrix, len);//copy bmp data
#endif
    
    *buf_len = bmp_file_header.bf_size;

#ifdef SCREEN_CAP_FILE_TEST
	{
		FILE *fp;

#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)
        fp = fopen("/sdcard/fb888.bmp", "wb");
#else
        fp = fopen("/sdcard/fb565.bmp", "wb");
#endif
		if( NULL == fp )
		{
			LOGE("open file error\n");
		}

		fwrite(p_bmp_data, bmp_file_header.bf_size, 1, fp);
		fclose(fp);
	}

	if( NULL != p_bmp_data )
	{
		free(p_bmp_data);
		p_bmp_data = NULL;
	}
#endif

#ifdef RC_DEBUG_TIME
    LOGI("picture data len = %d, use time = %d ms\n", (int)bmp_file_header.bf_size, (int)((systemTime(SYSTEM_TIME_MONOTONIC) - startTime)/(1000*1000)));
#endif

    free(rgbMatrix);
    return p_bmp_data;
    
#if 0
    unsigned char *com_buf= NULL;
    unsigned long com_len=0;					
    com_len = compressBound(*buf_len);
    com_buf = (unsigned char*)malloc(com_len);
    if(NULL != com_buf){
        nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
        int ret = compress(com_buf, &com_len,p_bmp_data,*buf_len);
        LOGI("Compress use time = %d ms\n", (int)((systemTime(SYSTEM_TIME_MONOTONIC) - startTime)/(1000*1000)));

        if(ret==Z_OK){				
            if(p_bmp_data!=NULL){
                free(p_bmp_data);	
                p_bmp_data = NULL; 
            }	

            *buf_len=com_len; 
            LOGI("remote_control compress length %ld",com_len);	
        }
    }
    return com_buf;
#endif

MEM_ERR:

    if( NULL != rgbMatrix)
    {
        free(rgbMatrix);
        rgbMatrix = NULL;
    }
    return NULL ;
}

#else

/*---------------------------------------------------------------
* FUNCTION NAME: remote_screen_cap_buf
* DESCRIPTION:
*               only capture graphic layer
*    			screen capture to a buffer, after usering need free this pointer
* ARGUMENTS:
*				unsigned long *buf_len:buffer len
*               unsigned int pic_w:request picture width
*               unsigned int pic_h:request picture height
* Return:
*    			char *:unsigned char pointer
* Note:
*
*---------------------------------------------------------------*/
unsigned char * remote_screen_cap_buf(unsigned long *buf_len, unsigned int pic_w, unsigned int pic_h)
{
    uint32_t w, h;
    char *p_data = NULL;
    char *rgbMatrix = NULL;
    unsigned char *p_bmp_data = NULL;

    if( NULL == buf_len )
    {
        LOGE("buffer length pointer is NULL");
        return NULL;
    }
    
#ifdef RC_DEBUG_TIME
    nsecs_t capTime = systemTime(SYSTEM_TIME_MONOTONIC);
#endif

    p_data = rc_fb2buf(&w, &h);
    if (NULL == p_data) 
    {
        LOGE("screen capture to buffer failed");
        return NULL;
    }

#ifdef RC_DEBUG_TIME
    LOGI("screen capture use time = %d ms\n", (int)((systemTime(SYSTEM_TIME_MONOTONIC) - capTime)/(1000*1000)));
#endif

    LOGI("screen capture: screen_w=%u, screen_h=%u, pic_w:%d, pic_h:%d", w, h, pic_w, pic_h);

    if( ( 0 == w ) || ( 0 == h ) )
    {
        return NULL;
    }

#ifdef RC_DEBUG_TIME
    nsecs_t startTime = systemTime(SYSTEM_TIME_MONOTONIC);
#endif

    int degree = 0;
    char property[100];
    if (property_get("ro.sf.hwrotation", property, NULL) > 0) 
    {
        switch (atoi(property)) 
        {
            case 270:
                LOGI("hardware has rotation, need rotate 180 degree");
                degree = 180;
                break;

            case 90:
            default:
                break;
        }
    }
    
#if (RC_BITMAP_RGB888 == RC_BITMAP_FORMAT)    
    int len = pic_w * pic_h * 3;
    rgbMatrix = (char *)malloc(len);
    if(!rgbMatrix) 
	{
		LOGE("malloc rgbMatrix memory error");
		goto MEM_ERR;
	}
    
    //scale and rotate the picture of capture screen
    {
        SkBitmap b;
        b.setConfig(SkBitmap::kARGB_8888_Config, w, h);
        b.setPixels(p_data);
        
        SkPaint paint;
        paint.setFilterBitmap(true);
        
        SkBitmap ScaleBmp;
        ScaleBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
        bool ret = ScaleBmp.allocPixels(b.getColorTable());
        if( !ret )
        {
            LOGE("alloc scale bitmap [error]memory not enough");
            goto MEM_ERR;
        }
        SkMatrix matrixScale;
        matrixScale.setScale(pic_w/(float)w, pic_h/(float)h);
        SkCanvas canvasScale(new SkDevice(NULL, ScaleBmp, false));
        canvasScale.drawBitmapMatrix(b, matrixScale, &paint);

        if( 0 == degree )
        {
            memcpy(rgbMatrix, (char *)ScaleBmp.getPixels(), len);
        }
        else
        {
            SkBitmap RotateBmp;
            RotateBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
            ret = RotateBmp.allocPixels(ScaleBmp.getColorTable());
            if( !ret )
            {
                LOGE("alloc rotate bitmap [error]memory not enough");
                goto MEM_ERR;
            }
            SkMatrix matrixRotate;
            matrixRotate.setRotate(degree, pic_w/2, pic_h/2);
            SkCanvas canvasRotate(new SkDevice(NULL,RotateBmp,false));
            canvasRotate.drawBitmapMatrix(ScaleBmp, matrixRotate, &paint);

            memcpy(rgbMatrix, (char *)RotateBmp.getPixels(), len);
        }   
    }

    //RGB888 bmp file
	//unsigned char *p_bmp_data = NULL;
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
	bmp_info_header.bi_width = pic_w;
	bmp_info_header.bi_height = -pic_h;
	bmp_info_header.bi_planes = 1;
	bmp_info_header.bi_bitcount = 24; //RGB 888

	p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);
	if( NULL == p_bmp_data )
	{
		LOGE("[error]memory not enough:NULL == p_bmp_data");
		goto MEM_ERR;
	}
	memset(p_bmp_data, 0, bmp_file_header.bf_size);
	memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
	memcpy(p_bmp_data + file_header_len, &bmp_info_header, info_header_len);//copy bmp info header
	memcpy(p_bmp_data + file_header_len + info_header_len, rgbMatrix, len);//copy bmp data
#else
	int len = pic_w * pic_h * 2;
    rgbMatrix = (char *)malloc(len);
    if(!rgbMatrix) 
	{
		LOGE("malloc rgbMatrix memory error");
		goto MEM_ERR;
	}
    
    //scale and rotate the picture of capture screen
    {
        SkBitmap b;
        b.setConfig(SkBitmap::kARGB_8888_Config, w, h);
        b.setPixels(p_data);
        
        SkPaint paint;
        paint.setFilterBitmap(true);
        
        SkBitmap ScaleBmp;
        ScaleBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
        bool ret = ScaleBmp.allocPixels(b.getColorTable());
        if( !ret )
        {
            LOGE("alloc scale bitmap [error]memory not enough");
            goto MEM_ERR;
        }
        SkMatrix matrixScale;
        matrixScale.setScale(pic_w/(float)w, pic_h/(float)h);
        SkCanvas canvasScale(new SkDevice(NULL, ScaleBmp, false));
        canvasScale.drawBitmapMatrix(b, matrixScale, &paint);

        if( 0 == degree )
        {
            memcpy(rgbMatrix, (char *)ScaleBmp.getPixels(), len);
        }
        else
        {
            SkBitmap RotateBmp;
            RotateBmp.setConfig(SkBitmap::kARGB_8888_Config, pic_w, pic_h);
            ret = RotateBmp.allocPixels(ScaleBmp.getColorTable());
            if( !ret )
            {
                LOGE("alloc rotate bitmap [error]memory not enough");
                goto MEM_ERR;
            }
            SkMatrix matrixRotate;
            matrixRotate.setRotate(degree, pic_w/2, pic_h/2);
            SkCanvas canvasRotate(new SkDevice(NULL, RotateBmp, false));
            canvasRotate.drawBitmapMatrix(ScaleBmp, matrixRotate, &paint);

            memcpy(rgbMatrix, (char *)RotateBmp.getPixels(), len);
        }   
    }

	//unsigned char *p_bmp_data = NULL;
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
	bmp_info_header.bi_width = pic_w;
	bmp_info_header.bi_height = -pic_h;
	bmp_info_header.bi_planes = 1;
	bmp_info_header.bi_bitcount = 16; //RGB 565

	//use to RGB565
	bmp_info_header.bi_compression = 3;//BI_BITFIELDS;

	p_bmp_data = (unsigned char *)malloc(bmp_file_header.bf_size);
	if( NULL == p_bmp_data )
	{
		LOGE("[error]memory not enough:NULL == p_bmp_data");
		goto MEM_ERR;
	}
	memset(p_bmp_data, 0, bmp_file_header.bf_size);
	memcpy(p_bmp_data, &bmp_file_header, file_header_len);//copy bmp file header
	memcpy(p_bmp_data + file_header_len, &bmp_info_header, info_header_len);//copy bmp info header
	memcpy(p_bmp_data + file_header_len + info_header_len, bmp_colors, color_table_len);//copy color table
	memcpy(p_bmp_data + file_header_len + info_header_len + color_table_len, rgbMatrix, len);//copy bmp data
#endif
    
    *buf_len = bmp_file_header.bf_size;

#ifdef RC_DEBUG_TIME
    LOGI("picture data len = %d, use time = %d ms\n", (int)bmp_file_header.bf_size, (int)((systemTime(SYSTEM_TIME_MONOTONIC) - startTime)/(1000*1000)));
#endif

    free(rgbMatrix);
    if( NULL != p_data )
    {
        free(p_data);
        p_data = NULL;
    }
    return p_bmp_data;
    
MEM_ERR:
    if( NULL != p_data )
    {
        free(p_data);
        p_data = NULL;
    }

    if( NULL != rgbMatrix)
    {
        free(rgbMatrix);
        rgbMatrix = NULL;
    }

    return NULL ;
}

#endif

