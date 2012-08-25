/** @file remote_control.h
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
 *  - 1 remote control server, use to receive 
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */
 
#ifndef _REMOTE_CONTROL_H_
#define _REMOTE_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <cutils/sockets.h>
#include <cutils/log.h>
#include <cutils/properties.h>

//#define RC_DEBUG 
//#define RC_DEBUG_TIME

#define RC_HEARTBEAT_TIME			30*60//heartbeat is 30 mins

#define RC_UDP_PORT					7001
#define RC_TCP_PORT					7002

#define RC_DATA_LEN					256

#define RC_CLIENT_SCAN				"amlogic-client-scan"
#define RC_CLIENT_REQ				"amlogic-client-request-connect"
#define RC_CLIENT_REQ_CONFIRM		"amlogic-client-request-connect-yes?"
#define RC_CLIENT_REQ_OK			"amlogic-client-request-ok"
#define RC_CLIENT_NO_CONNECT		"amlogic-client-no-connect"

#define RC_SERVER_LISTEN			"amlogic-server-listen"
#define RC_SERVER_IDLE				"amlogic-server-idle"
#define RC_SERVER_USED				"amlogic-server-used"

#define RC_BITMAP_RGB565			0
#define RC_BITMAP_RGB888			1
#define RC_BITMAP_FORMAT			RC_BITMAP_RGB565
//#define SCREEN_CAP_FILE_TEST		//screen capture to file

enum
{
	RC_FAIL = 0,
	RC_SUCCESS
};

typedef enum
{
	RC_THREAD_TCP_RECV = 0, //recv from client
	RC_THREAD_TCP_SEND, //send to client
	RC_THREAD_EVENT_RECV, //recv ui state change from framework
	RC_THREAD_EVENT_SEND,//send event to framwork
	RC_THREAD_SENSOR_LISTEN,//listen connect or close from framework
	RC_THREAD_SENSOR_SEND, //send sensor data to framwork
	RC_THREAD_PERCEPTION_SENSOR, //get perception sensor data from client
	RC_THREAD_SERVICE_SEND, //send servie data to framwork
	RC_THREAD_SERVICE_RECV //recv control info from framework
}rc_thread_type_t;

typedef enum
{
	RC_EVENT_TYPE_ACK = 0,	//client<-->server
	RC_EVENT_TYPE_KEY,		//client-->server     
	RC_EVENT_TYPE_TOUCH,	//client-->server 
	RC_EVENT_TYPE_TRACKBALL,	//client-->server 
	RC_EVENT_TYPE_SENSOR,		//client-->server 
	RC_EVENT_TYPE_UI_STATE,		//5. client<--server 
	RC_EVENT_TYPE_GET_PICTURE, //client-->server 	client get server framebuffer
	RC_EVENT_TYPE_SWITCH_MODE, //client-->server	touch or mouse mode
	RC_EVENT_TYPE_SERVICE		  //8.navite socket 	framework <-->server
}rc_event_type_t;

int rc_server_pthread_create(int thread_type, void *(*entry)(void *), int fd);
int rc_server_pthread_kill(pthread_t thread_id);
int rc_server_pthread_exit(pthread_t thread_id);
int rc_server_read(int fd, void *_buf, int count);
int rc_server_write(int socketfd, unsigned char *p_buf, int len);
int rc_server_byte_2_int(unsigned char *p);
void rc_server_tcp_enable_set(int enable);
int rc_server_tcp_enable_get(void);
void rc_server_tcp_client_set(int enable);
int rc_server_tcp_client_get(void);

#ifdef __cplusplus
}
#endif

#endif/*_REMOTE_CONTROL_H_*/

