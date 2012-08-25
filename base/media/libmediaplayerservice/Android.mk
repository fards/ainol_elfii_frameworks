LOCAL_PATH:= $(call my-dir)

#
# libmediaplayerservice
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    MediaRecorderClient.cpp     \
    MediaPlayerService.cpp      \
    MetadataRetrieverClient.cpp \
    TestPlayerStub.cpp          \
    MidiMetadataRetriever.cpp   \
    MidiFile.cpp                \
    StagefrightPlayer.cpp       \
    StagefrightRecorder.cpp     \
    AmlPlayerMetadataRetriever.cpp

ifeq ($(BUILD_WITH_AMLOGIC_PLAYER),true)
    LOCAL_SRC_FILES +=AmSuperPlayer.cpp
    LOCAL_SRC_FILES +=AmlogicPlayer.cpp
    LOCAL_SRC_FILES +=AmlogicPlayerRender.cpp
    LOCAL_SRC_FILES +=AmlogicPlayerStreamSource.cpp
    LOCAL_SRC_FILES +=AmlogicPlayerStreamSourceListener.cpp
endif

LOCAL_SHARED_LIBRARIES :=     		\
	libcutils             			\
	libutils              			\
	libbinder             			\
	libvorbisidec         			\
	libsonivox            			\
	libmedia              			\
	libcamera_client      			\
	libandroid_runtime    			\
	libstagefright        			\
	libstagefright_omx    			\
	libstagefright_foundation       \
	libgui                          \
	libdl

LOCAL_STATIC_LIBRARIES := \
        libstagefright_nuplayer                 \
        libstagefright_rtsp                     \

LOCAL_C_INCLUDES :=                                                 \
	$(JNI_H_INCLUDE)                                                \
	$(call include-path-for, graphics corecg)                       \
	$(TOP)/frameworks/base/include/media/stagefright/openmax \
	$(TOP)/frameworks/base/media/libstagefright/include             \
	$(TOP)/frameworks/base/media/libstagefright/rtsp                \
        $(TOP)/external/tremolo/Tremolo \



ifeq ($(BUILD_WITH_AMLOGIC_PLAYER),true)
AMPLAYER_APK_DIR=$(TOP)/packages/amlogic/LibPlayer/
LOCAL_C_INCLUDES +=\
        $(AMPLAYER_APK_DIR)/amplayer/player/include     \
        $(AMPLAYER_APK_DIR)/amplayer/control/include    \
        $(AMPLAYER_APK_DIR)/amadec/include      \
        $(AMPLAYER_APK_DIR)/amcodec/include     \
        $(AMPLAYER_APK_DIR)/amavutils/include     \
        $(AMPLAYER_APK_DIR)/amffmpeg/

LOCAL_SHARED_LIBRARIES += libui
LOCAL_SHARED_LIBRARIES +=libamplayer libamavutils
LOCAL_CFLAGS += -DBUILD_WITH_AMLOGIC_PLAYER=1
endif



LOCAL_MODULE:= libmediaplayerservice

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

