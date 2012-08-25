//
// Copyright 2010 The Android Open Source Project
//
// The input reader.
//
#define LOG_TAG "InputRemote"

#define REMOTE_DEBUG   0

//must sync with remote_control.h
/*
#define EVENT_TYPE_ACK				        0
#define EVENT_TYPE_KEY				        1
#define EVENT_TYPE_TOUCH				    2
#define EVENT_TYPE_TRACKBALL			    3
#define EVENT_TYPE_SENSOR				    4
#define EVENT_TYPE_UI_STATE			        5
#define EVENT_TYPE_GET_PICTURE		        6
#define INPUT_DEVICE_ID_KEY			        0x10004
#define INPUT_DEVICE_ID_TOUCH			    0x10000*/
#define PAD_MOUSE_TIMER				        5000//ms
#define MIN_TIME_OF_DOUBLE_TAP_SETP2 	    50
#define MAX_TIME_OF_DOUBLE_TAP_SETP2 	    500
#define MIN_TIME_OF_DOUBLE_TAP_SETP1	    0
#define MAX_TIME_OF_DOUBLE_TAP_SETP1	    150
#define PAD_MOUSE_SENSITIVE_RANGE		    20
#define KEYCOD_EPAD_MOUSE_SWITCH		    200
#define KEYCOD_EPAD_MOUSE_SELECT		    201
    
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <math.h>

#include <cutils/log.h>
#include <cutils/sockets.h>
#include "InputRemote.h"

