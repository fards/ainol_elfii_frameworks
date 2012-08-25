/** @file remote_control_local.h
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
 
#ifndef _REMOTE_CONTROL_LOCAL_H_
#define _REMOTE_CONTROL_LOCAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LISTEN_NUM				10

//this must sync. with RemoteControlService.java
#define CLIENT_STATUS_CONNECT		0x80;
#define CLIENT_STATUS_DISCONNECT	0x81;
	
//this must sync. with sensor.java
enum
{
	SENSOR_TYPE_ACCELEROMETER = 1,
    SENSOR_TYPE_MAGNETIC_FIELD = 2,
    SENSOR_TYPE_ORIENTATION = 3,
    SENSOR_TYPE_GYROSCOPE = 4,
    SENSOR_TYPE_LIGHT = 5,
	SENSOR_TYPE_PRESSURE = 6,
    SENSOR_TYPE_TEMPERATURE = 7,
    SENSOR_TYPE_PROXIMITY = 8,
    SENSOR_TYPE_GRAVITY = 9,
    SENSOR_TYPE_LINEAR_ACCELERATION = 10,
    SENSOR_TYPE_ROTATION_VECTOR = 11,
    SENSOR_TYPE_RELATIVE_HUMIDITY = 12,
    SENSOR_TYPE_AMBIENT_TEMPERATURE = 13,
    SENSOR_TYPE_ALL = -1
};

void *rc_event_send(void *p);
void *rc_event_recv(void *p);
void *rc_sensor_listen(void *p);
void *rc_sensor_send(void *p);
void *rc_service_send(void *p);
void *rc_service_recv(void *p);

#ifdef __cplusplus
}
#endif

#endif/*_REMOTE_CONTROL_LOCAL_H_*/

