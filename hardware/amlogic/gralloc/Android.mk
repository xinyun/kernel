# 
# Copyright (C) 2010 ARM Limited. All rights reserved.
# 
# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH := $(call my-dir)

#framebuffer apis
include $(CLEAR_VARS)
ifeq ($(TARGET_USE_TRIPLE_FB_BUFFERS), true)
LOCAL_CFLAGS += -DENABLE_FB_TRIPLE_BUFFERS
endif

ifeq ($(TARGET_EXTERNAL_DISPLAY),true)
ifeq ($(TARGET_SINGLE_EXTERNAL_DISPLAY_USE_FB1),true)
LOCAL_CFLAGS += -DSINGLE_EXTERNAL_DISPLAY_USE_FB1
endif
endif

#LOCAL_CFLAGS += -DDEBUG_EXTERNAL_DISPLAY_ON_PANEL
LOCAL_PRELINK_MODULE := false
LOCAL_SRC_FILES := framebuffer.cpp
LOCAL_MODULE := libfbcnf
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libsync 
include $(BUILD_SHARED_LIBRARY)


# HAL module implemenation, not prelinked and stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_MODULE := gralloc.amlogic
#LOCAL_MODULE_TAGS := optional

#LOCAL_CFLAGS:= -DLOG_TAG=\"gralloc\" -DGRALLOC_32_BITS -DSTANDARD_LINUX_SCREEN
LOCAL_CFLAGS:= -DLOG_TAG=\"gralloc\" -DGRALLOC_32_BITS  -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq ($(USING_ION),true)
SHARED_MEM_LIBS := libion
LOCAL_C_INCLUDES += system/core/include/
LOCAL_CFLAGS+= -DUSING_ION=1

else
SHARED_MEM_LIBS := libUMP
LOCAL_C_INCLUDES += $(LOCAL_PATH)/ump
endif

ifeq ($(TARGET_USE_TRIPLE_FB_BUFFERS), true)
LOCAL_CFLAGS += -DENABLE_FB_TRIPLE_BUFFERS
endif

#LOCAL_CFLAGS += -DDEBUG_EXTERNAL_DISPLAY_ON_PANEL

LOCAL_SHARED_LIBRARIES := liblog libcutils libGLESv1_CM $(SHARED_MEM_LIBS) libutils libhardware libfbcnf

LOCAL_SRC_FILES := \
	gralloc_module.cpp \
	alloc_device.cpp \
	framebuffer_device.cpp

#LOCAL_CFLAGS+= -DMALI_VSYNC_EVENT_REPORT_ENABLE
include $(BUILD_SHARED_LIBRARY)
