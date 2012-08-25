LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	osal_task_comm.c \
	remote_control.c \
	remote_control_stream.c \
	remote_control_local.c \
	remote_sensor_hub.c \
	remote_fb_picture.c \
	remote_screen_cap.cpp \
	
LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	libbinder \
	libskia \
	libui \
	libgui

LOCAL_MODULE:= remote_control

LOCAL_MODULE_TAGS := optional

LOCAL_C_INCLUDES += \
	external/skia/include/core \
	external/skia/include/effects \
	external/skia/include/images \
	external/skia/src/ports \
	external/skia/include/utils

include $(BUILD_EXECUTABLE)
