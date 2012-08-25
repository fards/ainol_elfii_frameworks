/** @file remote_screen_cap.h
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
 
#ifndef _REMOTE_SCREEN_CAP_H_
#define _REMOTE_SCREEN_CAP_H_

#ifdef __cplusplus
extern "C" {
#endif

void remote_screen_cap_file(char *name);
unsigned char * remote_screen_cap_buf(unsigned long *buf_len, unsigned int pic_w, unsigned int pic_h);
unsigned char * remote_screen_jpeg(unsigned long *buf_len);
void remote_screen_info(unsigned int *width, unsigned int *height);
int64_t rc_system_time(void);

#ifdef __cplusplus
}
#endif

#endif/*_REMOTE_SCREEN_CAP_H_*/

