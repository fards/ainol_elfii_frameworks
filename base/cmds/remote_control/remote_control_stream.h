/** @file remote_control_stream.h
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
 *  - 1 remote control server, communication with client use TCP protocol
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */
 
#ifndef _REMOTE_CONTROL_STREAM_H_
#define _REMOTE_CONTROL_STREAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RC_STREAM_TYPE_DATA			0//send data to stream send thread
#define RC_STREAM_TYPE_EXIT			1//exit stream send thread
#define RC_STREAM_TYPE_SOCKETFD		2

void *rc_stream_recv(void *p);
void *rc_stream_send(void *p);
void rc_stream_send_exit(void);

void rc_stream_send_msg(osal_queue_t *osal_q, unsigned char *p_data, int data_len);
void rc_stream_recv_msg(void);
#ifdef __cplusplus
}
#endif

#endif/*_REMOTE_CONTROL_STREAM_H_*/

