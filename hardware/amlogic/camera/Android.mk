LOCAL_PATH:= $(call my-dir)

CAMERA_HAL_SRC := \
	CameraHal_Module.cpp \
	CameraHal.cpp \
	CameraHalUtilClasses.cpp \
	AppCallbackNotifier.cpp \
	ANativeWindowDisplayAdapter.cpp \
	CameraProperties.cpp \
	MemoryManager.cpp \
	Encoder_libjpeg.cpp \
	SensorListener.cpp  \
	NV12_resize.c

CAMERA_COMMON_SRC:= \
	CameraParameters.cpp \
	ExCameraParameters.cpp \
	CameraHalCommon.cpp

CAMERA_V4L_SRC:= \
	BaseCameraAdapter.cpp \
	V4LCameraAdapter/V4LCameraAdapter.cpp

CAMERA_USB_FMT_SRC:= \
	usb_fmt.cpp
CAMERA_UTILS_SRC:= \
	utils/ErrorUtils.cpp \
	utils/MessageQueue.cpp \
	utils/Semaphore.cpp \
	utils/util.cpp

CAMERA_HAL_VERTURAL_CAMERA_SRC:= \
	vircam/VirtualCamHal.cpp \
	vircam/AppCbNotifier.cpp \
	vircam/V4LCamAdpt.cpp

CAMERA_HAL_JPEG_SRC:=\
	mjpeg/jpegdec.c \
	mjpeg/colorspaces.c
	
CAMERA_HAL_HW_JPEGENC_SRC:=\
	jpegenc_hw/jpegenc.cpp

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	$(CAMERA_HAL_SRC) \
	$(CAMERA_V4L_SRC) \
	$(CAMERA_COMMON_SRC) \
	$(CAMERA_UTILS_SRC) \
	$(CAMERA_HAL_JPEG_SRC) \
	$(CAMERA_USB_FMT_SRC)

ifeq ($(BOARD_HAVE_HW_JPEGENC),true)
LOCAL_SRC_FILES += $(CAMERA_HAL_HW_JPEGENC_SRC)
endif

ifneq (,$(wildcard hardware/amlogic/gralloc))
GRALLOC_DIR := hardware/amlogic/gralloc 
else 
GRALLOC_DIR := hardware/libhardware/modules/gralloc
endif

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/ \
    $(LOCAL_PATH)/utils \
    $(LOCAL_PATH)/inc/V4LCameraAdapter \
    frameworks/native/include/ui \
    frameworks/native/include/utils \
    frameworks/base/include/media/stagefright \
    external/jhead/ \
    external/jpeg/ \
    frameworks/native/include/media/hardware \
    system/core/include/ion \
    $(LOCAL_PATH)/inc/mjpeg/ \
    $(GRALLOC_DIR) \
    system/core/include/utils \
    external/libyuv/files/include/ \

LOCAL_STATIC_LIBRARIES := \
                         libyuv_static \

ifeq ($(BOARD_HAVE_HW_JPEGENC),true)
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/inc/jpegenc_hw/
endif

LOCAL_C_INCLUDES_VIRCAM := \
    $(LOCAL_PATH)/vircam/inc


LOCAL_SHARED_LIBRARIES:= \
    libui \
    libbinder \
    libutils \
    libcutils \
    libcamera_client \
    libexif \
    libjpeg \
    libgui \
    libion

LOCAL_CFLAGS := -fno-short-enums -DCOPY_IMAGE_BUFFER

CAMHAL_GIT_VERSION="$(shell cd $(LOCAL_PATH);git log | grep commit -m 1 | cut -d' ' -f 2)"
CAMHAL_GIT_UNCOMMIT_FILE_NUM=$(shell cd $(LOCAL_PATH);git diff | grep +++ -c)
CAMHAL_LAST_CHANGED="$(shell cd $(LOCAL_PATH);git log | grep Date -m 1)"
CAMHAL_BUILD_TIME=" $(shell date)"
CAMHAL_BUILD_NAME=" $(shell echo ${LOGNAME})"
CAMHAL_BRANCH_NAME="$(shell cd $(LOCAL_PATH);git branch -a | sed -n '/'*'/p')"
CAMHAL_BUILD_MODE=$(shell echo ${TARGET_BUILD_VARIANT})
CAMHAL_HOSTNAME="$(shell hostname)"
CAMHAL_IP="$(shell ifconfig eth0|grep -oE '([0-9]{1,3}\.?){4}'|head -n 1)"
CAMHAL_PATH="$(shell pwd)"

