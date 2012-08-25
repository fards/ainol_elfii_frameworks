/** @file remote_control_local.c
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
 *  - 1 remote control server, communication with local(framework)
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
#include "remote_control_local.h"
#include "remote_control_stream.h"

#include "remote_key_map.c"

static int g_cur_count = 0;
static int g_sock_fd[MAX_LISTEN_NUM];
osal_queue_t g_event_thread_queue;
osal_queue_t g_sensor_thread_queue;
osal_queue_t g_service_thread_queue;

/*---------------------------------------------------------------
* FUNCTION NAME: rc_event_send
* DESCRIPTION:
*    			remote control event send thread, send event to framework
* ARGUMENTS:
*				void *p:parameter
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_event_send(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	
    struct sockaddr addr;
    socklen_t alen;
    int lsocket, client_sockfd = 0;

	LOGI("local event, send thread entry \n");

	sem_post(psem);
	
    lsocket = android_get_control_socket("remote_control_event");
    if (lsocket < 0) 
	{
		LOGE("local event, Failed to get socket from environment: %s\n", strerror(errno));
        goto EXIT;
    }

    if (listen(lsocket, 5)) 
	{
        LOGE("local event, Listen on socket failed: %s\n", strerror(errno));
        goto EXIT;
    }
    //fcntl(lsocket, F_SETFD, FD_CLOEXEC);
	
	
	while(1)
	{
		alen = sizeof(addr);
        client_sockfd = accept(lsocket, &addr, &alen);
        if (client_sockfd >= 0)
		{
            //fcntl(lsocket, F_SETFD, FD_CLOEXEC);
			break;
        }

		LOGE("local event, Accept failed: %s\n", strerror(errno));
	}

	rc_server_pthread_create(RC_THREAD_EVENT_RECV, rc_event_recv, client_sockfd);
	
	osal_queue_create(&g_event_thread_queue, OSAL_MSG_QUEUE_DEPTH);
	while(1)
	{
		msg_t msg;
		unsigned char *p_data;
		unsigned char data[RC_DATA_LEN + 1];
		
		osal_queue_pend(&g_event_thread_queue, &msg);
		p_data = (unsigned char *)msg.pointer;
		memcpy(&data, p_data, p_data[0] + 1);
		p_data = data;
		rc_stream_recv_msg();

		#ifdef RC_DEBUG
		{
		    int i=0;
		    for(i = 0; i < p_data[0]; i++)
		    {
		        LOGI("local event, receive msg data = 0x%x \n", p_data[1 + i]);
		    }
		}
		#endif
		
		if( RC_EVENT_TYPE_KEY == p_data[1] )//map to system keycode
		{
			int i;
			for( i = 0; i < RC_KEYCODE_TOTAL; i++ )
			{
				if( g_key_map[i].rc_keycode == p_data[3] )
				{
					p_data[3] = g_key_map[i].sys_keycode;
					break;
				}
			}
		}
		
		rc_server_write(client_sockfd, p_data + 1, p_data[0]);
	}

EXIT:
	close(client_sockfd);
	close(lsocket);

	LOGI("local event, send thread exit \n");
	pthread_exit("thread exit success");
    return NULL;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_event_recv
* DESCRIPTION:
*    			receive ui state change from framework
* ARGUMENTS:
*				void *p:socket fd and semaphore
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_event_recv(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	int client_sockfd = data[0];
	int read_count;
	unsigned char rec_buf[RC_DATA_LEN];

	LOGI("local event, recv thread entry client_sockfd = %d\n", client_sockfd);

	sem_post(psem);
	
	while(1) 
    {
        memset(rec_buf, 0, RC_DATA_LEN);
		read_count = recv(client_sockfd, rec_buf, RC_DATA_LEN, 0);
		
		LOGI("local event, read stream from framework len = %d \n", read_count);
		#ifdef RC_DEBUG
		{
			int i;
			for(i = 0; i < read_count; i++)
			{
				LOGI( "local event, read_buf[%d] = %d\n", i, rec_buf[i]);
			}
		}	
		#endif
		
        if( read_count == 0 )            
        {
			LOGI("local event, client disconnect\n");
            break;
        }
        else if( read_count < 0 )
        {
			LOGI("local event, have error in connect\n");
            break;
        } 
        else
        {  
        	//process cmd
        }         
    }

	LOGI("local event, recv thread exit \n");
	
	pthread_exit("thread exit success");
    return NULL;
}

/*-------------------------------------------------------------------------
* FUNCTION NAME: rc_sensor_listen
* DESCRIPTION:
*    			remote control sensor listen thread, listen connect or close from client
* ARGUMENTS:
*				void *p:parameter
* Return:
*    			void *
* Note:
*    
*-------------------------------------------------------------------------*/
void *rc_sensor_listen(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];

	struct timeval time_out = {3, 0};
    struct sockaddr addr;
    socklen_t alen;
    int server_sockfd, client_sockfd = 0;
	int i, on, ret;
	fd_set fdsr;
    int max_sock;

	LOGI("local sensor, listen thread entry \n");

	sem_post(psem);
 
    server_sockfd = android_get_control_socket("remote_control_sensor");
    if (server_sockfd < 0) 
	{
		LOGE("local sensor, Failed to get socket from environment: %s\n", strerror(errno));
        goto EXIT;
    }

	/* Enable address reuse, then we can reconnect this server */ 
	on = 1; 
	setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ); 
	
    if (listen(server_sockfd, MAX_LISTEN_NUM)) 
	{
        LOGE("local sensor, Listen on socket failed: %s\n", strerror(errno));
        goto EXIT;
    }
    //fcntl(lsocket, F_SETFD, FD_CLOEXEC);

	memset(g_sock_fd, 0, sizeof(g_sock_fd));
    g_cur_count = 0;
    alen = sizeof(addr);
    max_sock = server_sockfd;
    while (1)
    {
        FD_ZERO(&fdsr);
        FD_SET(server_sockfd, &fdsr);
		
        for (i = 0; i < MAX_LISTEN_NUM; i++)
        {
            if (g_sock_fd[i] != 0)
            {
                FD_SET(g_sock_fd[i], &fdsr);
            }
        }

        //ret = select(max_sock + 1, &fdsr, NULL, NULL, &time_out);
		ret = select(max_sock + 1, &fdsr, NULL, NULL, NULL);
        if (ret < 0)
        {
            LOGE("local sensor, select failed: %s\n", strerror(errno));
            break;
        }
        else if (ret == 0)
        {
            LOGE("local sensor, select timeout\n");
            continue;
        }

        for (i = 0; i < MAX_LISTEN_NUM; i++)
        {
        	if( g_sock_fd[i] > 0 )
        	{
        		if (FD_ISSET(g_sock_fd[i], &fdsr))
	            {
	            	unsigned char buf[1];
	                ret = recv(g_sock_fd[i], buf, sizeof(buf), 0);
	                if (ret <= 0) //receive data error
	                {       
	                    close(g_sock_fd[i]);
	                    FD_CLR(g_sock_fd[i], &fdsr);
	                    g_sock_fd[i] = 0;
						g_cur_count--;
						LOGI("local sensor, client close the socket cur_num = %d\n", g_cur_count);
	                }
	                else
					{  
						LOGI("local sensor, client[%d] send:0x%x\n", i, buf[0]);
					}
	            }
        	}
        }

        if (FD_ISSET(server_sockfd, &fdsr))
        {
			client_sockfd = accept(server_sockfd, &addr, &alen);
            if (client_sockfd <= 0)
            {
				LOGE("local sensor, Accept failed: %s\n", strerror(errno));
                continue;
            }

            if (g_cur_count < MAX_LISTEN_NUM)
            {
            	for (i = 0; i < MAX_LISTEN_NUM; i++)
		        {
		            if ( 0 == g_sock_fd[i] )
		            {
		                g_sock_fd[i] = client_sockfd;
						g_cur_count++;
						break;
		            }
		        }

				if (client_sockfd > max_sock)
				{
					max_sock = client_sockfd;
				}

                LOGI("local sensor, new connection client_num = %d, client_fd = %d\n", g_cur_count, client_sockfd);
            }
            else
            {
				LOGE("local sensor, max connections arrive discard\n");
				send(client_sockfd, "bye", 4, 0);
                close(client_sockfd);
            }
        }
    }
	
