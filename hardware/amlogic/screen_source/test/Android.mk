# Build the unit tests.
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := screen_source_test

LOCAL_MODULE_TAGS := tests

LOCAL_SRC_FILES := screen_source_test.cpp

LOCAL_SHARED_LIBRARIES := libutils\
                          libhardware

#LOCAL_C_INCLUDES :=

include $(BUILD_EXECUTABLE)