namespace android {

template<typename T>
inline static T abs(const T& value) {
    return value < 0 ? - value : value;
}

// --- InputRemote ---
InputRemote::InputRemote(const sp<InputReaderPolicyInterface> &reader, 
        const sp<InputDispatcherInterface>& dispatcher) :
        mReader(reader),
        mDispatcher(dispatcher) {    
    mMetaState = AMETA_NONE;
    mConnected = false;
    mSocket = new RemoteSocket("remote_control_event");

    mCount = 0;
    mMotionMode = REMOTE_MOTION_MODE_TOUCH;
    mPadMouseCurX = 0;
    mPadMouseCurY = 0;
    mPadMouseLastX = 0;
    mPadMouseLastY = 0;
    mPointerController=NULL;
    mDisplayWidth = 0;
    mDisplayHeight = 0;
    //reader->getDisplayInfo(0, &mDisplayWidth, &mDisplayHeight, &mDisplayOrientation);
}

InputRemote::~InputRemote() {
}

bool InputRemote::loopOnce() {       
    if(!mConnected){
        if(mSocket->requestConnect()){
            LOGI("connect to remote_control_event socket ok");
            mConnected = true;
        }else{
            if( mCount > 10 ){
                return false;
            }
            mCount++;
        }
    }else{
        readEvent();

#if REMOTE_DEBUG
     LOGI("remote Input event: eventType=%d(1:key, 2:touch) action=%d keycode=%d x=%d y=%d",
                mEventData.type, mEventData.action, mEventData.keycode,
                (int32_t)mEventData.x, (int32_t)mEventData.y);
#endif

        process(&mEventData);
    }

    return true;
}

void InputRemote::readEvent() {
    int dataLen;
    char *inStream;
    char data[4];

#if REMOTE_DEBUG
    LOGI("readEvent...");
#endif
    
	if( mSocket->readx(data, 4) < 4 ){
        LOGE("1.read data length fail");
	}
	
	dataLen = 0;
	for(int i = 0; i < 4; i++){
		dataLen += (data[i]&0xff)<<(8*i); 
	}

#if REMOTE_DEBUG
    LOGI("receive data length = %d", dataLen);
#endif

	inStream = new char[dataLen];
    if( mSocket->readx(inStream, dataLen) < dataLen ){
        LOGE("2.read data length fail");
	}

    memset(&mEventData, 0, sizeof(EventData));
    switch(inStream[0]){
    	case EVENT_TYPE_KEY:
            LOGI("key event action = %d, keycode = %d", inStream[1], (inStream[2]&0xff));
            mEventData.type = EVENT_TYPE_KEY;
            mEventData.action = inStream[1];
            mEventData.keycode = (inStream[2]&0xff);
    	break;

    	case EVENT_TYPE_TOUCH:
            mEventData.type = EVENT_TYPE_TOUCH;
            mEventData.action = inStream[1];
            mEventData.x = ((inStream[2]&0xff)<<8)|(inStream[3]&0xff);
            mEventData.y = ((inStream[4]&0xff)<<8)|(inStream[5]&0xff);
    	break;

    	case EVENT_TYPE_TRACKBALL:
    		LOGI("trackball event");
    	break;

    	case EVENT_TYPE_SENSOR:
    		LOGI("sensor event ");
    	break;

    	case EVENT_TYPE_UI_STATE:
    		LOGI("get ui state");
    	break;

        case EVENT_TYPE_SWITCH_MODE:
            if( AKEY_EVENT_ACTION_UP == inStream[1] ){
                LOGI("motion mode change, action:%d, keycode:%d, mode:%d (0:touch 1:mouse)",inStream[1], (inStream[2]&0xff), inStream[3]);
                //touch or mouse mode
        		if( KEYCOD_EPAD_MOUSE_SWITCH == (inStream[2]&0xff) ){
        			mMotionMode = inStream[3]; 
              if(mMotionMode==REMOTE_MOTION_MODE_MOUSE){
                   mPointerController=mReader->obtainPointerController(0);
                   mPointerController->setPresentation(PointerControllerInterface::PRESENTATION_POINTER);       
                   float x,y;
                   mPointerController->getPosition(&x, &y); 
                   mPointerController->unfade(PointerControllerInterface::TRANSITION_IMMEDIATE);
              }
              else{
                 mPointerController=NULL; 
                }
        		}
            
            }  
    	break;

    	default:
    		LOGE("event error: %d", inStream[0]);
    	break;
    }
    
    delete []inStream;
}

void InputRemote::process(const EventData *Event) {
    nsecs_t curTime;
    int32_t newMetaState;
    int32_t oldMetaState;
    
    curTime = systemTime(SYSTEM_TIME_MONOTONIC);
    switch (Event->type) {
    case EVENT_TYPE_KEY:
        if( AKEY_EVENT_ACTION_DOWN == Event->action ){
            mDownTime = curTime;
        }

        //touch or mouse mode
		if( KEYCOD_EPAD_MOUSE_SWITCH == Event->keycode ){
			if( AKEY_EVENT_ACTION_UP == Event->action ){
                //mMotionMode = (REMOTE_MOTION_MODE_TOUCH == mMotionMode)?REMOTE_MOTION_MODE_MOUSE:REMOTE_MOTION_MODE_TOUCH;
                //LOGE("motion mode change:%d (0:touch 1:mouse)", mMotionMode);
            }
		}
        else if( KEYCOD_EPAD_MOUSE_SELECT == Event->keycode ){
            if(REMOTE_MOTION_MODE_MOUSE == mMotionMode){
                int32_t action = ( AKEY_EVENT_ACTION_DOWN == Event->action )?AMOTION_EVENT_ACTION_DOWN:AMOTION_EVENT_ACTION_UP;
                sendMotion(REMOTE_MOTION_MODE_MOUSE, action, mPadMouseCurX, mPadMouseCurY, mDownTime);
            }
        }
        else{
            oldMetaState = mMetaState;
            newMetaState = updateMetaState(Event->keycode, (AKEY_EVENT_ACTION_DOWN == Event->action)?true:false, oldMetaState);
            if (oldMetaState != newMetaState) {
                mMetaState = newMetaState;
            }
            NotifyKeyArgs  mNotifyKeyArgs(curTime, INPUT_DEVICE_ID_KEY, AINPUT_SOURCE_KEYBOARD, 
                0, Event->action, AKEY_EVENT_FLAG_FROM_SYSTEM, Event->keycode, 
                0, mMetaState, mDownTime);
            mDispatcher->notifyKey(&mNotifyKeyArgs);
        }
        break;

    case EVENT_TYPE_TOUCH:
        
        if( AMOTION_EVENT_ACTION_DOWN == Event->action ){
            mDownTime = curTime;
        }
        
        if(REMOTE_MOTION_MODE_MOUSE == mMotionMode){
            bool sendEvent = false;
            
            if(( 0 == mDisplayWidth ) || ( 0 == mDisplayHeight )){
            	InputReaderConfiguration outConfig;
            	mReader->getReaderConfiguration(&outConfig);                         
              outConfig.getDisplayInfo(0, false,&mDisplayWidth, &mDisplayHeight, &mDisplayOrientation);
            }
            
            if( AMOTION_EVENT_ACTION_MOVE == Event->action ){
    			//mPadMouseCurX += (abs(Event->x - mPadMouseLastX) < mDisplayWidth/PAD_MOUSE_SENSITIVE_RANGE) ?
    			//	(Event->x - mPadMouseLastX):0;
    			//mPadMouseCurY += (abs(Event->y - mPadMouseLastY) < mDisplayHeight/PAD_MOUSE_SENSITIVE_RANGE) ?
    			//	(Event->y - mPadMouseLastY):0;
              
                mPadMouseCurX += (abs(Event->x - mPadMouseLastX) > PAD_MOUSE_SENSITIVE_RANGE) ?
    				(Event->x - mPadMouseLastX):0;
    			mPadMouseCurY += (abs(Event->y - mPadMouseLastY) > PAD_MOUSE_SENSITIVE_RANGE) ?
    				(Event->y - mPadMouseLastY):0;
                sendEvent = true;
            //    LOGI("mPadMouseCurX:%f mPadMouseCurY:%f",mPadMouseLastX,mPadMouseLastY);
            //     LOGI("mPadMouseLastX:%f mPadMouseLastY:%f",abs(Event->x) ,abs(Event->y));
            if(mPointerController!=NULL){
                 mPointerController->move(Event->x - mPadMouseLastX, Event->y - mPadMouseLastY);
                 mPointerController->unfade(PointerControllerInterface::TRANSITION_IMMEDIATE);
                 LOGI("the move pointer is x=%f y=%f\n",(Event->x - mPadMouseLastX),(Event->y - mPadMouseLastY));
              }
               
            }
            
    		mPadMouseCurX = ((mPadMouseCurX < 0) ? 0 :
    			mPadMouseCurX >= mDisplayWidth ? mDisplayWidth - 1 : mPadMouseCurX);
    		mPadMouseCurY = ((mPadMouseCurY < 0) ? 0 :
    			mPadMouseCurY >= mDisplayHeight ? mDisplayHeight - 1 : mPadMouseCurY);

    		mPadMouseLastX = Event->x;
    		mPadMouseLastY = Event->y;
        
            if(sendEvent){
                /*
                out->x = mPadMouseCurX;
                out->y = mPadMouseCurY;
                mDispatcher->notifyMotion(curTime, INPUT_DEVICE_ID_TOUCH, AINPUT_SOURCE_MOUSE, 0,
                        Event->action, 0, mMetaState, AMOTION_EVENT_EDGE_FLAG_NONE,
                        1, &pointerId, pointerCoords,
                        1, 1, mDownTime);*/
                 
                sendMotion(REMOTE_MOTION_MODE_MOUSE, AMOTION_EVENT_ACTION_MOVE, mPadMouseCurX, mPadMouseCurY, mDownTime);
            }
        }
        else{
            /*
            mDispatcher->notifyMotion(curTime, INPUT_DEVICE_ID_TOUCH, AINPUT_SOURCE_TOUCHSCREEN, 0,
                    Event->action, 0, mMetaState, AMOTION_EVENT_EDGE_FLAG_NONE,
                    1, &pointerId, pointerCoords,
                    1, 1, mDownTime);*/
          
            sendMotion(REMOTE_MOTION_MODE_TOUCH, Event->action, Event->x, Event->y, mDownTime);
        }
               
        break;

    default:
        break;
    }
}

void InputRemote::sendMotion(int32_t motionType, int32_t action, float x, float y, nsecs_t downTime) {
    nsecs_t curTime;
    int32_t source;
    PointerCoords pointerCoords[MAX_POINTERS];
    // Write output coords.
    PointerCoords *out=&pointerCoords[0];
    curTime = systemTime(SYSTEM_TIME_MONOTONIC);
    out->clear();
    PointerProperties pointerId;
    pointerId.clear();
    pointerId.id = 0;
    pointerId.toolType=(REMOTE_MOTION_MODE_TOUCH == motionType)?AMOTION_EVENT_TOOL_TYPE_FINGER:AMOTION_EVENT_TOOL_TYPE_MOUSE;
 
    uint32_t policyFlags=0;
    policyFlags|=POLICY_FLAG_WAKE_DROPPED;
    
    source = (REMOTE_MOTION_MODE_TOUCH == motionType)?AINPUT_SOURCE_TOUCHSCREEN:AINPUT_SOURCE_MOUSE;
    if(mPointerController!=NULL){ 
        float xx, yy;
        mPointerController->getPosition(&xx, &yy);
        out->setAxisValue(AMOTION_EVENT_AXIS_X, xx);
        out->setAxisValue(AMOTION_EVENT_AXIS_Y, yy);
    }else{
        out->setAxisValue(AMOTION_EVENT_AXIS_X, x);
        out->setAxisValue(AMOTION_EVENT_AXIS_Y, y);
   
    }      
  
    NotifyMotionArgs mNotifyArgs(curTime, INPUT_DEVICE_ID_TOUCH, source, policyFlags,
            action, 0, mMetaState,0, AMOTION_EVENT_EDGE_FLAG_NONE,
            1, &pointerId, out,
            1, 1, downTime);
 
    mDispatcher->notifyMotion(&mNotifyArgs);
}

int32_t InputRemote::updateMetaState(int32_t keyCode, bool down, int32_t oldMetaState) {
    int32_t mask;
    switch (keyCode) {
    case AKEYCODE_ALT_LEFT:
        mask = AMETA_ALT_LEFT_ON;
        break;
    case AKEYCODE_ALT_RIGHT:
        mask = AMETA_ALT_RIGHT_ON;
        break;
    case AKEYCODE_SHIFT_LEFT:
        mask = AMETA_SHIFT_LEFT_ON;
        break;
    case AKEYCODE_SHIFT_RIGHT:
        mask = AMETA_SHIFT_RIGHT_ON;
        break;
    case AKEYCODE_SYM:
        mask = AMETA_SYM_ON;
        break;
    default:
        return oldMetaState;
    }

    int32_t newMetaState = down ? oldMetaState | mask : oldMetaState & ~ mask
            & ~ (AMETA_ALT_ON | AMETA_SHIFT_ON);

    if (newMetaState & (AMETA_ALT_LEFT_ON | AMETA_ALT_RIGHT_ON)) {
        newMetaState |= AMETA_ALT_ON;
    }

    if (newMetaState & (AMETA_SHIFT_LEFT_ON | AMETA_SHIFT_RIGHT_ON)) {
        newMetaState |= AMETA_SHIFT_ON;
    }

    return newMetaState;
}

// --- InputRemoteThread ---
InputRemoteThread::InputRemoteThread(const sp<InputRemote>& remoter) :
        Thread(/*canCallJava*/ true), mRemoter(remoter) {
}

InputRemoteThread::~InputRemoteThread() {
}

/*
    1) loop: if returns true, it will be called again if requestExit() wasn't called.
    2) once: if returns false, the thread will exit.
*/
bool InputRemoteThread::threadLoop() {
    return mRemoter->loopOnce();
    //return true;
}

// --- RemoteSocket ---
RemoteSocket::RemoteSocket(const char *socketNames){
    mSocketFd = -1;
    mSocketName = socketNames;
}

RemoteSocket::RemoteSocket(int socketFd){
    mSocketFd = socketFd;
    mSocketName = NULL;
}

RemoteSocket::~RemoteSocket(){
    if (mSocketName && mSocketFd > -1) {
        close(mSocketFd);
        mSocketFd = -1;
    }
}

bool RemoteSocket::requestConnect(){    
    if (!mSocketName && mSocketFd == -1) {
        LOGE("Failed to start unbound listener");
        errno = EINVAL;
        return false;
    } else if (mSocketName) {
    /*
        if ((mSocketFd = android_get_control_socket(mSocketName)) < 0) {
            LOGE("connecting...,obtain file descriptor socket '%s' failed: %s", mSocketName, strerror(errno));
            return false;
        }*/

        struct sockaddr_un address;
        int sockfd;
        int len;
        if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            LOGE("create socket '%s' failed: %s", mSocketName, strerror(errno));
            return false;
        }

        address.sun_family = AF_UNIX;
        snprintf(address.sun_path, sizeof(address.sun_path), ANDROID_SOCKET_DIR"/%s", mSocketName);
        len = sizeof (address);
        if(connect (sockfd, (struct sockaddr *)&address, len) == -1){
            LOGE("connect to server: '%s' failed: %s", mSocketName, strerror(errno));
            return false;
        }

        mSocketFd = sockfd;
    }
    return true;
}

void RemoteSocket::requestDisconnect() {
    LOGI("disconnecting...");

    if (mSocketName && mSocketFd > -1) {
        close(mSocketFd);
        mSocketFd = -1;
    }
}

int RemoteSocket::readx(void *_buf, int count){
    char *buf = (char *)_buf;
    int n = 0, r;
    if ((mSocketFd < 0) || (count < 0)) return -1;
    while (n < count) {
        r = read(mSocketFd, buf + n, count - n);
        if (r < 0) {
            if (errno == EINTR) continue;
            LOGE("read error: %s\n", strerror(errno));
            return -1;
        }
        if (r == 0) {
            LOGI("eof\n");
            return 0; /* EOF */
        }
        n += r;
    }
    return n;
}

int RemoteSocket::writex(const void *_buf, int count) {
    const char *buf = (const char *)_buf;
    int n = 0, r;
    if ((mSocketFd < 0) || (count < 0)) return -1;
    while (n < count) {
        r = write(mSocketFd, buf + n, count - n);
        if (r < 0) {
            if (errno == EINTR) continue;
            LOGE("write error: %s\n", strerror(errno));
            return -1;
        }
        n += r;
    }
    return 0;
}

} // namespace android

