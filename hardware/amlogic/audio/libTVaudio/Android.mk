LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libTVaudio

LOCAL_SHARED_LIBRARIES := libcutils libutils libtinyalsa libdl \
			libmedia libbinder libusbhost libstagefright libmedia_native \

LOCAL_C_INCLUDES := \
    external/tinyalsa/include \
    frameworks/av/include/media/stagefright \
    frameworks/native/include/media/openmax \
    $(LOCAL_PATH)/ \
    $(LOCAL_PATH)/audio \

LOCAL_SRC_FILES := \
    audio/aml_audio.c \
    audio/audio_effect_control.c \
    audio/android_out.cpp \
    audio/audio_amaudio.cpp \
    audio/audio_usb_check.cpp \
    audio/amaudio_main.cpp \
    audio/DDP_media_source.cpp \

LOCAL_CFLAGS := -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)
