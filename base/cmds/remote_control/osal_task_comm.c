/** @file osal_task_comm.c
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
#define LOG_TAG	"remote_control"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "osal_task_comm.h"
#include <cutils/log.h>
//#define OSAL_PRINT(...)				printf(__VA_ARGS__)
#define OSAL_PRINT(...)				LOGI(__VA_ARGS__)

static pthread_mutex_t queue_mutex;

static void _mqueue_lock_entity(void)
{
	assert(queue_mutex != NULL);
	osal_mutex_lock(&queue_mutex);
}

static void _mqueue_unlock_entity(void)
{
	assert(queue_mutex != NULL);	
	osal_mutex_unlock(&queue_mutex);
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_mutex_create
* DESCRIPTION:
*    			osal create a mutex
* ARGUMENTS:
*				pthread_mutex_t *mutex:
* Return:
*    			pthread_mutex_t *
* Note:
*    
*---------------------------------------------------------------*/
pthread_mutex_t *osal_mutex_create(void)
{
	pthread_mutexattr_t attr;

	pthread_mutex_t *mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	assert(OSAL_VALID_PTR(mutex));

	//OSAL_PRINT("Mutex create :0x%x\n", (int)mutex);

	pthread_mutexattr_init( &attr );
	if(mutex != NULL)
	{
		pthread_mutex_init(mutex, &attr );
	}

	return mutex;
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_mutex_delete
* DESCRIPTION:
*    			osal delete a mutex
* ARGUMENTS:
*				pthread_mutex_t *mutex:
* Return:
*    			OSAL_SUCCESS or error
* Note:
*    
*---------------------------------------------------------------*/
int osal_mutex_delete(pthread_mutex_t *mutex)
{
	unsigned int ret = OSAL_SUCCESS;

	assert(OSAL_VALID_PTR(mutex));
	//OSAL_PRINT("Mutex delete : 0x%x\n", (int)mutex);

	if(mutex == NULL)
	{
		ret = OSAL_INVALID_HDLR;
		goto _EXIT;
	}
	else
	{
		int tmp = pthread_mutex_destroy(mutex);
		free(mutex);
		mutex = NULL;

		ret = (tmp==0) ? OSAL_SUCCESS : OSAL_ERR_DESTROY;
	}
	
_EXIT:
	if(ret != OSAL_SUCCESS)
		OSAL_PRINT("OSAL, Mutex Delete error: result = 0x%x\n", (int)ret);
	
	return ret;
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_mutex_lock
* DESCRIPTION:
*    			osal mutex lock
* ARGUMENTS:
*				pthread_mutex_t *mutex:
* Return:
*    			-1:create fail 0:success
* Note:
*    
*---------------------------------------------------------------*/
int osal_mutex_lock(pthread_mutex_t *mutex)
{
	assert(OSAL_VALID_PTR(mutex));
	if( pthread_mutex_lock(mutex) == EDEADLK )
	{
		OSAL_PRINT("OSAL, Mutex is deadlock: 0x%x\n", (int)mutex);
		assert(0 && "System is deadlock!");
		return -1;
	}

	return 0;
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_mutex_unlock
* DESCRIPTION:
*    			osal mutex unlock
* ARGUMENTS:
*				pthread_mutex_t *mutex:
* Return:
*    			-1:create fail 0:success
* Note:
*    
*---------------------------------------------------------------*/
int osal_mutex_unlock(pthread_mutex_t *mutex)
{
	assert(OSAL_VALID_PTR(mutex));
	return pthread_mutex_unlock(mutex);
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_queue_create
* DESCRIPTION:
*    			osal create a message queue
* ARGUMENTS:
*				osal_queue_t *osal_q:queue struct
*				unsigned int queue_size:queue size
* Return:
*    			OSAL_SUCCESS or error
* Note:
*    
*---------------------------------------------------------------*/
int osal_queue_create(osal_queue_t *osal_q, unsigned int queue_size)
{
    int ret = OSAL_SUCCESS;
    
	_mqueue_lock_entity();	
	
    if(osal_q == NULL)
	{
		OSAL_PRINT("OSAL, create queue point is null \n");
		ret = OSAL_ERR_CREATING;
		goto _EXIT;
	}
    
	memset(osal_q, 0, sizeof(osal_queue_t));
    
    if(queue_size == 0)
    {
        OSAL_PRINT("OSAL, create queue size null \n");
        ret = OSAL_ERR_CREATING;
        goto _EXIT;
    }
    
	if((osal_q->msg = (msg_t *)malloc(queue_size*sizeof(msg_t))) == NULL )
	{
	    OSAL_PRINT("OSAL, create queue malloc fail \n");
        ret = OSAL_ERR_CREATING;
		goto _EXIT;
	}
    osal_q->queue_size = queue_size; 
    ret = sem_init(&(osal_q->msg_syn), 0, 0);  
    if(ret != 0)
    {
        OSAL_PRINT("OSAL, create queue sem_init fail \n");
        ret = OSAL_ERR_CREATING;
        goto _EXIT;
    }
 	//OSAL_PRINT("OSAL, created queue success size is %d\n",(osal_q->queue_size));

_EXIT:
	_mqueue_unlock_entity();
	return ret;    
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_queue_delete
* DESCRIPTION:
*    			osal delete a message queue
* ARGUMENTS:
*				osal_queue_t *osal_q:queue struct
* Return:
*    			OSAL_SUCCESS or error
* Note:
*    
*---------------------------------------------------------------*/
int osal_queue_delete(osal_queue_t *osal_q)
{
	int ret = OSAL_SUCCESS;

	_mqueue_lock_entity();

    osal_q->queue_head = 0;
    osal_q->queue_tail= 0;
    osal_q->queue_count= 0;

	OSAL_FREE(osal_q->msg);
    ret = sem_destroy(&osal_q->msg_syn);  
    assert(ret == 0);
	_mqueue_unlock_entity();

	return ret;
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_queue_post
* DESCRIPTION:
*    			osal post a message to destination pthread queue
* ARGUMENTS:
*				osal_queue_t *osal_q:queue struct
*				msg_t * p_msg:
* Return:
*    			OSAL_SUCCESS or error
* Note:
*    
*---------------------------------------------------------------*/
int osal_queue_post(osal_queue_t *osal_q, msg_t * p_msg)
{
	int ret = OSAL_SUCCESS;
	
	//OSAL_PRINT("OSAL, queue post queue_count: %d queue_size is %d\n", (osal_q->queue_count),(osal_q->queue_size));
    _mqueue_lock_entity();    
    if(osal_q->queue_count >= osal_q->queue_size)
    {
        OSAL_PRINT("OSAL, queue post but queue full queue = 0x%x\n", (unsigned int)osal_q);
        ret = OSAL_INTERNAL_ERR;
        goto _EXIT;
    }
    else
        osal_q->queue_count ++;

    if(osal_q->queue_head <= (osal_q->queue_size - 1) )
    {
        memcpy(&(osal_q->msg[osal_q->queue_head]),p_msg,sizeof(msg_t));
        ret = sem_post(&(osal_q->msg_syn));   
        assert(ret == 0);
    }
    
	if(osal_q->queue_head >= (osal_q->queue_size - 1) )
        osal_q->queue_head = 0;      
    else
        osal_q->queue_head ++;
        
_EXIT:
    _mqueue_unlock_entity();
    
	return ret;
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_queue_pend
* DESCRIPTION:
*    			osal pend a message
* ARGUMENTS:
*				osal_queue_t *osal_q:queue struct
*				msg_t * p_msg:
* Return:
*    			OSAL_SUCCESS or error
* Note:
*    
*---------------------------------------------------------------*/
int osal_queue_pend(osal_queue_t *osal_q, msg_t *p_msg)
{
	int ret = OSAL_SUCCESS;
    
    ret = sem_wait(&(osal_q->msg_syn));  //wait msg
    assert(ret == 0);
	_mqueue_lock_entity();
	if(osal_q->queue_count == 0)
    {
        goto _EXIT; //no msg
    }
    
    if(osal_q->queue_tail<= (osal_q->queue_size - 1) )
    {
        memcpy(p_msg,&(osal_q->msg[osal_q->queue_tail]),sizeof(msg_t));
        memset(&(osal_q->msg[osal_q->queue_tail]),0,sizeof(msg_t));
    }
    
    if(osal_q->queue_tail >= (osal_q->queue_size - 1) )
        osal_q->queue_tail = 0;      
    else
        osal_q->queue_tail ++;
    
    osal_q->queue_count --;
    
_EXIT:
    _mqueue_unlock_entity();
    return ret;
}

/*---------------------------------------------------------------
* FUNCTION NAME: osal_queue_clean
* DESCRIPTION:
*    			osal clean a pthread's message queue
* ARGUMENTS:
*				osal_queue_t *osal_q:queue struct
* Return:
*    			OSAL_SUCCESS or error
* Note:
*    
*---------------------------------------------------------------*/
int osal_queue_clean(osal_queue_t *osal_q)
{
	int ret = OSAL_SUCCESS;	
    _mqueue_lock_entity();
    osal_q->queue_head = 0;
    osal_q->queue_tail = 0;
    osal_q->queue_count = 0;    
    memset(osal_q->msg, 0, (osal_q->queue_size)*sizeof(msg_t));
    ret = sem_init(&(osal_q->msg_syn), 0, 0); 
    assert(ret == 0);
    _mqueue_unlock_entity();
    
	return ret;
}

