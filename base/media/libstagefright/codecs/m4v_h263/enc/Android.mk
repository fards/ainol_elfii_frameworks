LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    M4vH263Encoder.cpp \
    src/bitstream_io.cpp \
    src/combined_encode.cpp \
    src/datapart_encode.cpp \
    src/findhalfpel.cpp \
    src/fastcodemb.cpp \
    src/fastquant.cpp \
    src/me_utils.cpp \
    src/mp4enc_api.cpp \
    src/rate_control.cpp \
    src/motion_est.cpp \
    src/motion_comp.cpp \
    src/sad.cpp \
    src/vlc_encode.cpp \
    src/vop.cpp


ifeq ($(ARCH_ARM_HAVE_NEON),true)
LOCAL_SRC_FILES += src/dct_neon.s src/sad_neon.s src/sad_halfpel_neon.s src/fastidct_neon.s src/dct4sad.c ColorConverter_neon.s
else
LOCAL_SRC_FILES += src/dct.cpp src/fastidct.cpp src/sad_halfpel.cpp
endif

LOCAL_MODULE := libstagefright_m4vh263enc

LOCAL_CFLAGS := \
    -DBX_RC \
    -DOSCL_IMPORT_REF= -DOSCL_UNUSED_ARG= -DOSCL_EXPORT_REF=
ifeq ($(ARCH_ARM_HAVE_NEON),true)
LOCAL_CFLAGS += -DASM_OPT
endif

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/src \
    $(LOCAL_PATH)/include \
    $(TOP)/frameworks/base/include/media/stagefright/openmax \
    $(TOP)/frameworks/base/media/libstagefright/include

include $(BUILD_STATIC_LIBRARY)
