/*
 audio_usb_check.c
 check usb audio device.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#include <asm/byteorder.h>
#include <usbhost/usbhost.h>

#include <cutils/log.h>

#define LOG_TAG "audio_usb_check"

#include "audio_usb_check.h"

static unsigned int gUsbAudioCheckFlag = 0;
static pthread_mutex_t usb_audio_chk_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

static int SetUsbAudioCheckFlag(int tmp_flag) {
    int tmp_val = 0;

    pthread_mutex_lock(&usb_audio_chk_flag_mutex);

    tmp_val = gUsbAudioCheckFlag;
    gUsbAudioCheckFlag = tmp_flag;

    pthread_mutex_unlock(&usb_audio_chk_flag_mutex);

    return tmp_val;
}

int GetUsbAudioCheckFlag() {
    int tmp_val = 0;

    pthread_mutex_lock(&usb_audio_chk_flag_mutex);

    tmp_val = gUsbAudioCheckFlag;

    pthread_mutex_unlock(&usb_audio_chk_flag_mutex);

    return tmp_val;
}

#define USB_AUDIO_SOURCE_PATH "/proc/asound/usb_audio_info"

static int tryAccessFile(char *file_path, int dly_tm, int dly_cnt) {
    int i = 0;

    ALOGD("%s, entering...\n", __FUNCTION__);

    while (access(file_path, R_OK) < 0) {
        if (i >= dly_cnt) {
            break;
        }

        i++;

        usleep(dly_tm);
    }

    ALOGD("%s, try %d times.\n", __FUNCTION__, i);

    if (i >= dly_cnt) {
        return -1;
    }

    return 0;
}

static int getUsbAudioStreamPath(char file_path[], int card) {
    char tmp_str[32] = {
        '\0' };

    strcpy(file_path, "/proc/asound/card");
    sprintf(tmp_str, "%ld", card);
    strcat(file_path, tmp_str);
    strcat(file_path, "/stream0");

    return 0;
}

static int parseUsbAudioSource(char *file_path, unsigned int *usbid,
        unsigned int *card, unsigned int *device) {
    FILE *fp = NULL;

    fp = fopen(file_path, "r");
    if (fp == NULL) {
        ALOGE("%s, open %s ERROR(%s)!!\n", __FUNCTION__, file_path,
                strerror(errno));
        return -1;
    }

    fscanf(fp, "%x %d %d", usbid, card, device);

    fclose(fp);

    return 0;
}

static int parseUsbAudioStream(char *file_path, char *key_str, int card) {
    int i = 0;
    char tmp_buf[1024] = {
        '\0' };

    ALOGD("%s, entering...\n", __FUNCTION__);

    FILE *fp = NULL;

    fp = fopen(file_path, "r");
    if (fp == NULL) {
        ALOGE("%s, open %s ERROR(%s)!!\n", __FUNCTION__, file_path,
                strerror(errno));
        return -1;
    }

    while (fgets(tmp_buf, sizeof(tmp_buf), fp) != NULL) {
        if (strstr(tmp_buf, key_str) != NULL) {
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);

    return -1;
}

#define CC_MAX_USB_AUDIO_DEV_CNT (64)
static char gCurUSBAudioDevName[CC_MAX_USB_AUDIO_DEV_CNT][256];
static int gCurUSBAudioDevCnt = 0;

static int usb_device_added(const char *devname, void* client_data) {
    int i = 0, dly_tm = 100 * 1000, dly_cnt = 50;
    int tmp_ret, tmp_flag;
    unsigned int usbid, card, dev_num;
    unsigned int usb_add_id;
    char file_path[256] = {
        '\0' };
    struct usb_descriptor_header* desc;
    struct usb_descriptor_iter iter;

    if (devname == NULL) {
        ALOGE("%s, devname is NULL!\n", __FUNCTION__);
        return 0;
    }

    struct usb_device *device = usb_device_open(devname);
    if (!device) {
        ALOGE("%s, usb_device_open device(%s) failed!\n", __FUNCTION__,
                devname);
        return 0;
    }

    const usb_device_descriptor* deviceDesc = usb_device_get_device_descriptor(
            device);

    uint16_t vendorId = usb_device_get_vendor_id(device);
    uint16_t productId = usb_device_get_product_id(device);
    usb_add_id = (vendorId << 16) | productId;

    usb_descriptor_iter_init(device, &iter);

    while ((desc = usb_descriptor_iter_next(&iter)) != NULL) {
        if (desc->bDescriptorType == USB_DT_INTERFACE) {
            struct usb_interface_descriptor *interface =
                    (struct usb_interface_descriptor *) desc;
            if (interface->bInterfaceClass == 1) {
                ALOGD("%s, find one usb audio device.\n", __FUNCTION__);

                tmp_ret = tryAccessFile(USB_AUDIO_SOURCE_PATH, 100 * 1000, 50);
                if (tmp_ret < 0) {
                    ALOGE("%s, parse usb audio source error.\n", __FUNCTION__);
                    usb_device_close(device);
                    return 0;
                }

                while (1) {
                    usbid = 0;
                    card = 0;
                    dev_num = 0;
                    tmp_ret = parseUsbAudioSource(USB_AUDIO_SOURCE_PATH, &usbid,
                            &card, &dev_num);
                    if (tmp_ret < 0) {
                        ALOGE("%s, parse usb audio source error.\n",
                                __FUNCTION__);
                        usb_device_close(device);
                        return 0;
                    }

                    if (usb_add_id == usbid) {
                        break;
                    }

                    if (i >= dly_cnt) {
                        ALOGE("%s, parse usb id error.\n", __FUNCTION__);
                        usb_device_close(device);
                        return 0;
                    }

                    i++;

                    usleep(dly_tm);
                }

                if (gCurUSBAudioDevCnt < CC_MAX_USB_AUDIO_DEV_CNT) {
                    strcpy(gCurUSBAudioDevName[gCurUSBAudioDevCnt], devname);
                    gCurUSBAudioDevCnt += 1;
                }

                tmp_ret = getUsbAudioStreamPath(file_path, card);
                tmp_ret = tryAccessFile(file_path, 100 * 1000, 50);

                tmp_flag = 0;
                tmp_ret = parseUsbAudioStream(file_path, "Playback:", card);
                if (tmp_ret == 0) {
                    ALOGD(
                            "%s, usb audio device (0x%x) has Playback function.\n",
                            __FUNCTION__, usbid);
                    tmp_flag |= 1 << CC_USB_AUDIO_PLAYBACK_BIT_IND;
                    SetUsbAudioCheckFlag(tmp_flag);
                }

                tmp_ret = parseUsbAudioStream(file_path, "Capture:", card);
                if (tmp_ret == 0) {
                    ALOGD("%s, usb audio device (0x%x) has Capture function.\n",
                            __FUNCTION__, usbid);
                    tmp_flag |= 1 << CC_USB_AUDIO_CAPTURE_BIT_IND;
                    SetUsbAudioCheckFlag(tmp_flag);
                }

                usb_device_close(device);
                return 0;
            }
        } else if (desc->bDescriptorType == USB_DT_ENDPOINT) {
            struct usb_endpoint_descriptor *endpoint =
                    (struct usb_endpoint_descriptor *) desc;

        }
    }

    usb_device_close(device);
    return 0;
}

static int usb_device_removed(const char *devname, void* client_data) {
    unsigned int i = 0, j = 0, usb_rm_id;

    if (devname == NULL) {
        ALOGE("%s, devname is NULL!\n", __FUNCTION__);
        return 0;
    }

    for (i = 0; i < gCurUSBAudioDevCnt; i++) {
        if (strcmp(gCurUSBAudioDevName[i], devname) == 0) {
            break;
        }
    }

    if (i == gCurUSBAudioDevCnt) {
        ALOGD("%s, %s is not usb audio device.\n", __FUNCTION__, devname);
        return 0;
    }

    ALOGD("%s, %s is usb audio device.\n", __FUNCTION__, devname);

    strcpy(gCurUSBAudioDevName[i], "");
    for (j = i; j < gCurUSBAudioDevCnt - 1; j++) {
        strcpy(gCurUSBAudioDevName[j], gCurUSBAudioDevName[j + 1]);
    }

    SetUsbAudioCheckFlag(0);

    return 0;
}

static void* monitorUsbHostBusMain(void* data) {
    ALOGD("%s, entering...\n", __FUNCTION__);

    struct usb_host_context* context = usb_host_init();
    if (!context) {
        ALOGE("%s, usb_host_init failed!\n", __FUNCTION__);
        return NULL;
    }
    // this will never return
    usb_host_run(context, usb_device_added, usb_device_removed, NULL, NULL);

    return NULL;
}

#define CC_ERR_THREAD_ID    (0)

static pthread_t gMonitorUsbHostBusThreadID = CC_ERR_THREAD_ID;

int createMonitorUsbHostBusThread() {
    int ret = 0;
    pthread_attr_t attr;
    struct sched_param param;

    ALOGD("%s, entering...\n", __FUNCTION__);

    if (gMonitorUsbHostBusThreadID != CC_ERR_THREAD_ID) {
        ALOGD("%s, we have create monitor usb host thread.\n", __FUNCTION__);
        return 0;
    }

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &param);
    if ((ret = pthread_create(&gMonitorUsbHostBusThreadID, &attr,
            monitorUsbHostBusMain, NULL)) < 0) {
        pthread_attr_destroy(&attr);
        gMonitorUsbHostBusThreadID = CC_ERR_THREAD_ID;
        ALOGE("%s, pthread_create error.\n", __FUNCTION__);
        return -1;
    }

    pthread_attr_destroy(&attr);

    ALOGD("%s, exiting...\n", __FUNCTION__);
    return ret;
}
