/** @file remote_sensor_hub.c
 *  @par Copyright:
 *  - Copyright 2011 Amlogic Inc as unpublished work                             
 *  All Rights Reserved                                                                                                                              
 *  - The information contained herein is the confidential property            
 *  of Amlogic.  The use, copying, transfer or disclosure of such information
 *  is prohibited except by express written agreement with Amlogic Inc. 
 *  @author   tellen
 *  @version  1.0        
 *  @date     2012/03/01
 *  @par function description:
 *  - 1 remote control server, receive perception sensor data from client
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
#include <dirent.h>
#include <sys/resource.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/limits.h>

#include <linux/input.h>

#include "osal_task_comm.h"
#include "remote_control.h"
#include "remote_control_local.h"
#include "remote_control_stream.h"
#include "remote_sensor_hub.h"

#define test_bit(bit, array)    (array[bit/8] & (1<<(bit%8)))

static int g_epoll_fd = -1;
static int g_inotify_fd = -1;
static int g_data_fd = -1;
static char g_dev_name[PATH_MAX];

extern osal_queue_t g_sensor_thread_queue;
	
/*---------------------------------------------------------------
* FUNCTION NAME: rc_ps_init
* DESCRIPTION:
*    			init perception sensor
* ARGUMENTS:
*				void
* Return:
*    			RC_FAIL or RC_SUCCESS
* Note:
*    
*---------------------------------------------------------------*/
static int rc_ps_init(void)
{
	int result;
	struct epoll_event eventItem;
	
	g_epoll_fd = epoll_create(RC_SENSOR_EPOLL_SIZE);
	if( g_epoll_fd < 0 )
	{
		LOGE("perception sensor, Could not create epoll instance.  errno=%d", errno);
		return RC_FAIL;
	}

	g_inotify_fd = inotify_init();
    result = inotify_add_watch(g_inotify_fd, RC_SENSOR_DEVICE_PATH, IN_DELETE | IN_CREATE);
	if( result < 0 )
	{
		LOGE("perception sensor, Could not register INotify for %s.  errno=%d", RC_SENSOR_DEVICE_PATH, errno);
		return RC_FAIL;
	}

    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.u32 = RC_EPOLL_ID_INOTIFY;
    result = epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, g_inotify_fd, &eventItem);
	if( 0 != result )
	{
		LOGE("perception sensor, Could not add INotify to epoll instance.  errno=%d", errno);
		return RC_FAIL;
	}

	return RC_SUCCESS;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_ps_open_device
* DESCRIPTION:
*    			open perception sensor device
* ARGUMENTS:
*				void
* Return:
*    			RC_FAIL or RC_SUCCESS
* Note:
*    
*---------------------------------------------------------------*/
static int rc_ps_open_device(const char *devicePath) 
{
	int fd;
	char buffer[80];
	struct epoll_event eventItem;
	struct input_id inputId;
	
    LOGI("perception sensor, Opening device: %s", devicePath);

    fd = open(devicePath, O_RDWR);
    if(fd < 0) 
	{
        LOGE("perception sensor, could not open %s, %s\n", devicePath, strerror(errno));
        return RC_FAIL;
    }

	/*
	// Get device name.
    if(ioctl(fd, EVIOCGNAME(sizeof(buffer) - 1), &buffer) < 1)
	{
        LOGE("perception sensor, could not get device name for %s, %s\n", devicePath, strerror(errno));
		goto ERR;
    } 
	else
	{
        buffer[sizeof(buffer) - 1] = '\0';
    }*/

	// Get device identifier.
    if(ioctl(fd, EVIOCGID, &inputId)) 
	{
        LOGE("perception sensor, could not get device input id for %s, %s\n", devicePath, strerror(errno));
        goto ERR;
    }
	
	if(( 0x04b4 != inputId.vendor ) || ( 0x1035 != inputId.product ))//·ÉÖÇµç×Ódongle
	{
		LOGE("perception sensor, production fail\n");
        goto ERR;
	}

	// See if this is a cursor device such as a trackball or mouse. 
	{
		unsigned char keyBitmask[(KEY_MAX + 1) / 8];
        unsigned char relBitmask[(REL_MAX + 1) / 8];

		// Figure out the kinds of events the device reports.
	    ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBitmask)), keyBitmask);
	    ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relBitmask)), relBitmask);

	    if (!test_bit(BTN_MOUSE, keyBitmask)
	        || !test_bit(REL_X, relBitmask)
	        || !test_bit(REL_Y, relBitmask)) 
		{
			LOGI("perception sensor, this is not a sensor device\n");
			goto ERR;
	    }
	}

	LOGI("perception sensor, this is a sensor device\n");
	
    // Make file descriptor non-blocking for use with poll().
    if (fcntl(fd, F_SETFL, O_NONBLOCK)) 
	{
        LOGE("perception sensor, Error %d making device file descriptor non-blocking.", errno);
        goto ERR;
    }

    // Register with epoll.
    memset(&eventItem, 0, sizeof(eventItem));
    eventItem.events = EPOLLIN;
    eventItem.data.u32 = RC_EPOLL_ID_DATA;
    if (epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, fd, &eventItem)) 
	{
        LOGE("perception sensor, Could not add device fd to epoll instance.  errno=%d", errno);
		goto ERR;
    }

	g_data_fd = fd;
    return RC_SUCCESS;

