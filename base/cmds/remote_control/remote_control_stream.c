/** @file remote_control_stream.c
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
#define LOG_TAG	"remote_control"

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
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
#include <linux/fb.h>


#include "osal_task_comm.h"
#include "remote_control.h"
#include "remote_control_local.h"
#include "remote_control_stream.h"
#include "remote_screen_cap.h"

static sem_t g_sem_data_sync;
static sem_t g_thread_exit_sem;
static osal_queue_t g_tcp_thread_queue;
static unsigned char msg_buf[RC_DATA_LEN + 1];

//static int g_tcp_init = 0;
//static int g_server_sockfd = 0;
static int g_client_sockfd = 0;

extern pthread_t g_tcp_send_pthread_id;

extern osal_queue_t g_event_thread_queue;
extern osal_queue_t g_sensor_thread_queue;
extern osal_queue_t g_service_thread_queue;

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_recv_signal_process
* DESCRIPTION:
*    			exit tcp receive pthread no normal
* ARGUMENTS:
*				int sig:signal id
* Return:
*    			void
* Note:
*
*---------------------------------------------------------------*/
static void rc_stream_recv_signal_process(int sig)
{
	if( SIGQUIT == sig )
	{
		LOGI("TCP, recv thread exit, signal_process SIGQUIT : %d \n", SIGQUIT);

		rc_stream_send_exit();
		
		close(g_client_sockfd);
		//close(g_server_sockfd);

		//g_server_sockfd = 0;
		g_client_sockfd = 0;

		pthread_exit("thread kill success");
	}
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_send_msg
* DESCRIPTION:
*    			send message to specify queue
* ARGUMENTS:
*				osal_queue_t *osal_q:queue
*				unsigned char *p_data:data stream
*				int data_len:stream length
* Return:
*    			void
* Note:
*    
*---------------------------------------------------------------*/
void rc_stream_send_msg(osal_queue_t *osal_q, unsigned char *p_data, int data_len)
{
	msg_t msg;

	msg_buf[0] = data_len;
	memcpy(&msg_buf[1], p_data, data_len);

	msg.pointer = (unsigned int)msg_buf;
	osal_queue_post(osal_q, &msg);

	if( sem_wait(&g_sem_data_sync) < 0 ) 
		LOGE("TCP, send msg sem_wait failed\n");
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_recv_msg
* DESCRIPTION:
*    			receive message from stream thread
* ARGUMENTS:
*				void
* Return:
*    			void
* Note:
*    
*---------------------------------------------------------------*/
void rc_stream_recv_msg(void)
{
	sem_post(&g_sem_data_sync);
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_connect
* DESCRIPTION:
*    			client connect to server via tcp protocol
* ARGUMENTS:
*				int socketfd:client socket fd
* Return:
*    			void
* Note:
*
*---------------------------------------------------------------*/
static void rc_stream_connect(int *p_socketfd)
{
	msg_t msg;
	unsigned char data[2];
	
	rc_server_tcp_client_set(1);
	//rc_server_tcp_enable_set(0);

	msg.type = RC_STREAM_TYPE_SOCKETFD;
	msg.pointer = *p_socketfd;
	osal_queue_post(&g_tcp_thread_queue, &msg);

	//send msg to service thread and then send msg to framewrok via local socket
	data[0] = RC_EVENT_TYPE_SERVICE;
	data[1] = CLIENT_STATUS_CONNECT;
	rc_stream_send_msg(&g_service_thread_queue, data, 2);
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_disconnect
* DESCRIPTION:
*    			client disconnect to server
* ARGUMENTS:
*				int socketfd:client socket fd
* Return:
*    			void
* Note:
*
*---------------------------------------------------------------*/
static void rc_stream_disconnect(int *p_socketfd)
{
	msg_t msg;
	unsigned char data[2];
	
	close(*p_socketfd);
	*p_socketfd = 0;
	rc_server_tcp_client_set(0);

	msg.type = RC_STREAM_TYPE_SOCKETFD;
	msg.pointer = 0;
	osal_queue_post(&g_tcp_thread_queue, &msg);

	//send msg to service thread and then send msg to framewrok via local socket
	data[0] = RC_EVENT_TYPE_SERVICE;
	data[1] = CLIENT_STATUS_DISCONNECT;
	rc_stream_send_msg(&g_service_thread_queue, data, 2);
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_recv_process
* DESCRIPTION:
*    			process cmd from client and send ACK to client
* ARGUMENTS:
*				int *socketfd:client socket fd
*				unsigned char *p_data:data stream
*				int data_len:stream length
* Return:
*    			void
* Note:
*
*---------------------------------------------------------------*/

static void rc_stream_recv_process(int *socketfd, unsigned char *p_data, int data_len)
{
	msg_t msg;
	unsigned char cmd;
	unsigned char data[2];
	unsigned char result = RC_SUCCESS;
	
	assert(NULL != p_data);
	assert(NULL != socketfd);
	
	cmd = p_data[0];
    switch(cmd)
    {
    	case RC_EVENT_TYPE_KEY:
		case RC_EVENT_TYPE_TOUCH:
		case RC_EVENT_TYPE_TRACKBALL:
		case RC_EVENT_TYPE_SWITCH_MODE:
		{		
			if( data_len > RC_DATA_LEN )
			{
				LOGI("TCP, [error]data_len > RC_DATA_LEN\n");
				result = RC_FAIL;
				break;
			}

			#if 0
			msg_buf[0] = data_len;
			memcpy(&msg_buf[1], p_data, data_len);
	
			msg.pointer = (unsigned int)msg_buf;
			osal_queue_post(&g_event_thread_queue, &msg);
			#endif

			rc_stream_send_msg(&g_event_thread_queue, p_data, data_len);
        }
		break;

		case RC_EVENT_TYPE_SENSOR:
		{
			if( data_len > RC_DATA_LEN )
			{
				LOGI("TCP, [error]data_len > RC_DATA_LEN\n");
				result = RC_FAIL;
				break;
			}

			#if 0
			msg_buf[0] = data_len;
			memcpy(&msg_buf[1], p_data, data_len);
	
			msg.pointer = (unsigned int)msg_buf;
			osal_queue_post(&g_sensor_thread_queue, &msg);
			#endif
			rc_stream_send_msg(&g_sensor_thread_queue, p_data, data_len);
        }
		break;

		case RC_EVENT_TYPE_GET_PICTURE:
		{
			unsigned char *p_buf;
			unsigned long buf_len = 0;
			unsigned int w, h;
			w = ((p_data[1]&0xff)<<8)|(p_data[2]&0xff);
			h = ((p_data[3]&0xff)<<8)|(p_data[4]&0xff);
			p_buf = remote_screen_cap_buf(&buf_len, w, h);
			//p_buf=remote_screen_jpeg(&buf_len);
			if( NULL != p_buf )
			{
				int ret;
				int fp;
				unsigned char *p_data = NULL;							
				p_data = (unsigned char *)malloc(buf_len + 1);
				if( NULL == p_data )
				{
					LOGI("server, get picture [error] memory not enough\n");
					result = RC_FAIL;
					break;
				}	
				
				memset(p_data, 0, buf_len + 1);							
				p_data[0] = RC_EVENT_TYPE_GET_PICTURE;
				memcpy(p_data + 1, p_buf, buf_len);
				free(p_buf);
				p_buf = NULL;

				ret = rc_server_write(*socketfd, p_data, buf_len + 1);
				if(p_data){
					free(p_data);
					p_data = NULL;
				}
				
				if( ret <= 0 )
				{
					LOGI("TCP, socket has some error disconnect\n");
					rc_stream_disconnect(socketfd);
				}
			}
			else
			{
				result = RC_FAIL;
				break;
			}
		}
		return;//do not send ack to client

		case RC_EVENT_TYPE_SERVICE:
		{
			if( data_len > RC_DATA_LEN )
			{
				LOGI("TCP, [error]data_len > RC_DATA_LEN\n");
				result = RC_FAIL;
				break;
			}

			#if 0
			msg_buf[0] = data_len;
			memcpy(&msg_buf[1], p_data, data_len);
	
			msg.pointer = (unsigned int)msg_buf;
			osal_queue_post(&g_sensor_thread_queue, &msg);
			#endif
			rc_stream_send_msg(&g_service_thread_queue, p_data, data_len);
		}
		break;
		
		case RC_EVENT_TYPE_ACK:	return;

        default:
		{
			LOGE("TCP, [error]cmd name:%d\n", cmd);
			result = RC_FAIL;
        }
		break;
    }
	
	  data[0] = RC_EVENT_TYPE_ACK;
	  data[1] = result;
	  rc_server_write(*socketfd, data, sizeof(data));	 

}

#if 0
/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_recv
* DESCRIPTION:
*    			remote control stream recv thread entry, receive data from client 
* ARGUMENTS:
*				void *p:parameter
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_stream_recv(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	
	int on;
	//int server_sockfd = 0;
	int client_sockfd = 0;
    int server_len, stream_len;
	int read_count;
	socklen_t client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
	struct timeval time_out = {RC_HEARTBEAT_TIME, 0}; 
    unsigned char rec_buf[RC_DATA_LEN];

	LOGI("TCP, recv thread entry \n");

	signal(SIGQUIT, rc_stream_recv_signal_process);
	
	sem_post(psem);

	if(!g_tcp_init)
	{
		/*  Remove any old socket and create an unnamed socket for the server.  */
	    if((g_server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		{
			LOGE("TCP, can not create socket error(%s)\n", strerror(errno));
			goto EXIT;
		}
		/*  Name the socket.  */
		memset(&server_address, 0, sizeof(server_address));
	    server_address.sin_family = AF_INET;
	    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	    server_address.sin_port = htons(RC_TCP_PORT);
	    server_len = sizeof(server_address);

		/* Enable address reuse, then we can reconnect this server */ 
		on = 1; 
		setsockopt(g_server_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ); 
		
		//setsockopt(server_sockfd, SOL_SOCKET,SO_SNDTIMEO, (char *)&time_out, sizeof(struct timeval));
		//setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_out, sizeof(struct timeval));//receive timer out

		if(bind(g_server_sockfd, (struct sockaddr *)&server_address, server_len) == -1)
		{
			LOGE("TCP, can not Bind to socket error(%s)\n", strerror(errno));
			goto EXIT;
		}

		/*  Create a connection queue and wait for clients.  */
	    if(listen(g_server_sockfd, 5) == -1)
		{
			LOGE("TCP, listen %d error(%s)\n", g_server_sockfd, strerror(errno));
			goto EXIT;
	    }	
		g_tcp_init = 1;
	}
	

	LOGI("TCP, waiting connect... %d\n", RC_TCP_PORT);
		
	client_len = sizeof(client_address);
	client_sockfd = accept(g_server_sockfd, (struct sockaddr *)&client_address, &client_len);
	if( -1 == client_sockfd )
	{	
		LOGE("TCP, accept:%d error(%s)\n", client_sockfd, strerror(errno));
		goto EXIT;
	}

	LOGI("TCP, new connection accept %s:%d \n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

	rc_server_pthread_create(RC_THREAD_TCP_SEND, rc_stream_send, client_sockfd);
	
	//g_server_sockfd = server_sockfd;
	g_client_sockfd = client_sockfd;
    while(1) 
    {
    	unsigned char size_buf[4];
		
        memset(rec_buf, 0, RC_DATA_LEN);
		memset(size_buf, 0, 4);
		read_count = recv(client_sockfd, size_buf, 4, 0);
        if( read_count == 0 )            
        {
			LOGI("TCP, client disconnect\n");
            break;
        }
        else if( read_count < 0 )
        {
			LOGI("TCP, have error in connect, maybe recv timerout, exit recv thread\n");
            break;
        } 
        else
        {  
        	int bytes_left = 0;
			int bytes_read = 0;

			#ifdef RC_DEBUG
			{
			    int i=0;
			    for(i = 0; i < read_count; i++)
			    {
			        LOGI("TCP, receive stream buffer[%d] = 0x%x \n", i, size_buf[i]);
			    }
			}
			#endif
			
        	stream_len = rc_server_byte_2_int(size_buf);
			bytes_left = stream_len;
			while(bytes_left > 0)//begin to read data
			{			
				bytes_read = recv(client_sockfd, &rec_buf[bytes_read], bytes_left, 0);
				bytes_left -= bytes_read; 
			}

			//LOGI("TCP, read stream from client len = %d \n", stream_len);
			#ifdef RC_DEBUG
			{
			    int i=0;
			    for(i = 0; i < stream_len; i++)
			    {
			        LOGI("TCP, receive stream data[%d] = 0x%x \n", i, rec_buf[i]);
			    }
			}
			#endif
			
        	//process cmd
        	rc_stream_recv_process(client_sockfd, rec_buf, stream_len);
        }         
    }

EXIT:
	close(client_sockfd);
	//close(server_sockfd);

	//g_server_sockfd = 0;
	g_client_sockfd = 0;

	g_tcp_pthread_create = 0;
	rc_stream_send_exit();
	
	LOGI("TCP, recv thread exit \n");
	pthread_exit("thread exit success");

	return NULL;
}
#endif

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_recv
* DESCRIPTION:
*    			remote control stream recv thread entry, receive data from client 
* ARGUMENTS:
*				void *p:parameter
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_stream_recv(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	
	int on;
	int server_sockfd = 0;
	int client_sockfd = 0;
    int server_len, stream_len;
	int read_count;
	socklen_t client_len;
    struct sockaddr_in server_address;
    struct sockaddr_in client_address;
	struct timeval time_out = {RC_HEARTBEAT_TIME, 0}; 
    unsigned char rec_buf[RC_DATA_LEN];
	fd_set fdsr;
    int max_sock;
	msg_t msg;
	
	LOGI("TCP, recv thread entry \n");

	sem_post(psem);

	if( sem_init(&g_sem_data_sync, 0, 0) < 0 )
	{
		LOGE("TCP, recv thread sem_init fail\n");
		goto EXIT;
	}

	/*  Remove any old socket and create an unnamed socket for the server.  */
    if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		LOGE("TCP, can not create socket error(%s)\n", strerror(errno));
		goto EXIT;
	}
	/*  Name the socket.  */
	memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(RC_TCP_PORT);
    server_len = sizeof(server_address);

	/* Enable address reuse, then we can reconnect this server */ 
	on = 1; 
	setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ); 
	
	//setsockopt(server_sockfd, SOL_SOCKET,SO_SNDTIMEO, (char *)&time_out, sizeof(struct timeval));
	//setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&time_out, sizeof(struct timeval));//receive timer out

	if(bind(server_sockfd, (struct sockaddr *)&server_address, server_len) == -1)
	{
		LOGE("TCP, can not Bind to socket error(%s)\n", strerror(errno));
		goto EXIT;
	}

	/*  Create a connection queue and wait for clients.  */
    if(listen(server_sockfd, 5) == -1)
	{
		LOGE("TCP, listen %d error(%s)\n", server_sockfd, strerror(errno));
		goto EXIT;
    }		

	LOGI("TCP, waiting connect... %d\n", RC_TCP_PORT);
	max_sock = server_sockfd;
    while (1)
    {
        FD_ZERO(&fdsr);
        FD_SET(server_sockfd, &fdsr);
		if( 0 != client_sockfd )
		{
			FD_SET(client_sockfd, &fdsr);
		}
		
        if (select(max_sock + 1, &fdsr, NULL, NULL, NULL) < 0)
        {
            LOGE("TCP, select failed: %s\n", strerror(errno));
            break;
        }

		//client send data to server
		if( 0 != client_sockfd )
		{
			if (FD_ISSET(client_sockfd, &fdsr))
	        {
	        	unsigned char size_buf[4];
		
		        memset(rec_buf, 0, RC_DATA_LEN);
				memset(size_buf, 0, 4);
				read_count = recv(client_sockfd, size_buf, 4, 0);
		        if( read_count == 0 )            
		        {
					LOGI("TCP, client disconnect\n");
		            rc_stream_disconnect(&client_sockfd);
		        }
		        else if( read_count < 0 )
		        {
					LOGI("TCP, have error in connect, maybe recv timerout, exit recv thread\n");
					rc_stream_disconnect(&client_sockfd);
		        } 
		        else
		        {  
		        	int bytes_left = 0;
					int bytes_read = 0;

					#ifdef RC_DEBUG
					{
					    int i=0;
					    for(i = 0; i < read_count; i++)
					    {
					        LOGI("TCP, receive stream buffer[%d] = 0x%x \n", i, size_buf[i]);
					    }
					}
					#endif
					
		        	stream_len = rc_server_byte_2_int(size_buf);
					bytes_left = stream_len;
					while(bytes_left > 0)//begin to read data
					{			
						bytes_read = recv(client_sockfd, &rec_buf[bytes_read], bytes_left, 0);
						bytes_left -= bytes_read; 
					}

					//LOGI("TCP, read stream from client len = %d \n", stream_len);
					#ifdef RC_DEBUG
					{
					    int i=0;
					    for(i = 0; i < stream_len; i++)
					    {
					        LOGI("TCP, receive stream data[%d] = 0x%x \n", i, rec_buf[i]);
					    }
					}
					#endif
					
		        	//process cmd
		        	rc_stream_recv_process(&client_sockfd, rec_buf, stream_len);
		        }   
	        }
		}

		//client connect to server
        if (FD_ISSET(server_sockfd, &fdsr))
        {
        	int sockfd = 0;
			
        	client_len = sizeof(client_address);
			sockfd = accept(server_sockfd, (struct sockaddr *)&client_address, &client_len);
			if( sockfd <= 0 )
			{	
				LOGE("TCP, accept:%d error(%s)\n", sockfd, strerror(errno));
				continue;
			}

			LOGI("TCP, new connection accept %s:%d \n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
			
			//whether enable tcp connect or not
			if(rc_server_tcp_enable_get())
			//if(1)
			{
				LOGI("TCP, server receive this client, server_fd = %d, cur_fd = %d, recv_fd = %d\n", server_sockfd, client_sockfd, sockfd);
				
				if( 0 != client_sockfd )
				{
					close(client_sockfd);
				}
				client_sockfd = sockfd;
				if (client_sockfd > max_sock)
				{
					max_sock = client_sockfd;
				}

				rc_stream_connect(&client_sockfd);
			}
			else
			{
				close(sockfd);
				LOGE("TCP, server refuse this client, because not confirm \n");
			}
        }
    }

EXIT:
	close(client_sockfd);
	close(server_sockfd);

	LOGI("TCP, recv thread exit \n");
	pthread_exit("thread exit success");

	return NULL;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_send
* DESCRIPTION:
*    			send ui state change to client
* ARGUMENTS:
*				void *p:socket fd and semaphore
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_stream_send(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	int client_sockfd = 0;
	int i, read_count;
	msg_t msg;
	unsigned char send_buf[RC_DATA_LEN];

	LOGI("TCP, send thread entry\n");
	
	sem_post(psem);

	osal_queue_create(&g_tcp_thread_queue, OSAL_MSG_QUEUE_DEPTH);
	while(1)
	{
		osal_queue_pend(&g_tcp_thread_queue, &msg);

		//LOGI("TCP, send thread receive msg type = %d\n", msg.type);
		if( RC_STREAM_TYPE_SOCKETFD == msg.type )
		{
			client_sockfd = msg.pointer;
		}
		
		if( RC_STREAM_TYPE_DATA == msg.type )
		{
			memset(send_buf, 0, RC_DATA_LEN);
			send_buf[0] = RC_EVENT_TYPE_UI_STATE;
			if( 0 != client_sockfd )
			{
				rc_server_write(client_sockfd, send_buf, sizeof(send_buf));
			}
		}
	}

	osal_queue_clean(&g_tcp_thread_queue);
	osal_queue_delete(&g_tcp_thread_queue);
	
	LOGI("TCP, send thread exit success\n");
	
	pthread_exit("thread exit success");
    return NULL;
}

#if 0
/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_send
* DESCRIPTION:
*    			send ui state change to client
* ARGUMENTS:
*				void *p:socket fd and semaphore
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_stream_send(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	int client_sockfd = data[0];
	int i, read_count;
	msg_t msg;
	unsigned char send_buf[RC_DATA_LEN];

	LOGI("TCP, send thread entry client_sockfd = %d\n", client_sockfd);
	
	sem_post(psem);
	if( sem_init(&g_thread_exit_sem, 0, 0) < 0 )
	{
		LOGE("TCP, send thread sem_init fail\n");
	}
	
	osal_queue_create(&g_tcp_thread_queue, OSAL_MSG_QUEUE_DEPTH);
	while(1)
	{
		osal_queue_pend(&g_tcp_thread_queue, &msg);

		//LOGI("TCP, send thread receive msg type = %d\n", msg.type);
		if( RC_STREAM_TYPE_EXIT == msg.type )
		{
			break;
		}
		
		if( RC_STREAM_TYPE_DATA == msg.type )
		{
			memset(send_buf, 0, RC_DATA_LEN);
			send_buf[0] = RC_EVENT_TYPE_UI_STATE;
			rc_server_write(client_sockfd, send_buf, sizeof(send_buf));
		}
	}

	sem_post(&g_thread_exit_sem);
	sem_destroy(&g_thread_exit_sem); 
	
	LOGI("TCP, send thread exit success\n");
	
	pthread_exit("thread exit success");
    return NULL;
}
#endif

/*---------------------------------------------------------------
* FUNCTION NAME: rc_stream_send_exit
* DESCRIPTION:
*    			exit tcp send pthread
* ARGUMENTS:
*				void
* Return:
*    			void
* Note:
*
*---------------------------------------------------------------*/
void rc_stream_send_exit(void)
{
	int ret;
	msg_t msg;

	LOGI("TCP, send thread exit, thread_id : %d \n", (int)g_tcp_send_pthread_id);
	if( 0 != g_tcp_send_pthread_id )
	{
		msg.type = RC_STREAM_TYPE_EXIT;
		osal_queue_post(&g_tcp_thread_queue, &msg);
		ret = sem_wait(&g_thread_exit_sem);
		if( ret < 0 ) LOGE("TCP, sem_wait failed\n");
		
		osal_queue_clean(&g_tcp_thread_queue);
		osal_queue_delete(&g_tcp_thread_queue);
			
		rc_server_pthread_exit(g_tcp_send_pthread_id);
		//pthread_join(g_tcp_send_pthread_id, NULL); //waiting for the end of listen_thread
		g_tcp_send_pthread_id = 0;
	}
}

