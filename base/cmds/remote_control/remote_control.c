/** @file remote_control.c
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

#define LOG_TAG	"remote_control"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <pthread.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include "semaphore.h"
#include <assert.h>
#include <errno.h>
#include <sys/resource.h>

#include "osal_task_comm.h"
#include "remote_control.h"
#include "remote_control_stream.h"
#include "remote_control_local.h"
#include "remote_fb_picture.h"
#include "remote_screen_cap.h"
#include "remote_sensor_hub.h"

static pthread_mutex_t tcp_mutex;
static pthread_mutex_t client_connect_mutex;
static pthread_mutex_t pthread_mutex;
static sem_t g_pthread_sem;

static int g_tcp_client_connect = 0;
static int g_tcp_recv_connect = 1;
static int g_connect_confirm = 0;
static pthread_t g_tcp_recv_pthread_id;
pthread_t g_tcp_send_pthread_id;

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_pthread_create
* DESCRIPTION:
*    			create a new pthread 
* ARGUMENTS:
*				int thread_type:tcp thread or local thread
*				void *(*entry)(void *):function pointer
*				int fd:socket fd
* Return:
*    			0:create fail 1:success
* Note:
*    
*---------------------------------------------------------------*/
int rc_server_pthread_create(int thread_type, void *(*entry)(void *), int fd)
{
	int ret;
	pthread_t thread_id;
	int data[2];

	data[0] = fd;
	data[1] = (int)(&g_pthread_sem);

	osal_mutex_lock(&pthread_mutex);
	ret = pthread_create(&thread_id, NULL, entry, (void *)data);
	if( ret != 0 ) LOGE("server, thread create failed\n");
	
	ret = sem_wait(&g_pthread_sem);
	if( ret < 0 ) LOGE("server, sem_wait failed\n");

	if( RC_THREAD_TCP_RECV == thread_type )
	{
		g_tcp_recv_pthread_id = thread_id;
		LOGI("server, create TCP recv thread id = %lu\n", thread_id);
	}
	else if( RC_THREAD_TCP_SEND == thread_type )
	{
		g_tcp_send_pthread_id = thread_id;
		LOGI("server, create TCP send thread id = %lu\n", thread_id);
	}
	
	osal_mutex_unlock(&pthread_mutex);

	//LOGI("server, create the thread id = %lu\n", thread_id);
	return 1;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_pthread_kill
* DESCRIPTION:
*    			kill a pthread, send a signal to specified thread
* ARGUMENTS:
*				pthread_t thread_id:
* Return:
*    			0:fail 1:success
* Note:
*    
*---------------------------------------------------------------*/
int rc_server_pthread_kill(pthread_t thread_id)
{
	void *thread_result;
	int ret = 1;

	//osal_mutex_lock(&pthread_mutex);
	
	int kill_rc = pthread_kill(thread_id, 0);
	if( ESRCH == kill_rc )
	{
		LOGI("server, the specified thread did not exists or already quit\n");
		ret = 0;
		goto EXIT;
	}
	else if( EINVAL == kill_rc )
	{
		LOGI("server, signal is invalid\n");
		ret = 0;
		goto EXIT;
	}
	else
	{
		LOGI("server, kill the thread:%lu \n", thread_id);
		pthread_kill(thread_id, SIGQUIT);
	}

	sleep(1);
	pthread_join(thread_id, &thread_result);

	//osal_mutex_unlock(&pthread_mutex);
	LOGI("server, pthread_kill id = %lu, %s\n", thread_id, (char *)thread_result);
	return ret;
	
EXIT:
	//osal_mutex_unlock(&pthread_mutex);
	return ret;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_pthread_exit
* DESCRIPTION:
*    			exit a pthread
* ARGUMENTS:
*				pthread_t thread_id:
* Return:
*    			0:fail 1:success
* Note:
*    
*---------------------------------------------------------------*/
int rc_server_pthread_exit(pthread_t thread_id)
{
	void *thread_result;
	int ret = 1;

	if( 0 != thread_id )
	{
		osal_mutex_lock(&pthread_mutex);
		if( 0 != pthread_join(thread_id, &thread_result) )
		{
			LOGE("server, thread exit failed\n");
			ret = 0;
		}
		osal_mutex_unlock(&pthread_mutex);
		LOGI("server, pthread_exit id = %lu, %s\n", thread_id, (char *)thread_result);
	}
	
	return ret;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_read
* DESCRIPTION:
*    			remote server read data from fd
* ARGUMENTS:
*				int fd:
*				unsigned char *p_buf:data buffer
*				int len:send data length
* Return:
*				-1:read fail
*				0:end
*				1:success
* Note:
*
*---------------------------------------------------------------*/
int rc_server_read(int fd, void *_buf, int count)
{
    char *buf = (char *)_buf;
    int n = 0, r;
    if ((fd < 0) || (count < 0)) return -1;
    while (n < count) 
	{
        r = read(fd, buf + n, count - n);
        if (r < 0) 
		{
            if (errno == EINTR) continue;
            LOGE("server, [error]read error: %s\n", strerror(errno));
            return -1;
        }
		
        if (r == 0) 
		{
			LOGI("server, read eof\n");
            return 0;
        }
        n += r;
    }
    return n;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_write
* DESCRIPTION:
*    			remote server send ack to client
* ARGUMENTS:
*				int socketfd:client socketfd
*				unsigned char *p_buf:send data buffer
*				int len:send data length
* Return:
*    			write length
*				-1:write fail
*				0:client disconnect 
*				1:success
* Note:
*
*---------------------------------------------------------------*/
int rc_server_write(int socketfd, unsigned char *p_buf, int len)
{
	int send_len;
	unsigned char *p_send_buf = NULL;
	int bytes_left = 0;
	int written_bytes = 0;
	int ret = 1;
	unsigned char *ptr;

#ifdef RC_DEBUG_TIME
	int64_t startTime;
#endif
	
	send_len = sizeof(len) + len;
	p_send_buf = (unsigned char *)malloc(send_len);
	if( NULL == p_send_buf )
	{
		LOGI("server, [error]write memory not enough\n");
		ret = -1;
		goto EXIT;
	}
	memset(p_send_buf , 0, send_len);
	
	memcpy(p_send_buf, &len, sizeof(len));
	if( NULL != p_buf )
	{
		memcpy(&p_send_buf[sizeof(len)], p_buf, len);
	}
	
	//begin to write
	bytes_left = send_len;
	ptr = p_send_buf;
	
#ifdef RC_DEBUG_TIME
	startTime = rc_system_time();
#endif

	while( bytes_left > 0 )
	{		
         written_bytes = write(socketfd, ptr, bytes_left);
		 if( written_bytes == 0 )//disconnect
		 {
		 	LOGE("server, write return 0, client have disconnected\n");
			ret = 0;
			goto EXIT;
		 }
         else if( written_bytes < 0 )
         {
             if( EINTR == errno )//interrupt error, continue to write
             {
             	LOGE("server, write interrupt, continue to write\n");
             	written_bytes = 0;
             }
             else//other error exit
             {
             	LOGE("server, write error other error(%s)\n", strerror(errno));				
				ret = -1;
             	goto EXIT;
             }
         }
         bytes_left -= written_bytes;
         ptr += written_bytes;
	}

#ifdef RC_DEBUG_TIME
	LOGI("server, send length is:%d byte, use time = %d ms\n", send_len, (int)((rc_system_time() - startTime)/(1000*1000)));
#endif

EXIT:
	if( NULL != p_send_buf )
	{
		free(p_send_buf);
		p_send_buf = NULL;
	}
	return ret;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_byte_2_int
* DESCRIPTION:
*    			change byte stream to int data
* ARGUMENTS:
*				unsigned char *p:byte stream
* Return:
*    			int data
* Note:
*
*---------------------------------------------------------------*/
int rc_server_byte_2_int(unsigned char *p)
{
	int i, sum = 0;
	unsigned char loop;

	if( NULL != p )
	{
		for(i = 0; i < 4; i++)
		{
			loop = p[i];
			sum += (loop&0xff)<<(8*(4 - i - 1)); 
		}
	}

	return sum;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_tcp_enable_set
* DESCRIPTION:
*    			set tcp connect 
* ARGUMENTS:
*				bool enable:true enable tcp connect, false:disable tcp connect
* Return:
*    			void
* Note:
*    
*---------------------------------------------------------------*/
void rc_server_tcp_enable_set(int enable)
{
	osal_mutex_lock(&tcp_mutex);
	g_tcp_recv_connect = enable;
	osal_mutex_unlock(&tcp_mutex);
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_tcp_enable_get
* DESCRIPTION:
*    			get tcp connect 
* ARGUMENTS:
*				void
* Return:
*    			enable or disable tcp connect
* Note:
*    
*---------------------------------------------------------------*/
int rc_server_tcp_enable_get(void)
{
	int tcp_recv_connect = 0;
	
	osal_mutex_lock(&tcp_mutex);
	tcp_recv_connect = g_tcp_recv_connect;
	osal_mutex_unlock(&tcp_mutex);

	return tcp_recv_connect;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_tcp_client_set
* DESCRIPTION:
*    			set tcp client connect 
* ARGUMENTS:
*				int enable:1 client have connected, 0:client have not connected
* Return:
*    			void
* Note:
*    
*---------------------------------------------------------------*/
void rc_server_tcp_client_set(int enable)
{
	osal_mutex_lock(&client_connect_mutex);
	g_tcp_client_connect = enable;
	osal_mutex_unlock(&client_connect_mutex);
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_server_tcp_client_get
* DESCRIPTION:
*    			get tcp client connect 
* ARGUMENTS:
*				void
* Return:
*    			have or not client connected
* Note:
*    
*---------------------------------------------------------------*/
int rc_server_tcp_client_get(void)
{
	int tcp_client_connect = 0;
	
	osal_mutex_lock(&client_connect_mutex);
	tcp_client_connect = g_tcp_client_connect;
	osal_mutex_unlock(&client_connect_mutex);

	return tcp_client_connect;
}

#ifdef SCREEN_CAP_FILE_TEST
extern unsigned char * remote_screen_cap_buf(unsigned long *buf_len, unsigned int pic_w, unsigned int pic_h); 
extern int rc_fb2bmp(void);
int main(int argc, char** argv)
{
	unsigned long buf_len;
	
	LOGI("server, screen capture start... ");
	if( 3 != argc )
	{
		LOGI("input error, you need input: [process name] [width] [height], example: test_remote_control 640 480");
		return 0;
	}
	LOGI("picture w:%d, h:%d", atoi(argv[1]), atoi(argv[2]));
	remote_screen_cap_buf(&buf_len, atoi(argv[1]), atoi(argv[2]));
	//rc_fb2bmp();
	return 0;
}

#else
/*---------------------------------------------------------------
* FUNCTION NAME: main
* DESCRIPTION:
*    			remote control process entry point
* ARGUMENTS:
*				void 
* Return:
*    			void
* Note:
*    
*---------------------------------------------------------------*/
int main(void)
{
	struct sockaddr_in to_addr;
	struct sockaddr_in from_addr;
	socklen_t addr_len;

	int on;
	int server_sockfd;
	int recv_len, send_len;	
	rc_fb_t fb;
	char recv_buf[RC_DATA_LEN];
	char send_buf[RC_DATA_LEN];
	
	LOGI("server, remote control start... \n");
	
	if( sem_init(&g_pthread_sem, 0, 0) < 0 )
	{
		LOGE("server, sem_init failed\n");
		exit(0);
	}
	
	rc_server_pthread_create(RC_THREAD_EVENT_SEND, rc_event_send, 0);
	rc_server_pthread_create(RC_THREAD_SENSOR_LISTEN, rc_sensor_listen, 0);
	rc_server_pthread_create(RC_THREAD_SENSOR_SEND, rc_sensor_send, 0);
	rc_server_pthread_create(RC_THREAD_TCP_RECV, rc_stream_recv, 0);
	rc_server_pthread_create(RC_THREAD_TCP_SEND, rc_stream_send, 0);
	rc_server_pthread_create(RC_THREAD_PERCEPTION_SENSOR, rc_perception_sensor, 0);
	rc_server_pthread_create(RC_THREAD_SERVICE_SEND, rc_service_send, 0);
	
	server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if( server_sockfd < 0 )
	{
		LOGE("server, create socket fail \n");
		exit(0);
	}

	/* Enable address reuse, then we can reconnect this server */ 
	on = 1; 
	setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ); 
	
	memset(&from_addr, 0, sizeof(from_addr));
	from_addr.sin_family = AF_INET;
	from_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	from_addr.sin_port = htons(RC_UDP_PORT);
	if( bind(server_sockfd, (struct sockaddr*)&from_addr, sizeof(from_addr)) < 0 )
	{
		LOGE("server, bind socket fail \n");
		goto EXIT;
	}

	if( rc_framebuffer_info(FB_DEV_PATH, &fb) < 0 )
	{
		LOGE("UDP, cannnot open framebuffer device\n");
		fb.width = 0;
		fb.height = 0;
	}
	
	LOGI("UDP, receive data at port : %d \n", RC_UDP_PORT);
	addr_len = sizeof(to_addr);
	while(1)
	{
		memset(recv_buf, 0, RC_DATA_LEN);
		recv_len = recvfrom(server_sockfd, recv_buf, RC_DATA_LEN, 0, (struct sockaddr*)&to_addr, &addr_len);
		if( recv_len < 0 )
		{
			LOGE("UDP, recvfrom fail \n");
			goto EXIT;
		}

		LOGI("UDP, client scanning server, IP:%s :%d, cmd:%s\n", inet_ntoa(to_addr.sin_addr), ntohs(to_addr.sin_port), recv_buf);
		
		if( 0 == strcmp(recv_buf, RC_CLIENT_SCAN) )
		{
			//LOGI("UDP, 1.client scanning server, IP:%s :%d\n", inet_ntoa(to_addr.sin_addr), ntohs(to_addr.sin_port));
			LOGI("UDP, 1.client scanning server, IP:%s ,w = %d, h = %d\n", inet_ntoa(to_addr.sin_addr), fb.width, fb.height);
			
			send_buf[0] = ((fb.width>>8)&0xff);
			send_buf[1] = (fb.width&0xff);
			send_buf[2] = ((fb.height>>8)&0xff);
			send_buf[3] = (fb.height&0xff);
			send_buf[4] = rc_server_tcp_client_get();
			send_buf[5] = 0;
			
			send_len = sendto(server_sockfd, send_buf, 5, 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
			if( send_len < 0 )
			{
				LOGE("UDP, sendto client scan fail \n");
				goto EXIT;
			}
			continue;
		}
		
		//LOGI("UDP, receive data from client len = %d, data = %s\n", recv_len, recv_buf);
		if( 0 == strcmp(recv_buf, RC_CLIENT_REQ) )
		{			LOGI("UDP, 2.client send request connect \n");
			strcpy( send_buf, RC_CLIENT_REQ_CONFIRM);
			send_len = sendto(server_sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
			if( send_len < 0 )
			{
				LOGE("UDP, sendto client request connect fail \n");
				goto EXIT;
			}

			g_connect_confirm = 1;
			continue;
		}

		if( 0 == strcmp(recv_buf, RC_CLIENT_REQ_OK) )
		{
			LOGI("UDP, 3.client send request OK\n");
			if( 1 == g_connect_confirm )
			{
			#if 0
				if( 1 == g_tcp_pthread_create )
				{
					LOGI("UDP, server is used, exit TCP recv thread\n");
					rc_server_pthread_kill(g_tcp_recv_pthread_id);
					g_tcp_recv_pthread_id = 0;
				}

				rc_server_pthread_create(RC_THREAD_TCP_RECV, rc_stream_recv, 0);
				g_tcp_pthread_create = 1;
			#endif

				//rc_server_tcp_enable_set(1);
				g_connect_confirm = 0;

				strcpy( send_buf, RC_SERVER_LISTEN);
				send_len = sendto(server_sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
				if( send_len < 0 )
				{
					LOGE("UDP, sendto client listen fail \n");
					goto EXIT;
				}
			}
			else
			{
				LOGE("UDP, client send request OK, but not confirm\n");
				strcpy( send_buf, RC_CLIENT_NO_CONNECT);
				send_len = sendto(server_sockfd, send_buf, strlen(send_buf), 0, (struct sockaddr*)&to_addr, sizeof(to_addr));
				if( send_len < 0 )
				{
					LOGE("UDP, sendto client no connect fail \n");
					goto EXIT;
				}
			}
		}
	}

EXIT:
	close(server_sockfd);

	sem_destroy(&g_pthread_sem);
	exit(1);
	return 0;
}

#endif