ERR:
	close(fd);
	return RC_FAIL;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_ps_close_device
* DESCRIPTION:
*    			close perception sensor device
* ARGUMENTS:
*				void
* Return:
*    			RC_FAIL or RC_SUCCESS
* Note:
*    
*---------------------------------------------------------------*/
static int rc_ps_close_device(const char *devicePath) 
{
	if (epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, g_data_fd, NULL)) 
	{
        LOGE("perception sensor, Could not remove device fd from epoll instance.  errno=%d", errno);
		return RC_FAIL;
    }

	if (g_data_fd >= 0) 
	{
        close(g_data_fd);
        g_data_fd = -1;
    }
	
    return RC_SUCCESS;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_ps_read_nofity
* DESCRIPTION:
*    			read perception sensor notify
* ARGUMENTS:
*				void
* Return:
*    			RC_FAIL or RC_SUCCESS
* Note:
*    
*---------------------------------------------------------------*/
static int rc_ps_read_nofity(void) 
{
    int res;
    char devname[PATH_MAX];
    char *filename;
    char event_buf[512];
    int event_size;
    int event_pos = 0;
    struct inotify_event *event;

    res = read(g_inotify_fd, event_buf, sizeof(event_buf));
    if(res < (int)sizeof(*event)) 
	{
        if(errno == EINTR)
            return RC_SUCCESS;
        LOGE("perception sensor, could not get event, %s\n", strerror(errno));
        return RC_FAIL;
    }

    strcpy(devname, RC_SENSOR_DEVICE_PATH);
    filename = devname + strlen(devname);
    *filename++ = '/';

    while(res >= (int)sizeof(*event))
	{
        event = (struct inotify_event *)(event_buf + event_pos);
        if(event->len) 
		{
			//LOGI("perception sensor, device '%s' add to system\n", event->name);
			
			//if( !strcmp(event->name, RC_SENSOR_DEVICE_NAME) )
			{
				strcpy(filename, event->name);
	            if(event->mask & IN_CREATE) 
				{
					if( RC_SUCCESS == rc_ps_open_device(devname) )
					{
						strcpy(g_dev_name, event->name);
					}
	            } 
				else
				{
					if( !strcmp(g_dev_name, devname) )
					{
						LOGI("perception sensor, Removing device '%s' due to inotify event\n", devname);
		                rc_ps_close_device(devname);
						g_dev_name[0] = '\0';
					}
	            }
			}
        }
        event_size = sizeof(*event) + event->len;
        res -= event_size;
        event_pos += event_size;
    }
    return RC_SUCCESS;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_ps_scan_dir
* DESCRIPTION:
*    			remote control perception sensor, check device point exist or not
* ARGUMENTS:
*				const char *dirname:
* Return:
*    			int
* Note:
*    
*---------------------------------------------------------------*/
static int rc_ps_scan_dir(const char *dirname)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return RC_FAIL;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir))) {
        if(de->d_name[0] == '.' &&
           (de->d_name[1] == '\0' ||
            (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        rc_ps_open_device(devname);
    }
    closedir(dir);
    return RC_SUCCESS;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_ps_process
* DESCRIPTION:
*    			remote control perception sensor, process data from client
* ARGUMENTS:
*				const char *data:
* Return:
*    			int
* Note:
*    
*---------------------------------------------------------------*/
static int rc_ps_process(const char *data)
{
	int i;
	char acc_data[3];
	char gyro_data[3];
	char send_data[50];
	
	for( i = 0; i < 33; i++ )
	{
		LOGI("perception sensor, data[%d]: %d\n", i, data[i]);
	}
					
	send_data[0] = RC_EVENT_TYPE_SENSOR;//input type
	send_data[1] = SENSOR_TYPE_ACCELEROMETER;//sensor type
	//x axis
	send_data[2] = 0;
	send_data[3] = 0;
	send_data[4] = 0;
	send_data[5] = data[13];
	//y axis
	send_data[6] = 0;
	send_data[7] = 0;
	send_data[8] = 0;
	send_data[9] = data[15];
	//z axis
	send_data[10] = 0;
	send_data[11] = 0;
	send_data[12] = 0;
	send_data[13] = data[17];
	//accuracy
	send_data[14] = 0;
	send_data[15] = 0;
	send_data[16] = 0;
	send_data[17] = 0;
	rc_stream_send_msg(&g_sensor_thread_queue, (unsigned char *)send_data, 18);

	send_data[0] = RC_EVENT_TYPE_SENSOR;//input type
	send_data[1] = SENSOR_TYPE_GYROSCOPE;//sensor type
	//x axis
	send_data[2] = 0;
	send_data[3] = 0;
	send_data[4] = 0;
	send_data[5] = data[19];
	//y axis
	send_data[6] = 0;
	send_data[7] = 0;
	send_data[8] = 0;
	send_data[9] = data[21];
	//z axis
	send_data[10] = 0;
	send_data[11] = 0;
	send_data[12] = 0;
	send_data[13] = data[23];
	//accuracy
	send_data[14] = 0;
	send_data[15] = 0;
	send_data[16] = 0;
	send_data[17] = 0;
	rc_stream_send_msg(&g_sensor_thread_queue, (unsigned char *)send_data, 18);

	return RC_SUCCESS;
}

/*---------------------------------------------------------------
* FUNCTION NAME: rc_perception_sensor
* DESCRIPTION:
*    			remote control perception sensor, receive data from client
* ARGUMENTS:
*				void *p:parameter
* Return:
*    			void *
* Note:
*    
*---------------------------------------------------------------*/
void *rc_perception_sensor(void *p)
{
	int poll_result;
	int *data = (int *)p;
	sem_t *psem = (sem_t *)data[1];

	// The array of pending epoll events and the index of the next event to be handled.
	struct epoll_event pending_event_items[RC_SENSOR_EPOLL_MAX_EVENTS];
	size_t pending_event_count = 0;
	size_t pending_event_index = 0;
	bool pending_inotify = false;

	LOGI("perception sensor, thread entry \n");

	sem_post(psem);

	if( RC_SUCCESS == rc_ps_init() )
	{
		rc_ps_scan_dir(RC_SENSOR_DEVICE_PATH);
		while(1)
		{
			while (pending_event_index < pending_event_count) 
			{
	            const struct epoll_event *eventItem = &pending_event_items[pending_event_index++];
	            if (eventItem->data.u32 == RC_EPOLL_ID_INOTIFY) 
				{
	                if (eventItem->events & EPOLLIN) 
					{
	                    pending_inotify = true;
	                } 
					else 
					{
	                    LOGW("perception sensor, Received unexpected epoll event 0x%08x for INotify.", eventItem->events);
	                }
	                continue;
	            }

	            if (eventItem->events & EPOLLIN) 
				{
					char buf[100] = {0};
					int len;

					//len = rc_server_read(g_data_fd, buf, 1);

					len = read(g_data_fd, buf, 33);
					LOGW("perception sensor, g_data_fd:%d, len:%d", g_data_fd, len);
					
	                if (len == 0 || (len < 0 && errno == ENODEV)) 
					{
	                    // Device was removed before INotify noticed.
	                    LOGW("perception sensor, could not get event, removed? (fd: %d size: %d errno: %d)\n", g_data_fd, len, errno);
	                    rc_ps_close_device(NULL);
	                } 
					else if (len < 0) 
	                {
	                    if (errno != EAGAIN && errno != EINTR) 
						{
	                        LOGW("perception sensor, could not get event (errno=%d)", errno);
	                    }
	                } 
					else 
					{
						//process data
						rc_ps_process(buf);
					}
	            }
				else 
				{
	                LOGW("perception sensor, Received unexpected epoll event 0x%08x", eventItem->events);
	            }
	        }

	        if (pending_inotify && pending_event_index >= pending_event_count) 
			{
	            pending_inotify = false;
	            rc_ps_read_nofity();
	        }

			pending_event_index = 0;
			
			poll_result = epoll_wait(g_epoll_fd, pending_event_items, RC_SENSOR_EPOLL_MAX_EVENTS, -1);
			if (poll_result < 0) 
			{
	            // An error occurred.
	            pending_event_count = 0;

	            // Sleep after errors to avoid locking up the system.
	            // Hopefully the error is transient.
	            if (errno != EINTR) 
				{
	                LOGW("perception sensor, poll failed (errno=%d)\n", errno);
	                usleep(100000);
	            }
	        } 
			else
			{
	            // Some events occurred.
	            pending_event_count = (size_t)poll_result;
        	}
		}
	}

#if 0
	int fd = inotify_init();
	int wd = inotify_add_watch(fd, RC_SENSOR_DEVICE_PATH, IN_ALL_EVENTS);
	while(1)
	{   
		fd_set fds;   
		FD_ZERO(&fds);                
		FD_SET(fd, &fds);   

		if (select(fd + 1, &fds, NULL, NULL, NULL) > 0)
		{   
			int len, index = 0;   
			while (((len = read(fd, &buf, sizeof(buf))) < 0) && (errno == EINTR));//no event
				while (index < len) 
				{   
					event = (struct inotify_event *)(buf + index);//get event                   
					LOGI("perception sensor, event->mask: 0x%08x, event->name: %s\n", event->mask, event->name);
					index += sizeof(struct inotify_event) + event->len;//next event
				}   
		}   
	}   

	//remove fd watch
	inotify_rm_watch(fd, wd);
#endif
	LOGI("perception sensor, thread exit \n");
	pthread_exit("thread exit success");
    return NULL;
}

