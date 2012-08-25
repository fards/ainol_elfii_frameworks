/** @file osal_task_comm.h
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
 *  - 1 Operating System Abstraction Layer
 *  @warning This class may explode in your face.
 *  @note If you inherit anything from this class, you're doomed.
 */

#ifndef _OSAL_TASK_COMM_H_
#define _OSAL_TASK_COMM_H_

#ifdef __cplusplus	
extern "C" {	
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
//#include <mqueue.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

//#include "remote_control.h"

#define OSAL_MSG_QUEUE_DEPTH 		20

#define OSAL_MACRO_START			do {
#define OSAL_MACRO_END				}while(0)

#define OSAL_FREE(p)				OSAL_MACRO_START	\
										free(p);	\
										p=NULL;		\
									OSAL_MACRO_END
									
#define OSAL_VALID_PTR(ptr)			((int)(ptr) != 0)

enum
{
	OSAL_SUCCESS = 0,
	OSAL_INVALID_APP = 0xE00,	/* The app is invalid */
	OSAL_MISSING_CODE,  /* Missing code, can not load the instructions */
	OSAL_NOMORE_STACK,
	OSAL_ERR_CREATING,
	OSAL_ERR_DESTROY,
	OSAL_INTERNAL_ERR,
	OSAL_INVALID_HDLR,
	OSAL_INVALID_NAME
};

typedef struct
{
	unsigned char type;
	unsigned char sub_type;

	union 
	{
		unsigned int pointer;
		unsigned short data[3];
		//unsigned char data[RC_DATA_LEN + 1];
	};
}msg_t;

typedef struct
{
    unsigned char queue_head;
    unsigned char queue_tail;
    unsigned char queue_count;
    unsigned char queue_size;
	msg_t *msg;
    sem_t msg_syn;  
}osal_queue_t;

pthread_mutex_t *osal_mutex_create(void);
int osal_mutex_delete(pthread_mutex_t *mutex);
int osal_mutex_lock(pthread_mutex_t *mutex);
int osal_mutex_unlock(pthread_mutex_t *mutex);

int osal_queue_create(osal_queue_t *osal_q, unsigned int queue_size);
int osal_queue_delete(osal_queue_t *osal_q);
int osal_queue_post(osal_queue_t *osal_q, msg_t * p_msg);
int osal_queue_pend(osal_queue_t *osal_q, msg_t *msg);
int osal_queue_clean(osal_queue_t *osal_q);

#ifdef __cplusplus	
}	
#endif

#endif/*_OSAL_TASK_COMM_H_*/