EXIT:
	for (i = 0; i < MAX_LISTEN_NUM; i++)
    {
        if (g_sock_fd[i] != 0)
        {
            close(g_sock_fd[i]);
        }
    }

	close(server_sockfd);

	LOGI("local sensor, listen thread exit \n");
	pthread_exit("thread exit success");
    return NULL;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_sensor_send
* DESCRIPTION:
*    			remote control sensor send thread, send sensor data to framework
* ARGUMENTS:
*				void *p:parameter
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_sensor_send(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	int i;

	LOGI("local sensor, send thread entry \n");

	sem_post(psem);
	osal_queue_create(&g_sensor_thread_queue, OSAL_MSG_QUEUE_DEPTH);
	while(1)
	{
		msg_t msg;
		unsigned char *p_data;
		unsigned char data[RC_DATA_LEN + 1];
		
		osal_queue_pend(&g_sensor_thread_queue, &msg);
		p_data = (unsigned char *)msg.pointer;
		memcpy(&data, p_data, p_data[0] + 1);
		p_data = data;
		rc_stream_recv_msg();
		
		#ifdef RC_DEBUG
	    for(i = 0; i < p_data[0]; i++)
	    {
	        LOGI("local sensor, receive msg data = 0x%x \n", p_data[1 + i]);
	    }
		LOGI("local sensor, receive msg, current client num:%d\n", g_cur_count);
		#endif

		//LOGI("local sensor, receive msg, current client num:%d\n", g_cur_count);
		for (i = 0; i < MAX_LISTEN_NUM; i++)
        {
            if (g_sock_fd[i] != 0)
            {
				rc_server_write(g_sock_fd[i], p_data + 1, p_data[0]);
            }
        }
	}

	osal_queue_clean(&g_sensor_thread_queue);
	osal_queue_delete(&g_sensor_thread_queue);
		
	LOGI("local sensor, send thread exit \n");
	pthread_exit("thread exit success");
    return NULL;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_service_send
* DESCRIPTION:
*    			remote control service send thread, send service data to framework
* ARGUMENTS:
*				void *p:parameter
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_service_send(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	
    struct sockaddr addr;
    socklen_t alen;
    int lsocket, client_sockfd = 0;

	LOGI("local service, send thread entry \n");

	sem_post(psem);
	
    lsocket = android_get_control_socket("remote_control_service");
    if (lsocket < 0) 
	{
		LOGE("local service, Failed to get socket from environment: %s\n", strerror(errno));
        goto EXIT;
    }

    if (listen(lsocket, 5)) 
	{
        LOGE("local service, Listen on socket failed: %s\n", strerror(errno));
        goto EXIT;
    }
    //fcntl(lsocket, F_SETFD, FD_CLOEXEC);

	while(1)
	{
		alen = sizeof(addr);
        client_sockfd = accept(lsocket, &addr, &alen);
        if (client_sockfd >= 0)
		{
            //fcntl(lsocket, F_SETFD, FD_CLOEXEC);
			break;
        }

		LOGE("local service, Accept failed: %s\n", strerror(errno));
	}

	rc_server_pthread_create(RC_THREAD_SERVICE_RECV, rc_service_recv, client_sockfd);
	
	osal_queue_create(&g_service_thread_queue, OSAL_MSG_QUEUE_DEPTH);
	while(1)
	{
		msg_t msg;
		unsigned char *p_data;
		unsigned char data[RC_DATA_LEN + 1];
		
		osal_queue_pend(&g_service_thread_queue, &msg);
		p_data = (unsigned char *)msg.pointer;
		memcpy(&data, p_data, p_data[0] + 1);
		p_data = data;
		rc_stream_recv_msg();

		#ifdef RC_DEBUG
		{
		    int i=0;
		    for(i = 0; i < p_data[0]; i++)
		    {
		        LOGI("local service, receive msg data = 0x%x \n", p_data[1 + i]);
		    }
		}
		#endif
		
		rc_server_write(client_sockfd, p_data + 1, p_data[0]);
	}

EXIT:
	close(client_sockfd);
	close(lsocket);

	LOGI("local service, send thread exit \n");
	pthread_exit("thread exit success");
    return NULL;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_service_recv
* DESCRIPTION:
*    			receive control info from framework
* ARGUMENTS:
*				void *p:socket fd and semaphore
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_service_recv(void *p)
{
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];
	int client_sockfd = data[0];
	int read_count;
	unsigned char rec_buf[RC_DATA_LEN];

	LOGI("local service, recv thread entry client_sockfd = %d\n", client_sockfd);

	sem_post(psem);
	
	while(1) 
    {
    	unsigned char size_buf[4];

        memset(rec_buf, 0, RC_DATA_LEN);
		memset(size_buf, 0, 4);
		read_count = recv(client_sockfd, size_buf, 4, 0);
        if( read_count == 0 )            
        {
			LOGI("local service, client disconnect\n");
            break;
        }
        else if( read_count < 0 )
        {
			LOGI("local service, have error in connect\n");
            break;
        } 
        else
        {  
        	int bytes_left = 0;
			int bytes_read = 0;
			int stream_len = 0;

			#ifdef RC_DEBUG
			{
			    int i=0;
			    for(i = 0; i < read_count; i++)
			    {
			        LOGI("local service, receive stream buffer[%d] = 0x%x \n", i, size_buf[i]);
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

			#ifdef RC_DEBUG
			{
			    int i=0;
			    for(i = 0; i < stream_len; i++)
			    {
			        LOGI("local service, receive stream data[%d] = 0x%x \n", i, rec_buf[i]);
			    }
			}
			#endif
			

			if( 0 == rec_buf[0] )
			{
				//disable daemon, daemon can not receive client
				rc_server_tcp_enable_set(0);
			}
			else
			{
				//enable daemon
				rc_server_tcp_enable_set(1);
			}
        }   
    }

	LOGI("local service, recv thread exit \n");
	
	pthread_exit("thread exit success");
    return NULL;
}

