/** @file remote_sensor_hub.h
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
 
#ifndef _REMOTE_SENSOR_HUB_H_
#define _REMOTE_SENSOR_HUB_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
	飞智电子游戏手柄HID数据格式 Ver3.0
	Byte (Zero-based)	Name			Detail information	Data range	Resolution	Remark
	0				HID Device ID							0x05		
	1				HID Mouse Button	Bit 0 : Left Btn
										Bit 1 : right Btn			Copy of HID mouse data
	2				Projected X			X axis pointing data	[-128, 127]		
	3				Projected Y			Y axis pointing data	[-128, 127]		
	4	Scroll	Scroll Data	[-128, 127]		
	5 ~ 6	Version ID	Indicate version ID(unsigned short)	0x3412		
	7	Controller ID	Differentiate different controllers	[1,5]		Indicate different controller for multiple devices single USB-dongle connection simultaneously
	8	Reserved		0		
	9 ~ 10	Button	Hardware button data	BYTE 8:
	BIT7 : O button
	BIT6 : B button
	BIT5 : UP button
	BIT4 : DOWN button
	BIT3 : RIGHT button
	BIT2 : LEFT button
	BIT1 : A button
	BIT0 : □ button	BYTE 9: 
	BIT7 : reserved
	BIT6 : reserved
	BIT5 : reserved
	BIT4 : ? button
	BIT3 : - button
	BIT2 : + button
	BIT1 : Menu button
	BIT0 : X button	Each bit indicate button status:
	1 : pressed
	0 : released
	11 ~ 12	Reserved		0		
	13	Acc data X(char)	Acceleration data	[-128,127]	72mg/digit	
	14	Reserved		0		
	15	Acc data Y(char)	Acceleration data	[-128,127]	72mg/digit	
	16	Reserved		0		
	17	Acc data Z(char)	Acceleration data	[-128,127]	72mg/digit	
	18	Reserved		0		
	19	Gyro data X(char)	Gyro data	[-128,127]	4.5 deg/sec	
	20	Reserved		0		
	21	Gyro data Y(char)	Gyro data	[-128,127]	4.5 deg/sec	
	22	Reserved				
	23	Gyro data Z(char)	Gyro data	[-128,127]	4.5 deg/sec	
	24 ~ 32	Reserved		0		
**/

#define RC_SENSOR_DEVICE_PATH			"/dev/input"//"/dev"//"/dev/hidraw2"
#define RC_SENSOR_DEVICE_NAME			"mouse1"//"hidraw2"
#define RC_SENSOR_EPOLL_SIZE			8
#define RC_SENSOR_EPOLL_MAX_EVENTS		16

enum
{
	RC_EPOLL_ID_INOTIFY = 0,
	RC_EPOLL_ID_DATA
};

void *rc_perception_sensor(void *p);

#ifdef __cplusplus
}
#endif

#endif/*_REMOTE_SENSOR_HUB_H_*/