LOCAL_CFLAGS+=-DHAVE_VERSION_INFO
LOCAL_CFLAGS+=-DCAMHAL_GIT_VERSION=\"${CAMHAL_GIT_VERSION}${CAMHAL_GIT_DIRTY}\"
LOCAL_CFLAGS+=-DCAMHAL_BRANCH_NAME=\"${CAMHAL_BRANCH_NAME}\"
LOCAL_CFLAGS+=-DCAMHAL_LAST_CHANGED=\"${CAMHAL_LAST_CHANGED}\"
LOCAL_CFLAGS+=-DCAMHAL_BUILD_TIME=\"${CAMHAL_BUILD_TIME}\"
LOCAL_CFLAGS+=-DCAMHAL_BUILD_NAME=\"${CAMHAL_BUILD_NAME}\"
LOCAL_CFLAGS+=-DCAMHAL_GIT_UNCOMMIT_FILE_NUM=${CAMHAL_GIT_UNCOMMIT_FILE_NUM}
LOCAL_CFLAGS+=-DCAMHAL_HOSTNAME=\"${CAMHAL_HOSTNAME}\"
LOCAL_CFLAGS+=-DCAMHAL_IP=\"${CAMHAL_IP}\"
LOCAL_CFLAGS+=-DCAMHAL_PATH=\"${CAMHAL_PATH}\"

ifeq ($(CAMHAL_BUILD_MODE),user)
    LOCAL_CFLAGS += -DCAMHAL_USER_MODE
endif
ifeq ($(BOARD_HAVE_FRONT_CAM),true)
    LOCAL_CFLAGS += -DAMLOGIC_FRONT_CAMERA_SUPPORT
endif

ifeq ($(BOARD_HAVE_BACK_CAM),true)
    LOCAL_CFLAGS += -DAMLOGIC_BACK_CAMERA_SUPPORT
endif

ifeq ($(IS_CAM_NONBLOCK),true)
LOCAL_CFLAGS += -DAMLOGIC_CAMERA_NONBLOCK_SUPPORT
endif

ifeq ($(BOARD_USE_USB_CAMERA),true)
    LOCAL_CFLAGS += -DAMLOGIC_USB_CAMERA_SUPPORT
#descrease the number of camera captrue frames,and let skype run smoothly
ifeq ($(BOARD_USB_CAMREA_DECREASE_FRAMES), true)
	LOCAL_CFLAGS += -DAMLOGIC_USB_CAMERA_DECREASE_FRAMES
endif
ifeq ($(BOARD_USBCAM_IS_TWOCH),true)
    LOCAL_CFLAGS += -DAMLOGIC_TWO_CH_UVC
endif
else
    ifeq ($(BOARD_HAVE_MULTI_CAMERAS),true)
        LOCAL_CFLAGS += -DAMLOGIC_MULTI_CAMERA_SUPPORT
    endif
    ifeq ($(BOARD_HAVE_FLASHLIGHT),true)
        LOCAL_CFLAGS += -DAMLOGIC_FLASHLIGHT_SUPPORT
    endif
endif

ifeq ($(BOARD_ENABLE_VIDEO_SNAPSHOT),true)
    LOCAL_CFLAGS += -DAMLOGIC_ENABLE_VIDEO_SNAPSHOT
endif

ifeq ($(BOARD_HAVE_VIRTUAL_CAMERA),true)
    LOCAL_CFLAGS += -DAMLOGIC_VIRTUAL_CAMERA_SUPPORT

	ifneq ($(IS_VIRTUAL_CAMERA_NONBLOCK),false)
		LOCAL_CFLAGS += -DAMLOGIC_VCAM_NONBLOCK_SUPPORT
	endif

    LOCAL_SRC_FILES+= \
		$(CAMERA_HAL_VERTURAL_CAMERA_SRC)
	LOCAL_C_INCLUDES += \
		$(LOCAL_C_INCLUDES_VIRCAM)
endif

ifeq ($(BOARD_HAVE_HW_JPEGENC),true)
    LOCAL_CFLAGS += -DAMLOGIC_HW_JPEGENC
endif

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE:= camera.amlogic
LOCAL_MODULE_TAGS:= optional

#include $(BUILD_HEAPTRACKED_SHARED_LIBRARY)
include $(BUILD_SHARED_LIBRARY)
