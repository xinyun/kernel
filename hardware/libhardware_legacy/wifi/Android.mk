# Copyright 2006 The Android Open Source Project

ifdef WIFI_DRIVER_MODULE_PATH
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH=\"$(WIFI_DRIVER_MODULE_PATH)\"
endif
ifdef WIFI_DRIVER_MODULE_ARG
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_ARG=\"$(WIFI_DRIVER_MODULE_ARG)\"
endif
ifdef WIFI_DRIVER_MODULE_NAME
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME=\"$(WIFI_DRIVER_MODULE_NAME)\"
endif

ifdef WIFI_AP_DRIVER_MODULE_PATH
LOCAL_CFLAGS += -DWIFI_AP_DRIVER_MODULE_PATH=\"$(WIFI_AP_DRIVER_MODULE_PATH)\"
endif

ifdef WIFI_AP_DRIVER_MODULE_NAME
LOCAL_CFLAGS += -DWIFI_AP_DRIVER_MODULE_NAME=\"$(WIFI_AP_DRIVER_MODULE_NAME)\"
endif

ifdef WIFI_DRIVER_MODULE_PATH_2
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH_2=\"$(WIFI_DRIVER_MODULE_PATH_2)\"
endif
ifdef WIFI_DRIVER_MODULE_ARG_2
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_ARG_2=\"$(WIFI_DRIVER_MODULE_ARG_2)\"
endif
ifdef WIFI_DRIVER_MODULE_NAME_2
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME_2=\"$(WIFI_DRIVER_MODULE_NAME_2)\"
endif
ifdef WIFI_DRIVER_MODULE_PATH_3
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH_3=\"$(WIFI_DRIVER_MODULE_PATH_3)\"
endif
ifdef WIFI_DRIVER_MODULE_ARG_3
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_ARG_3=\"$(WIFI_DRIVER_MODULE_ARG_3)\"
endif
ifdef WIFI_DRIVER_MODULE_NAME_3
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME_3=\"$(WIFI_DRIVER_MODULE_NAME_3)\"
endif
ifdef WIFI_FIRMWARE_LOADER
LOCAL_CFLAGS += -DWIFI_FIRMWARE_LOADER=\"$(WIFI_FIRMWARE_LOADER)\"
endif
ifdef WIFI_DRIVER_FW_PATH_STA
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_STA=\"$(WIFI_DRIVER_FW_PATH_STA)\"
endif
ifdef WIFI_DRIVER_FW_PATH_AP
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_AP=\"$(WIFI_DRIVER_FW_PATH_AP)\"
endif
ifdef WIFI_DRIVER_FW_PATH_P2P
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_P2P=\"$(WIFI_DRIVER_FW_PATH_P2P)\"
endif
ifdef WIFI_DRIVER_FW_PATH_PARAM
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_PARAM=\"$(WIFI_DRIVER_FW_PATH_PARAM)\"
endif

ifeq ($(BOARD_WIFI_VENDOR),realtek)
LOCAL_CFLAGS += -DBOARD_WIFI_REALTEK -DBOARD_WIFI_USE_USB
endif

ifeq ($(BOARD_WLAN_DEVICE), atheros)
LOCAL_CFLAGS += -DBOARD_WIFI_USE_USB -DBOARD_WIFI_ATHEROS
endif

ifeq ($(BOARD_WLAN_DEVICE), mtk)
LOCAL_CFLAGS += -DBOARD_WIFI_USE_USB
endif

ifdef MTK_WLAN_SUPPORT
LOCAL_SRC_FILES += wifi/mtk_wifi.c
else
LOCAL_SRC_FILES += wifi/wifi.c
endif

LOCAL_SHARED_LIBRARIES += libnetutils
