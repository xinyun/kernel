/*
 ** aml_audio.c
 **
 ** This program is designed for TV application. 
 ** author: Wang Zhe
 ** Email: Zhe.Wang@amlogic.com
 **
 */
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h> 
#include <sys/mman.h> 
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include "tinyalsa/asoundlib.h"

#include "android_out.h"
#include "aml_audio.h"

#define LOG_TAG "aml_audio"

#define ANDROID_OUT_BUFFER_SIZE    (2048*8*2)//in byte
#define DDP_OUT_BUFFER_SIZE        (2048*8*2)//in byte
#define DEFAULT_OUT_SAMPLE_RATE    (48000)
#define DEFAULT_IN_SAMPLE_RATE     (48000)
#define PLAYBACK_PERIOD_SIZE       (512)
#define CAPTURE_PERIOD_SIZE        (512)
#define PLAYBACK_PERIOD_COUNT      (4)
#define CAPTURE_PERIOD_COUNT       (4)
#define TEMP_BUFFER_SIZE           (PLAYBACK_PERIOD_SIZE * 4 * 3)

static struct pcm_config pcm_config_out = {
    .channels = 2,
    .rate = DEFAULT_OUT_SAMPLE_RATE,
    .period_size = PLAYBACK_PERIOD_SIZE,
    .period_count = PLAYBACK_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = PLAYBACK_PERIOD_SIZE*PLAYBACK_PERIOD_COUNT,
};

static struct pcm_config pcm_config_in = {
    .channels = 2,
    .rate = DEFAULT_IN_SAMPLE_RATE,
    .period_size = CAPTURE_PERIOD_SIZE,
    .period_count = CAPTURE_PERIOD_COUNT,
    .format = PCM_FORMAT_S16_LE,
    .stop_threshold = CAPTURE_PERIOD_SIZE*CAPTURE_PERIOD_COUNT*10,
};

struct buffer_status {
    short *start_add;
    int size;
    int level;
    unsigned int rd;
    unsigned int wr;
};

struct resample_para {
    unsigned int FractionStep;
    unsigned int SampleFraction;
    short lastsample_left;
    short lastsample_right;
};

struct aml_stream_in {
    pthread_mutex_t lock;
    struct pcm_config config;
    struct pcm *pcm;
    int card;
    int device;
    int standby;
    int resample_request;
    void *resample_temp_buffer;
    struct resample_para resample;
    int max_bytes;
    void *temp_buffer;
    void *write_buffer;
};

struct aml_stream_out {
    pthread_mutex_t lock;
    struct pcm_config config;
    struct pcm *pcm;
    int card;
    int device;
    int standby;
    int max_bytes;
    void *temp_buffer;
    void *read_buffer;
    int output_device;
    int amAudio_OutHandle;
    struct buffer_status playback_buf;
    struct buffer_status read_buf;
    int amaudio_out_read_enable;
};

struct aml_dev {
    struct aml_stream_in in;
    struct aml_stream_out out;
    pthread_t aml_Audio_ThreadID;
    int aml_Audio_ThreadTurnOnFlag;
    int aml_Audio_ThreadExecFlag;
    int has_EQ_lib;
    int has_SRS_lib;
    int output_deviceID;
};

static int HDMI_rawdata_in_enable = 0;
static void *start_temp_buffer = NULL;

static struct aml_dev gmAmlDevice = {
    .in = {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .config = {
            .channels = 2,
            .rate = DEFAULT_IN_SAMPLE_RATE,
            .period_size = CAPTURE_PERIOD_SIZE,
            .period_count = CAPTURE_PERIOD_COUNT,
            .format = PCM_FORMAT_S16_LE,
            .stop_threshold = CAPTURE_PERIOD_SIZE*CAPTURE_PERIOD_COUNT*10,
        },
        .pcm = NULL,
        .card = 0,
        .device = 0,
        .standby = 0,
        .resample_request = 0,
        .resample_temp_buffer = NULL,
        .resample = {
            .FractionStep = 0,
            .SampleFraction = 0,
            .lastsample_left = 0,
            .lastsample_right = 0,
        },
        .max_bytes = 0,
        .temp_buffer = NULL,
        .write_buffer = NULL,
    },

    .out = {
        .lock = PTHREAD_MUTEX_INITIALIZER,
        .config = {
            .channels = 2,
            .rate = DEFAULT_OUT_SAMPLE_RATE,
            .period_size = PLAYBACK_PERIOD_SIZE,
            .period_count = PLAYBACK_PERIOD_COUNT,
            .format = PCM_FORMAT_S16_LE,
            .stop_threshold = PLAYBACK_PERIOD_SIZE*PLAYBACK_PERIOD_COUNT,
        },
        .pcm = NULL,
        .card = 0,
        .device = 0,
        .standby = 0,
        .max_bytes = 0,
        .temp_buffer = NULL,
        .read_buffer = NULL,
        .output_device = 0,
        .amAudio_OutHandle = 0,
        .playback_buf = {
            .start_add = NULL,
            .size = 0,
            .level = 0,
            .rd = 0,
            .wr = 0,
        },
        .read_buf = {
            .start_add = NULL,
            .size = 0,
            .level = 0,
            .rd = 0,
            .wr = 0,
        },
        .amaudio_out_read_enable = 0,
    },

    .aml_Audio_ThreadID = 0,
    .aml_Audio_ThreadTurnOnFlag = 0,
    .aml_Audio_ThreadExecFlag = 0,
    .has_EQ_lib = 0,
    .has_SRS_lib = 0,
    .output_deviceID = 0,
};

struct circle_buffer android_out_buffer = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .start_add = NULL,
    .rd = NULL,
    .wr = NULL,
    .size = 0,
};

struct circle_buffer DDP_out_buffer = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .start_add = NULL,
    .rd = NULL,
    .wr = NULL,
    .size = 0,
};

static struct aml_dev *gpAmlDevice = NULL;
static pthread_mutex_t amaudio_dev_op_mutex = PTHREAD_MUTEX_INITIALIZER;

extern int omx_codec_init(void);
extern void omx_codec_close(void);
extern int I2S_state;

#define AMAUDIO_IN                     "/dev/amaudio2_in"
#define AMAUDIO_OUT                    "/dev/amaudio2_out"

#define AMAUDIO_IOC_MAGIC              'A'                      
#define AMAUDIO_IOC_GET_SIZE           _IOW(AMAUDIO_IOC_MAGIC, 0x00, int)
#define AMAUDIO_IOC_GET_PTR            _IOW(AMAUDIO_IOC_MAGIC, 0x01, int)
#define AMAUDIO_IOC_RESET              _IOW(AMAUDIO_IOC_MAGIC, 0x02, int)
#define AMAUDIO_IOC_UPDATE_APP_PTR     _IOW(AMAUDIO_IOC_MAGIC, 0x03, int)
#define AMAUDIO_IOC_AUDIO_OUT_MODE     _IOW(AMAUDIO_IOC_MAGIC, 0x04, int)
#define AMAUDIO_IOC_MIC_LEFT_GAIN      _IOW(AMAUDIO_IOC_MAGIC, 0x05, int)
#define AMAUDIO_IOC_MIC_RIGHT_GAIN     _IOW(AMAUDIO_IOC_MAGIC, 0x06, int)
#define AMAUDIO_IOC_MUSIC_GAIN         _IOW(AMAUDIO_IOC_MAGIC, 0x07, int)
#define AMAUDIO_IOC_GET_PTR_READ        _IOW(AMAUDIO_IOC_MAGIC, 0x08, int)
#define AMAUDIO_IOC_UPDATE_APP_PTR_READ _IOW(AMAUDIO_IOC_MAGIC, 0x09, int)
#define AMAUDIO_IOC_OUT_READ_ENABLE     _IOW(AMAUDIO_IOC_MAGIC, 0x0a, int)

#define CC_DUMP_SRC_TYPE_INPUT      (0)
#define CC_DUMP_SRC_TYPE_OUTPUT     (1)
#define CC_DUMP_SRC_TYPE_IN_OUT     (2)
#define CC_DUMP_SRC_TYPE_OUT_IN     (3)

static int gDumpDataFlag = 0;
static int gDumpDataFd1 = -1;
static int gDumpDataFd2 = -1;

static void DoDumpData(void *data_buf, int size, int aud_src_type);

inline int GetWriteSpace(char *WritePoint, char *ReadPoint,
        int buffer_size) {
    int bytes;

    if (WritePoint >= ReadPoint) {
        bytes = buffer_size - (WritePoint - ReadPoint);
    } else {
        bytes = ReadPoint - WritePoint;
    }
    return bytes;
}

inline int GetReadSpace(char *WritePoint, char *ReadPoint,
        int buffer_size) {
    int bytes;

    if (WritePoint >= ReadPoint) {
        bytes = WritePoint - ReadPoint;
    } else {
        bytes = buffer_size - (ReadPoint - WritePoint);
    }
    return bytes;
}

inline int write_to_buffer(char *current_pointer, char *buffer,
        int bytes, char *start_buffer, int buffer_size) {
    int left_bytes = start_buffer + buffer_size - current_pointer;

    if (left_bytes >= bytes) {
        memcpy(current_pointer, buffer, bytes);
    } else {
        memcpy(current_pointer, buffer, left_bytes);
        memcpy(start_buffer, buffer + left_bytes, bytes - left_bytes);
    }
    return 0;
}

inline int read_from_buffer(char *current_pointer, char *buffer,
        int bytes, char *start_buffer, int buffer_size) {
    int left_bytes = start_buffer + buffer_size - current_pointer;

    if (left_bytes >= bytes) {
        memcpy(buffer, current_pointer, bytes);
    } else {
        memcpy(buffer, current_pointer, left_bytes);
        memcpy(buffer + left_bytes, start_buffer, bytes - left_bytes);
    }
    return 0;
}

inline void* update_pointer(char *current_pointer, int bytes,
        char *start_buffer, int buffer_size) {
    current_pointer += bytes;
    if (current_pointer >= start_buffer + buffer_size) {
        current_pointer -= buffer_size;
    }
    return current_pointer;
}

//Clip from 16.16 fixed-point to 0.15 fixed-point.
inline static short clip(int x) {
    if (x < -32768) {
        return -32768;
    } else if (x > 32767) {
        return 32767;
    } else {
        return x;
    }
}

static int resampler_init(struct aml_stream_in *in) {
    ALOGD("%s, Init Resampler!\n", __FUNCTION__);

    static const double kPhaseMultiplier = 1L << 28;

    in->resample.FractionStep = (unsigned int) (in->config.rate
            * kPhaseMultiplier / pcm_config_in.rate);
    in->resample.SampleFraction = 0;

    size_t buffer_size = in->config.period_size * 4;
    in->resample_temp_buffer = malloc(buffer_size);
    if (in->resample_temp_buffer == NULL) {
        ALOGE("%s, Malloc resample buffer failed!\n", __FUNCTION__);
        return -1;
    }
    in->max_bytes = (in->config.period_size * pcm_config_in.rate
            / in->config.rate + 1) << 2;
    return 0;
}

static int resample_process(struct aml_stream_in *in, unsigned int in_frame,
        short* input, short* output) {
    unsigned int inputIndex = 0;
    unsigned int outputIndex = 0;
    unsigned int FractionStep = in->resample.FractionStep;

    static const uint32_t kPhaseMask = (1LU << 28) - 1;
    unsigned int frac = in->resample.SampleFraction;
    short lastsample_left = in->resample.lastsample_left;
    short lastsample_right = in->resample.lastsample_right;

    while (inputIndex == 0) {
        *output++ = clip(
                (int) lastsample_left
                        + ((((int) input[0] - (int) lastsample_left)
                                * ((int) frac >> 13)) >> 15));
        *output++ = clip(
                (int) lastsample_right
                        + ((((int) input[1] - (int) lastsample_right)
                                * ((int) frac >> 13)) >> 15));

        frac += FractionStep;
        inputIndex += (frac >> 28);
        frac = (frac & kPhaseMask);
        outputIndex++;
    }

    while (inputIndex < in_frame) {
        *output++ = clip(
                (int) input[2 * inputIndex - 2]
                        + ((((int) input[2 * inputIndex]
                                - (int) input[2 * inputIndex - 2])
                                * ((int) frac >> 13)) >> 15));
        *output++ = clip(
                (int) input[2 * inputIndex - 1]
                        + ((((int) input[2 * inputIndex + 1]
                                - (int) input[2 * inputIndex - 1])
                                * ((int) frac >> 13)) >> 15));

        frac += FractionStep;
        inputIndex += (frac >> 28);
        frac = (frac & kPhaseMask);
        outputIndex++;
    }

    in->resample.lastsample_left = input[2 * in_frame - 2];
    in->resample.lastsample_right = input[2 * in_frame - 1];
    in->resample.SampleFraction = frac;

    return outputIndex;
}

static int tmp_buffer_init(struct circle_buffer *tmp, int buffer_size) {
    struct circle_buffer *buf = tmp;
    pthread_mutex_lock(&buf->lock);

    buf->size = buffer_size;
    buf->start_add = malloc(buffer_size * sizeof(char));
    if (buf->start_add == NULL) {
        ALOGD("%s, Malloc android out buffer error!\n", __FUNCTION__);
        pthread_mutex_unlock(&buf->lock);
        return -1;
    }
    buf->rd = buf->start_add;
    buf->wr = buf->start_add + buf->size / 2;

    pthread_mutex_unlock(&buf->lock);
    return 0;
}

static int tmp_buffer_release(struct circle_buffer *tmp) {
    struct circle_buffer *buf = tmp;
    pthread_mutex_lock(&buf->lock);

    if (buf->start_add != NULL) {
        free(buf->start_add);
        buf->start_add = NULL;
    }
    buf->rd = NULL;
    buf->wr = NULL;
    buf->size = 0;

    pthread_mutex_unlock(&buf->lock);
    return 0;
}

static int tmp_buffer_reset(struct circle_buffer *tmp) {
    struct circle_buffer *buf = tmp;
    buf->rd = buf->wr + buf->size / 2;
    if (buf->rd >= (buf->start_add + buf->size))
        buf->rd -= buf->size;
    return 0;
}

int buffer_write(struct circle_buffer *tmp, char* buffer, size_t bytes) {
    struct circle_buffer *buf = tmp;
    pthread_mutex_lock(&buf->lock);;
    if (buf->start_add == NULL || buf->wr == NULL || buf->wr == NULL || buf->size == 0) {
        ALOGE("%s, Buffer malloc fail!\n", __FUNCTION__);
        pthread_mutex_unlock(&buf->lock);
        return -1;
    }
    size_t write_space = GetWriteSpace(buf->wr, buf->rd, buf->size);
    if (write_space < bytes) {
        //tmp_buffer_reset(buf);
        //ALOGD("%s, no space to write %d bytes to buffer.\n", __FUNCTION__,bytes);
        pthread_mutex_unlock(&buf->lock);
        return -1;
    }
    write_to_buffer(buf->wr, buffer, bytes, buf->start_add, buf->size);
    buf->wr = update_pointer(buf->wr, bytes, buf->start_add, buf->size);
    pthread_mutex_unlock(&buf->lock);
    return bytes;
}

int buffer_read(struct circle_buffer *tmp, char* buffer, size_t bytes) {
    struct circle_buffer *buf = tmp;
    pthread_mutex_lock(&buf->lock);
    if (buf->start_add == NULL || buf->wr == NULL || buf->wr == NULL || buf->size == 0) {
        ALOGE("%s, Buffer malloc fail!\n", __FUNCTION__);
        pthread_mutex_unlock(&buf->lock);
        return -1;
    }
    size_t read_space = GetReadSpace(buf->wr, buf->rd, buf->size);
    if (read_space < bytes) {
        //tmp_buffer_reset(buf);
        //ALOGD("%s, no space to read %d bytes to buffer.\n", __FUNCTION__,bytes);
        pthread_mutex_unlock(&buf->lock);
        return -1;
    }
    read_from_buffer(buf->rd, buffer, bytes, buf->start_add, buf->size);
    buf->rd = update_pointer(buf->rd, bytes, buf->start_add, buf->size);
    pthread_mutex_unlock(&buf->lock);
    return bytes;
}

int GetOutputdevice(void){
    if(gpAmlDevice == NULL) return -1;
    return gpAmlDevice->output_deviceID;
}

static int set_input_stream_sample_rate(unsigned int sr,
        struct aml_stream_in *in) {
    if (check_input_stream_sr(sr) == 0) {
        in->config.rate = sr;
    } else {
        if (sr == 0) {
            in->config.rate = pcm_config_in.rate;
        } else {
            ALOGE("%s, The sample rate (%u) is invalid!\n", __FUNCTION__, sr);
            return -1;
        }
    }
    return 0;
}

static int get_aml_card() {
    int card = -1, err = 0;
    int fd = -1;
    unsigned fileSize = 512;
    char *read_buf = NULL, *pd = NULL;
    static const char * const SOUND_CARDS_PATH = "/proc/asound/cards";
    fd = open(SOUND_CARDS_PATH, O_RDONLY);
    if (fd < 0) {
        ALOGE("ERROR: failed to open config file %s error: %d\n",
                SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }

    read_buf = (char *) malloc(fileSize);
    if (!read_buf) {
        ALOGE("Failed to malloc read_buf");
        close(fd);
        return -ENOMEM;
    }
    memset(read_buf, 0x0, fileSize);
    err = read(fd, read_buf, fileSize);
    if (fd < 0) {
        ALOGE("ERROR: failed to read config file %s error: %d\n",
                SOUND_CARDS_PATH, errno);
        close(fd);
        return -EINVAL;
    }
    pd = strstr(read_buf, "AML");
    card = *(pd - 3) - '0';

OUT:
    free(read_buf);
    close(fd);
    return card;
}

static int alsa_in_open(struct aml_stream_in *in) {
    in->config.channels = pcm_config_in.channels;
    in->config.period_size = pcm_config_in.period_size;
    in->config.period_count = pcm_config_in.period_count;
    in->config.format = pcm_config_in.format;
    in->config.stop_threshold = CAPTURE_PERIOD_SIZE * CAPTURE_PERIOD_COUNT * 4;
    in->standby = 1;
    in->resample_request = 0;
    in->resample_temp_buffer = NULL;
    in->max_bytes = in->config.period_size << 2;

    if (in->config.rate == 0) {
        in->config.rate = pcm_config_in.rate;
    }

    if (in->config.rate != pcm_config_in.rate) {
        in->resample_request = 1;
        int ret = resampler_init(in);
        if (ret < 0) {
            return -1;
        }
    }

    pthread_mutex_lock(&in->lock);
    in->card = get_aml_card();
    ALOGE("pcm in card ID = %d \n", in->card);
    in->pcm = pcm_open(in->card, in->device, PCM_IN, &pcm_config_in);
    if (!pcm_is_ready(in->pcm)) {
        ALOGE("%s, Unable to open PCM device in: %s\n", __FUNCTION__,
                pcm_get_error(in->pcm));
        pcm_close(in->pcm);
        pthread_mutex_unlock(&in->lock);
        return -1;
    }

    in->standby = 0;
    ALOGD("%s, Input device is opened: card(%d), device(%d)\n", __FUNCTION__,
            in->card, in->device);
    pthread_mutex_unlock(&in->lock);
    return 0;
}

static int alsa_in_close(struct aml_stream_in *in) {
    ALOGD("%s, Do input close!\n", __FUNCTION__);

    pthread_mutex_lock(&in->lock);
    if (!in->standby) {
        pcm_close(in->pcm);
        in->pcm = NULL;
        in->standby = 1;
    }
    if (in->resample_request && (in->resample_temp_buffer != NULL)) {
        free(in->resample_temp_buffer);
        in->resample_temp_buffer = NULL;
    }
    pthread_mutex_unlock(&in->lock);
    return 0;
}

static int get_in_framesize(struct aml_stream_in *in) {
    int sample_format = 0;
    if (in->config.format == PCM_FORMAT_S16_LE) {
        sample_format = 2;
    }
    return sample_format * in->config.channels;
}

static int alsa_in_read(struct aml_stream_in *in, void* buffer, size_t bytes) {
    int ret;

    pthread_mutex_lock(&in->lock);
    if (in->standby) {
        pthread_mutex_unlock(&in->lock);
        ALOGD("%s, Input device is closed!\n", __FUNCTION__);
        return 0;
    }
    //if raw data in HDMI-in, no need to resample
    if(GetOutputdevice() == 2){
        in->resample_request == 0;
    }
    
    int output_size = 0;
    if (in->resample_request == 1) {
        ret = pcm_read(in->pcm, in->resample_temp_buffer, bytes);
        if (ret < 0) {
            //wait for next frame
            usleep(bytes * 1000000 / get_in_framesize(in) / in->config.rate);
            pthread_mutex_unlock(&in->lock);
            return ret;
        }

        DoDumpData(in->resample_temp_buffer, bytes, CC_DUMP_SRC_TYPE_INPUT);

        output_size = resample_process(in, bytes >> 2,
                (short *) in->resample_temp_buffer, (short *) buffer) << 2;
    } else {
        ret = pcm_read(in->pcm, buffer, bytes);
        if (ret < 0) {
            //wait for next frame
            usleep(bytes * 1000000 / get_in_framesize(in) / in->config.rate);
            pthread_mutex_unlock(&in->lock);
            return ret;
        }

        DoDumpData(buffer, bytes, CC_DUMP_SRC_TYPE_INPUT);

        output_size = bytes;
    }
    pthread_mutex_unlock(&in->lock);
    return output_size;
}

static int alsa_out_open(struct aml_stream_out *out) {
    out->config.channels = pcm_config_out.channels;
    out->config.rate = pcm_config_out.rate;
    out->config.period_size = pcm_config_out.period_size;
    out->config.period_count = pcm_config_out.period_count;
    out->config.format = pcm_config_out.format;
    out->config.stop_threshold = PLAYBACK_PERIOD_SIZE * PLAYBACK_PERIOD_COUNT;
    out->standby = 1;
    out->max_bytes = out->config.period_size << 2;
    out->amaudio_out_read_enable = 0;

    pthread_mutex_lock(&out->lock);
    out->card = get_aml_card();
    out->pcm = pcm_open(out->card, out->device, PCM_OUT, &pcm_config_out);
    if (!pcm_is_ready(out->pcm)) {
        ALOGE("%s, Unable to open PCM device out: %s\n", __FUNCTION__,
                pcm_get_error(out->pcm));
        pcm_close(out->pcm);
        pthread_mutex_unlock(&out->lock);
        return -1;
    }
    out->standby = 0;
    ALOGD("%s, Output device is opened: card(%d), device(%d)\n", __FUNCTION__,
            out->card, out->device);
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int alsa_out_close(struct aml_stream_out *out) {
    ALOGD("%s, Do output close!\n", __FUNCTION__);

    pthread_mutex_lock(&out->lock);
    if (!out->standby) {
        pcm_close(out->pcm);
        out->pcm = NULL;
        out->standby = 1;
    }
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int get_out_framesize(struct aml_stream_out *out) {
    int sample_format = 0;
    if (out->config.format == PCM_FORMAT_S16_LE)
        sample_format = 2;
    return sample_format * out->config.channels;
}

static int alsa_out_write(struct aml_stream_out *out, void* buffer,
        size_t bytes) {
    int ret;

    pthread_mutex_lock(&out->lock);
    if (out->standby) {
        pthread_mutex_unlock(&out->lock);
        ALOGD("%s, Output device is closed!\n", __FUNCTION__);
        return 0;
    }

    ret = pcm_write(out->pcm, buffer, bytes);
    if (ret < 0) {
        usleep(bytes * 1000000 / get_out_framesize(out) / out->config.rate);
        pthread_mutex_unlock(&out->lock);
        return ret;
    }

    pthread_mutex_unlock(&out->lock);
    return bytes;
}

static int reset_amaudio(struct aml_stream_out *out, int delay_size) {
    struct buffer_status *buf = &out->playback_buf;
    buf->rd = 0;
    buf->wr = 0;
    buf->level = buf->size;
    struct buffer_status *buf1 = &out->read_buf;
    buf1->rd = 0;
    buf1->wr = 0;
    buf1->level = 0;
    int ret = ioctl(out->amAudio_OutHandle, AMAUDIO_IOC_RESET, delay_size);
    if (ret < 0) {
        ALOGE("%s, amaudio reset delay_size error!\n", __FUNCTION__);
        return -1;
    }
    return 0;
}

#define AMAUDIO2_PREENABLE "/sys/class/amaudio2/aml_amaudio2_enable"
static int set_amaudio2_enable(int flag){
    int fd = 0;
	char string[16];	
	fd = open(AMAUDIO2_PREENABLE, O_CREAT | O_RDWR, 0664);
    if (fd < 0) {
        ALOGE("unable to open file %s \n", AMAUDIO2_PREENABLE);
		return -1;
    }
	sprintf(string, "%d", flag);
    write(fd, string, strlen(string));
    close(fd);
	return 0;
}
static int new_audiotrack(struct aml_stream_out *out) {
    int i = 0, ret = 0;
    int dly_tm = 20 * 1000, dly_cnt = 1000; //20s

    pthread_mutex_lock(&out->lock);

    ALOGD("%s, entering...\n", __FUNCTION__);
	set_amaudio2_enable(1);
    ret = new_android_audiotrack();
    if (ret < 0) {
        ALOGE("%s, New an audio track is fail!\n", __FUNCTION__);
        pthread_mutex_unlock(&out->lock);
        return -1;
    }

    while (I2S_state < 10) {
        usleep(dly_tm);

        i++;
        if (i >= dly_cnt) {
            release_android_audiotrack();
            pthread_mutex_unlock(&out->lock);
            ALOGE("%s, Time out error: wait %d ms for waiting I2S ready.\n", __FUNCTION__, i * dly_tm / 1000);
            return -1;
        }
    }
    set_amaudio2_enable(1);
    ALOGD("%s, sucess: wait %d ms for waiting I2S ready.\n", __FUNCTION__, i * dly_tm / 1000);
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int new_audiotrack_nowait(struct aml_stream_out *out) {
    int ret = 0;
    pthread_mutex_lock(&out->lock);
    ALOGD("%s, entering...\n", __FUNCTION__);
	set_amaudio2_enable(1);
    ret = new_android_audiotrack();
    if (ret < 0) {
        ALOGE("%s, New an audio track is fail!\n", __FUNCTION__);
        pthread_mutex_unlock(&out->lock);
        return -1;
    }
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int release_audiotrack(struct aml_stream_out *out) {
    ALOGD("%s, Release audio track!\n", __FUNCTION__);
    pthread_mutex_lock(&out->lock);
    int ret = release_android_audiotrack();
    if (ret < 0) {
        ALOGE("%s, Delete audio track is fail!\n", __FUNCTION__);
    }
	set_amaudio2_enable(0);
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int amaudio_out_open(struct aml_stream_out *out) {

    out->config.channels = pcm_config_out.channels;
    out->config.rate = pcm_config_out.rate;
    out->config.period_size = pcm_config_out.period_size;
    out->config.period_count = pcm_config_out.period_count;
    out->config.format = pcm_config_out.format;

    out->standby = 1;
    out->max_bytes = out->config.period_size << 2;
    out->amaudio_out_read_enable = 0;

    pthread_mutex_lock(&out->lock);
    out->amAudio_OutHandle = -1;
    out->amAudio_OutHandle = open(AMAUDIO_OUT, O_RDWR, 777);
    if (out->amAudio_OutHandle < 0) {
        close(out->amAudio_OutHandle);
        out->amAudio_OutHandle = -1;
        release_android_audiotrack();
        pthread_mutex_unlock(&out->lock);
        ALOGE("%s, The device amaudio_out cant't be opened!\n", __FUNCTION__);
        return -1;
    }

    struct buffer_status *buf = &out->playback_buf;
    buf->size = ioctl(out->amAudio_OutHandle, AMAUDIO_IOC_GET_SIZE);
    buf->start_add = (short*) mmap(NULL, buf->size, PROT_READ | PROT_WRITE,
            MAP_FILE | MAP_SHARED, out->amAudio_OutHandle, 0);
    if (buf->start_add == 0) {
        close(out->amAudio_OutHandle);
        out->amAudio_OutHandle = -1;
        release_android_audiotrack();
        pthread_mutex_unlock(&out->lock);
        ALOGE("%s, Error create mmap!\n", __FUNCTION__);
        return -1;
    }

    struct buffer_status *buf1 = &out->read_buf;
    buf1->size = buf->size;

    out->standby = 0;
    pthread_mutex_unlock(&out->lock);
    ALOGD("%s, Amaudio device is opened!\n", __FUNCTION__);
    return 0;
}

static int amaudio_out_close(struct aml_stream_out *out) {
    ALOGD("%s, Do amaudio device close!\n", __FUNCTION__);
    pthread_mutex_lock(&out->lock);
    if (out->amAudio_OutHandle > 0) {
        close(out->amAudio_OutHandle);
        out->amAudio_OutHandle = -1;
        munmap(out->playback_buf.start_add, out->playback_buf.size);
    }
    pthread_mutex_unlock(&out->lock);
    return 0;
}

static int amaudio_out_write(struct aml_stream_out *out, void* buffer,
        size_t bytes) {
    int block = bytes >> 6 << 6; //align 2^6
    short *in = (short *) buffer;
    short *left = NULL;
    short *right = NULL;
    int k = 0, i = 0;
    struct buffer_status *buf = &out->playback_buf;

    pthread_mutex_lock(&out->lock);

    //get rd ptr, and calculate write space
    buf->rd = ioctl(out->amAudio_OutHandle, AMAUDIO_IOC_GET_PTR);
    buf->level = buf->size - ((buf->size + buf->wr - buf->rd) % buf->size);

    if(buf->level <= block){
        ALOGD("Reset amaudio: buf->level=%x,buf->rd = %x,buf->wr=%x\n",buf->level,buf->rd,buf->wr);
        pthread_mutex_unlock(&out->lock);
        return -1;
    }

    left = &(buf->start_add[buf->wr / 2 + 0]);
    right = &(buf->start_add[buf->wr / 2 + 16]);

    for (k = 0; k < block; k += 64) {
        for (i = 0; i < 16; i++) {
            *left++ = *in++;
            *right++ = *in++;
        }
        left += 16;
        right += 16;
    }

    // update the write pointer and write space
    buf->wr = (buf->wr + block)%buf->size;
    buf->level = buf->size - ((buf->size + buf->wr - buf->rd) % buf->size);
    ioctl(out->amAudio_OutHandle, AMAUDIO_IOC_UPDATE_APP_PTR, buf->wr);
    pthread_mutex_unlock(&out->lock);

    return block;
}

int amaudio_out_read_enable(int enable) {
    int ret;

    if (gpAmlDevice == NULL) {
        ALOGE("%s, aml audio is not open, must open it first!\n", __FUNCTION__);
        return -1;
    }

    int OutHandle = gpAmlDevice->out.amAudio_OutHandle;
    if (OutHandle < 0) {
        ALOGE("%s, amaudio out handle error!\n", __FUNCTION__);
        return -1;
    }

    pthread_mutex_lock(&gpAmlDevice->out.lock);
    gpAmlDevice->out.amaudio_out_read_enable = enable;
    reset_amaudio(&gpAmlDevice->out, 64 * 32 * 2);
    ioctl(OutHandle, AMAUDIO_IOC_OUT_READ_ENABLE, enable);
    ALOGD("%s, out read enable :%d!\n", __FUNCTION__, enable);
    pthread_mutex_unlock(&gpAmlDevice->out.lock);
    return 0;
}

static int amaudio_out_read(struct aml_stream_out *out, void* buffer,
        size_t bytes) {
    int block = bytes >> 6 << 6; //align 2^6
    char *in = (char *) buffer;
    struct buffer_status *buf = &out->read_buf;

    pthread_mutex_lock(&out->lock);
    
    buf->wr = ioctl(out->amAudio_OutHandle, AMAUDIO_IOC_GET_PTR_READ);
    buf->level = (buf->size + buf->wr - buf->rd) % buf->size;

    if(buf->level < block){
        usleep(5000);
        pthread_mutex_unlock(&out->lock);
        ALOGD("buf->level=%d,buf->rd = %d,buf->wr=%d\n",buf->level,buf->rd,buf->wr);
        return 0;
    }
    
    int Ret = read(out->amAudio_OutHandle, (char *) buffer, block);
    if (Ret != block) {
        ALOGE("%s, amAudio2 read from hardware buffer failed!\n", __FUNCTION__);
        pthread_mutex_unlock(&out->lock);
        return -1;
    }

    // update the read pointer
    buf->rd += block;
    buf->rd %= buf->size;
    buf->level = (buf->size + buf->wr - buf->rd) % buf->size;

    ioctl(out->amAudio_OutHandle, AMAUDIO_IOC_UPDATE_APP_PTR_READ, buf->rd);
    pthread_mutex_unlock(&out->lock);
    return block;
}

static int malloc_buffer(struct aml_dev *device) {
    void *buffer = NULL;
    struct aml_stream_in *in = &device->in;
    struct aml_stream_out *out = &device->out;

    buffer = malloc(TEMP_BUFFER_SIZE);
    if (buffer == NULL) {
        ALOGD("%s, Malloc temp buffer failed!\n", __FUNCTION__);
        return -1;
    }
    start_temp_buffer = buffer;
    in->write_buffer = buffer;
    out->read_buffer = buffer;

    in->temp_buffer = malloc(in->max_bytes);
    if (in->temp_buffer == NULL) {
        ALOGD("%s, Malloc input temp buffer failed!\n", __FUNCTION__);
        return -1;
    }

    out->temp_buffer = malloc(pcm_config_out.period_size<<2);
    if (out->temp_buffer == NULL) {
        ALOGD("%s, Malloc output temp buffer failed!\n", __FUNCTION__);
        return -1;
    }

    return 0;
}

static int release_buffer(struct aml_dev *device) {
    struct aml_stream_in *in = &device->in;
    struct aml_stream_out *out = &device->out;

    if (start_temp_buffer != NULL) {
        free(start_temp_buffer);
        start_temp_buffer = NULL;
        in->write_buffer = NULL;
        out->read_buffer = NULL;
    }
    if (in->temp_buffer != NULL) {
        free(in->temp_buffer);
        in->temp_buffer = NULL;
    }
    if (out->temp_buffer != NULL) {
        free(out->temp_buffer);
        out->temp_buffer = NULL;
    }
    return 0;
}

static int audio_effect_release() {
    unload_EQ_lib();
    unload_SRS_lib();
    return 0;
}

static int set_output_deviceID(int deviceID) {
    int ret;

    if (gpAmlDevice == NULL) {
        ALOGE("%s, aml audio is not open, must open it first!\n", __FUNCTION__);
        return -1;
    }

    gpAmlDevice->output_deviceID = deviceID;
    ALOGE("%s, set output device ID: %d!\n", __FUNCTION__,deviceID);
    return 0;
}

static int aml_device_init(struct aml_dev *device) {
    int ret;

    ALOGD("%s, start to open Devices!\n", __FUNCTION__);

    //Malloc temp buffer for audiotrak out
    ret = tmp_buffer_init(&android_out_buffer,ANDROID_OUT_BUFFER_SIZE);
    if (ret < 0) {
        ALOGE("%s, malloc temp buffer error!\n", __FUNCTION__);
        goto error1;
    }

    if(HDMI_rawdata_in_enable == 1){
        ret = tmp_buffer_init(&DDP_out_buffer,DDP_OUT_BUFFER_SIZE);
        if(ret < 0){
            ALOGE("%s, malloc ddp buffer failed!\n", __FUNCTION__);
            goto error2;
        }
        ret = omx_codec_init();
        if(ret < 0){
            ALOGE("%s, start ddp omx codec failed!\n", __FUNCTION__);
            goto error3;
        }
        device->out.output_device = CC_OUT_USE_ANDROID;
    }

    //open input device of tinyalsa
    ret = alsa_in_open(&device->in);
    if (ret < 0) {
        ALOGE("%s, open alsa in device open error!\n", __FUNCTION__);
        goto error4;
    }

    //Malloc temp buffer for input and output
    ret = malloc_buffer(device);
    if (ret < 0) {
        ALOGE("%s, malloc buffer error!\n", __FUNCTION__);
        goto error5;
    }
    
    if (device->out.output_device == CC_OUT_USE_ALSA) {
        set_output_deviceID(0);
        //open output device of tinyalsa
        ret = alsa_out_open(&device->out);
        if (ret < 0) {
            ALOGE("%s, open alsa out device open error!\n", __FUNCTION__);
            goto error6;
        }
    } else if(device->out.output_device == CC_OUT_USE_AMAUDIO){
        set_output_deviceID(0);
        //open output device of amaudio
        ret = new_audiotrack(&device->out);
        if (ret < 0) {
            ALOGE("%s, new audiotrack error!\n", __FUNCTION__);
            goto error6;
        }
        ret = amaudio_out_open(&device->out);
        if (ret < 0) {
            release_audiotrack(&device->out);
            ALOGE("%s, open amaudio out device error!\n", __FUNCTION__);
            goto error6;
        }
    }else if(device->out.output_device == CC_OUT_USE_ANDROID){
        //open output device of android
        if(HDMI_rawdata_in_enable == 0){
            set_output_deviceID(1);
        }else{
            set_output_deviceID(2);
        }
        ret = new_audiotrack_nowait(&device->out);
        if (ret < 0) {
            ALOGE("%s, open android out device error!\n", __FUNCTION__);
            goto error6;
        }
    }
    
    //EQ lib load and init EQ
    ret = load_EQ_lib();
    if (ret < 0) {
        ALOGE("%s, Load EQ lib fail!\n", __FUNCTION__);
        device->has_EQ_lib = 0;
    } else {
        ret = HPEQ_init();
        if (ret < 0) {
            device->has_EQ_lib = 0;
        } else {
            device->has_EQ_lib = 1;
        }
        HPEQ_enable(1);
    }

    //load srs lib and init it. SRS is behand resampling, so sample rate is as default sr.
    ret = load_SRS_lib();
    if (ret < 0) {
        ALOGE("%s, Load EQ lib fail!\n", __FUNCTION__);
        device->has_SRS_lib = 0;
    } else {
        ret = srs_init(device->out.config.rate);
        if (ret < 0) {
            device->has_SRS_lib = 0;
        } else {
            device->has_SRS_lib = 1;
        }
    }

    ALOGD("%s, exiting...\n", __FUNCTION__);
    return 0;
    
error6:
    release_buffer(device);
error5:
    alsa_in_close(&device->in);
error4:
    if(HDMI_rawdata_in_enable == 1){
        omx_codec_close();
    }
error3:
    if(HDMI_rawdata_in_enable == 1){
        tmp_buffer_release(&DDP_out_buffer);
    }
error2:
    tmp_buffer_release(&android_out_buffer);
error1:
    return ret;

}

static int aml_device_close(struct aml_dev *device) {
    struct aml_stream_in *in = &device->in;
    struct aml_stream_out *out = &device->out;

    alsa_in_close(in);

    if (out->output_device == CC_OUT_USE_ALSA) {
        alsa_out_close(out);
    } else if(out->output_device == CC_OUT_USE_AMAUDIO){
        amaudio_out_close(out);
        release_audiotrack(out);
    } else if(out->output_device == CC_OUT_USE_ANDROID){
        release_audiotrack(out);
    }

    if(HDMI_rawdata_in_enable == 1) {   
        omx_codec_close();
        tmp_buffer_release(&DDP_out_buffer);
    }
    
    tmp_buffer_release(&android_out_buffer);
    release_buffer(device);
    audio_effect_release();
    return 0;
}

static int gUSBCheckLastFlag = 0;
static int gUSBCheckFlag = 0;
static pthread_mutex_t usb_check_flag_mutex = PTHREAD_MUTEX_INITIALIZER;

int getUSBCheckFlag() {
    int tmp_val = 0;

    pthread_mutex_lock(&usb_check_flag_mutex);

    tmp_val = gUSBCheckFlag;

    pthread_mutex_unlock(&usb_check_flag_mutex);

    return tmp_val;
}

static void USB_check(struct aml_stream_out *out) {
    pthread_mutex_lock(&usb_check_flag_mutex);

    gUSBCheckFlag = ((GetUsbAudioCheckFlag()
            & (0x01 << CC_USB_AUDIO_PLAYBACK_BIT_IND))
            >> CC_USB_AUDIO_PLAYBACK_BIT_IND);
    if (gUSBCheckLastFlag == gUSBCheckFlag) {
        pthread_mutex_unlock(&usb_check_flag_mutex);
        return;
    }

    gUSBCheckLastFlag = gUSBCheckFlag;
    if (gUSBCheckFlag == 1) {
        if(out->output_device == CC_OUT_USE_AMAUDIO){
            amaudio_out_close(out);
            set_output_deviceID(1);
        }else if(out->output_device == CC_OUT_USE_ALSA){
            alsa_out_close(out);
            new_audiotrack_nowait(out);
            set_output_deviceID(1);
        }
        ALOGI("%s, USB audio playback device is in.\n", __FUNCTION__);
    } else if (gUSBCheckFlag == 0) {
        if(out->output_device == CC_OUT_USE_AMAUDIO){
            amaudio_out_open(out);
            set_output_deviceID(0);
        }else if(out->output_device == CC_OUT_USE_ALSA){
            release_audiotrack(out);
            alsa_out_open(out);
            set_output_deviceID(0);
        }
        ALOGI("%s, USB audio playback device is out.\n", __FUNCTION__);
    }

    pthread_mutex_unlock(&usb_check_flag_mutex);
    return;
}

static void* aml_audio_threadloop(void *data) {
    struct aml_stream_in *in = NULL;
    struct aml_stream_out *out = NULL;
    int output_size = 0;

    ALOGD("%s, entering...\n", __FUNCTION__);

    if (gpAmlDevice == NULL) {
        ALOGE("%s, gpAmlDevice is NULL\n", __FUNCTION__);
        return ((void *) 0);
    }

    in = &gpAmlDevice->in;
    out = &gpAmlDevice->out;
    out->max_bytes = pcm_config_out.period_size << 2;
    
    gpAmlDevice->aml_Audio_ThreadExecFlag = 1;
    ALOGD("%s, set aml_Audio_ThreadExecFlag as 1.\n", __FUNCTION__);

    if (gpAmlDevice->out.output_device == CC_OUT_USE_AMAUDIO) {
        int delay_size = 64 * 32 * 2; //less than 10.67*2 ms delay
        reset_amaudio(out, delay_size);
    }

    gUSBCheckLastFlag = 0;
    gUSBCheckFlag = 0;

    while (gpAmlDevice != NULL && gpAmlDevice->aml_Audio_ThreadTurnOnFlag) {
        //exit threadloop
        if (gpAmlDevice->aml_Audio_ThreadTurnOnFlag == 0) {
            ALOGD("%s, aml_Audio_ThreadTurnOnFlag is 0 break now.\n",
                    __FUNCTION__);
            break;
        }
        if (GetWriteSpace((char *) in->write_buffer, (char *) out->read_buffer,
                TEMP_BUFFER_SIZE) > in->max_bytes) {
            output_size = alsa_in_read(in, in->temp_buffer,
                    in->config.period_size * 4);
            if (output_size < 0) {
                //ALOGE("%s, alsa_in_read fail!\n", __FUNCTION__);
            } else {
                if (gpAmlDevice->has_EQ_lib && (GetOutputdevice() == 0)) {
                    HPEQ_process((short *) in->temp_buffer,
                            (short *) in->temp_buffer, output_size >> 2);
                }
                write_to_buffer((char *) in->write_buffer,
                        (char *) in->temp_buffer, output_size,
                        (char *) start_temp_buffer, TEMP_BUFFER_SIZE);
                in->write_buffer = update_pointer((char *) in->write_buffer,
                        output_size, (char *) start_temp_buffer,
                        TEMP_BUFFER_SIZE);
            }

        }
        USB_check(out);
        if (GetReadSpace((char *) in->write_buffer, (char *) out->read_buffer,
                TEMP_BUFFER_SIZE) > out->max_bytes) {
            read_from_buffer((char *) out->read_buffer,
                    (char *) out->temp_buffer, out->max_bytes,
                    (char *) start_temp_buffer, TEMP_BUFFER_SIZE);

            output_size = out->max_bytes;
            if (gpAmlDevice->has_SRS_lib && (GetOutputdevice() == 0)) {
                output_size = srs_process((short *) out->temp_buffer,
                        (short *) out->temp_buffer, out->max_bytes >> 2);
            }            
            if (getUSBCheckFlag() == 0 && gpAmlDevice->out.output_device == CC_OUT_USE_ALSA) {
                output_size = alsa_out_write(out, out->temp_buffer,
                        output_size);
            } else if (getUSBCheckFlag() == 0 && gpAmlDevice->out.output_device == CC_OUT_USE_AMAUDIO) {
                output_size = amaudio_out_write(out, out->temp_buffer,
                        output_size);
                if(output_size < 0){
                    amaudio_out_close(out);
                    set_output_deviceID(0);
                    amaudio_out_open(out);
                    reset_amaudio(out, 4096);
                }
            } else if (getUSBCheckFlag() != 0 || gpAmlDevice->out.output_device == CC_OUT_USE_ANDROID){
                output_size = buffer_write(&android_out_buffer, out->temp_buffer,
                        output_size);
            }
            
            if (output_size < 0) {
                //ALOGE("%s, out_write fail! bytes = %d \n", __FUNCTION__, output_size);
            } else {
                out->read_buffer = update_pointer((char *) out->read_buffer,
                        output_size, (char *) start_temp_buffer,
                        TEMP_BUFFER_SIZE);
                DoDumpData(out->temp_buffer, output_size,
                        CC_DUMP_SRC_TYPE_OUTPUT);
                memset(out->temp_buffer,0,output_size);
            }
        }
    }

    if (gpAmlDevice != NULL) {
        gpAmlDevice->aml_Audio_ThreadTurnOnFlag = 0;
        ALOGD("%s, set aml_Audio_ThreadTurnOnFlag as 0.\n", __FUNCTION__);
        gpAmlDevice->aml_Audio_ThreadExecFlag = 0;
        ALOGD("%s, set aml_Audio_ThreadExecFlag as 0.\n", __FUNCTION__);
    }

    ALOGD("%s, exiting...\n", __FUNCTION__);
    return ((void *) 0);
}

static int clrDevice(struct aml_dev *device) {
    memset((void *) device, 0, sizeof(struct aml_dev));

    device->in.config.channels = 2;
    device->in.config.rate = DEFAULT_IN_SAMPLE_RATE;
    device->in.config.period_size = CAPTURE_PERIOD_SIZE;
    device->in.config.period_count = CAPTURE_PERIOD_COUNT;
    device->in.config.format = PCM_FORMAT_S16_LE;

    device->out.config.channels = 2;
    device->out.config.rate = DEFAULT_OUT_SAMPLE_RATE;
    device->out.config.period_size = PLAYBACK_PERIOD_SIZE;
    device->out.config.period_count = PLAYBACK_PERIOD_COUNT;
    device->out.config.format = PCM_FORMAT_S16_LE;

    return 0;
}

int aml_audio_open(unsigned int sr, int input_device, int output_device) {
    pthread_attr_t attr;
    struct sched_param param;
    int ret;

    ALOGD("%s, sr = %d, output_device = %d\n", __FUNCTION__, sr, output_device);

    aml_audio_close();

    pthread_mutex_lock(&amaudio_dev_op_mutex);

    gpAmlDevice = &gmAmlDevice;
    clrDevice(gpAmlDevice);

    ret = set_input_stream_sample_rate(sr, &gpAmlDevice->in);
    if (ret < 0) {
        ALOGE("%s, set_input_stream_sample_rate fail!\n", __FUNCTION__);
        clrDevice(gpAmlDevice);
        gpAmlDevice = NULL;
        pthread_mutex_unlock(&amaudio_dev_op_mutex);
        return -1;
    }

    gpAmlDevice->out.output_device = output_device;
    if (gpAmlDevice->out.output_device == CC_OUT_USE_ALSA) {
        ALOGD("%s,Use tinyalsa as output device!\n", __FUNCTION__);
    } else if (gpAmlDevice->out.output_device == CC_OUT_USE_AMAUDIO) {
        ALOGD("%s, Use amlogic amaudio as output device!\n", __FUNCTION__);
    } else if (gpAmlDevice->out.output_device == CC_OUT_USE_ANDROID) {
        ALOGD("%s, Use amlogic android as output device!\n", __FUNCTION__);
    } else {
        ALOGE("%s, Unkown output device, use default amaudio\n", __FUNCTION__);
        gpAmlDevice->out.output_device = CC_OUT_USE_AMAUDIO;
    }

    gpAmlDevice->in.device = input_device;
    
    ret = aml_device_init(gpAmlDevice);
    if (ret < 0) {
        ALOGE("%s, Devices fail opened!\n", __FUNCTION__);
        clrDevice(gpAmlDevice);
        gpAmlDevice = NULL;
        pthread_mutex_unlock(&amaudio_dev_op_mutex);
        return -1;
    }

    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam(&attr, &param);
    gpAmlDevice->aml_Audio_ThreadTurnOnFlag = 1;
    ALOGD("%s, set aml_Audio_ThreadTurnOnFlag as 1.\n", __FUNCTION__);
    gpAmlDevice->aml_Audio_ThreadExecFlag = 0;
    ALOGD("%s, set aml_Audio_ThreadExecFlag as 0.\n", __FUNCTION__);
    ret = pthread_create(&gpAmlDevice->aml_Audio_ThreadID, &attr,
            &aml_audio_threadloop, NULL);
    pthread_attr_destroy(&attr);
    if (ret != 0) {
        ALOGE("%s, Create thread fail!\n", __FUNCTION__);
        aml_device_close(gpAmlDevice);
        clrDevice(gpAmlDevice);
        gpAmlDevice = NULL;
        pthread_mutex_unlock(&amaudio_dev_op_mutex);
        return -1;
    }

    pthread_mutex_unlock(&amaudio_dev_op_mutex);

    ALOGD("%s, exiting...\n", __FUNCTION__);
    return 0;
}

int aml_audio_close(void) {
    int i = 0, tmp_timeout_count = 1000;

    ALOGD("%s, gpAmlDevice = %p\n", __FUNCTION__, gpAmlDevice);

    pthread_mutex_lock(&amaudio_dev_op_mutex);

    if (gpAmlDevice != NULL) {
        gpAmlDevice->aml_Audio_ThreadTurnOnFlag = 0;
        ALOGD("%s, set aml_Audio_ThreadTurnOnFlag as 0.\n", __FUNCTION__);
        while (1) {
            if (gpAmlDevice->aml_Audio_ThreadExecFlag == 0) {
                break;
            }

            if (i >= tmp_timeout_count) {
                break;
            }

            i++;

            usleep(10 * 1000);
        }

        if (i >= tmp_timeout_count) {
            ALOGE("%s, we have try %d times, but the aml audio thread's exec flag is still(%d)!!!\n",
                    __FUNCTION__, tmp_timeout_count,
                    gpAmlDevice->aml_Audio_ThreadExecFlag);
        } else {
            ALOGD("%s, kill aml audio thread success after try %d times.\n",
                    __FUNCTION__, i);
        }

        pthread_join(gpAmlDevice->aml_Audio_ThreadID, NULL);
        gpAmlDevice->aml_Audio_ThreadID = 0;

        aml_device_close(gpAmlDevice);
        clrDevice(gpAmlDevice);
        gpAmlDevice = NULL;

        ALOGD("%s, aml audio close success.\n", __FUNCTION__);
    }

    pthread_mutex_unlock(&amaudio_dev_op_mutex);
    return 0;
}

int set_rawdata_in_enable(unsigned int sr, int input_device){
    int ret = 0;
    aml_audio_close();
    HDMI_rawdata_in_enable = 1;
    ret = aml_audio_open(sr, input_device, CC_OUT_USE_ANDROID);
    return ret;
}

int set_rawdata_in_disable(unsigned int sr, int input_device, int output_device){
    int ret = 0;
    aml_audio_close();
    HDMI_rawdata_in_enable = 0;
    ret = aml_audio_open(sr, input_device, output_device);
    return ret;
}

int check_input_stream_sr(unsigned int sr) {
    if (sr >= 8000 && sr <= 48000) {
        return 0;
    }
    return -1;
}

int set_output_mode(int mode) {
    if (gpAmlDevice == NULL) {
        ALOGE("%s, aml audio is not open, must open it first!\n", __FUNCTION__);
        return -1;
    }

    if (mode < CC_OUT_MODE_DIRECT || mode > CC_OUT_MODE_DIRECT_MIX) {
        ALOGE("%s, mode error: mode = %d!\n", __FUNCTION__, mode);
        return -1;
    }

    int OutHandle = gpAmlDevice->out.amAudio_OutHandle;
    if (OutHandle < 0) {
        ALOGE("%s, amaudio out handle error!\n", __FUNCTION__);
        return -1;
    }

    pthread_mutex_lock(&gpAmlDevice->out.lock);
    ioctl(OutHandle, AMAUDIO_IOC_AUDIO_OUT_MODE, mode);
    pthread_mutex_unlock(&gpAmlDevice->out.lock);
    return 0;
}

int set_music_gain(int gain) {
    if (gpAmlDevice == NULL) {
        ALOGE("%s, aml audio is not open, must open it first!\n", __FUNCTION__);
        return -1;
    }

    int OutHandle = gpAmlDevice->out.amAudio_OutHandle;
    if (OutHandle < 0) {
        ALOGE("%s, amaudio out handle error!\n", __FUNCTION__);
        return -1;
    }

    pthread_mutex_lock(&gpAmlDevice->out.lock);
    if (gain > 256) {
        gain = 256;
    }
    if (gain < 0) {
        gain = 0;
    }
    ioctl(OutHandle, AMAUDIO_IOC_MUSIC_GAIN, gain);
    ALOGD("%s, music gain :%d!\n", __FUNCTION__, gain);
    pthread_mutex_unlock(&gpAmlDevice->out.lock);
    return 0;
}

int set_left_gain(int left_gain) {
    if (gpAmlDevice == NULL) {
        ALOGE("%s, aml audio is not open, must open it first!\n", __FUNCTION__);
        return -1;
    }

    int OutHandle = gpAmlDevice->out.amAudio_OutHandle;
    if (OutHandle < 0) {
        ALOGE("%s, amaudio out handle error!\n", __FUNCTION__);
        return -1;
    }
    pthread_mutex_lock(&gpAmlDevice->out.lock);
    if (left_gain > 256) {
        left_gain = 256;
    }
    if (left_gain < 0) {
        left_gain = 0;
    }
    ioctl(OutHandle, AMAUDIO_IOC_MIC_LEFT_GAIN, left_gain);
    ALOGD("%s, left mic gain :%d!\n", __FUNCTION__, left_gain);
    pthread_mutex_unlock(&gpAmlDevice->out.lock);
    return 0;
}

int set_right_gain(int right_gain) {
    if (gpAmlDevice == NULL) {
        ALOGE("%s, aml audio is not open, must open it first!\n", __FUNCTION__);
        return -1;
    }
    int OutHandle = gpAmlDevice->out.amAudio_OutHandle;
    if (OutHandle < 0) {
        ALOGE("%s, amaudio out handle error!\n", __FUNCTION__);
        return -1;
    }
    pthread_mutex_lock(&gpAmlDevice->out.lock);
    if (right_gain > 256) {
        right_gain = 256;
    }
    if (right_gain < 0) {
        right_gain = 0;
    }
    ioctl(OutHandle, AMAUDIO_IOC_MIC_RIGHT_GAIN, right_gain);
    ALOGD("%s, right mic gain :%d!\n", __FUNCTION__, right_gain);
    pthread_mutex_unlock(&gpAmlDevice->out.lock);
    return 0;
}

int SetDumpDataFlag(int tmp_flag) {
    int tmp_val;
    tmp_val = gDumpDataFlag;
    gDumpDataFlag = tmp_flag;
    return tmp_val;
}

int GetDumpDataFlag(void) {
    int tmp_val = 0;
    tmp_val = gDumpDataFlag;
    return tmp_val;
}

static void DoDumpData(void *data_buf, int size, int aud_src_type) {
    int tmp_type = 0;
    char prop_value[PROPERTY_VALUE_MAX] = {
        0 };
    char file_path_01[PROPERTY_VALUE_MAX] = {
        0 };
    char file_path_02[PROPERTY_VALUE_MAX] = {
        0 };

    if (GetDumpDataFlag() == 0) {
        return;
    }

    memset(prop_value, '\0', PROPERTY_VALUE_MAX);
    property_get("audio.dumpdata.en", prop_value, "null");
    if (strcasecmp(prop_value, "null") == 0
            || strcasecmp(prop_value, "0") == 0) {
        if (gDumpDataFd1 >= 0) {
            close(gDumpDataFd1);
            gDumpDataFd1 = -1;
        }
        if (gDumpDataFd2 >= 0) {
            close(gDumpDataFd2);
            gDumpDataFd2 = -1;
        }

        return;
    }

    tmp_type = CC_DUMP_SRC_TYPE_INPUT;
    property_get("audio.dumpdata.src", prop_value, "null");
    if (strcasecmp(prop_value, "null") == 0
            || strcasecmp(prop_value, "input") == 0) {
        tmp_type = CC_DUMP_SRC_TYPE_INPUT;
    } else if (strcasecmp(prop_value, "output") == 0) {
        tmp_type = CC_DUMP_SRC_TYPE_OUTPUT;
    } else if (strcasecmp(prop_value, "input,output") == 0) {
        tmp_type = CC_DUMP_SRC_TYPE_IN_OUT;
    } else if (strcasecmp(prop_value, "output,input") == 0) {
        tmp_type = CC_DUMP_SRC_TYPE_OUT_IN;
    }

    if (tmp_type == CC_DUMP_SRC_TYPE_INPUT
            || tmp_type == CC_DUMP_SRC_TYPE_OUTPUT) {
        if (tmp_type != aud_src_type) {
            return;
        }
    }

    memset(file_path_01, '\0', PROPERTY_VALUE_MAX);
    property_get("audio.dumpdata.path", file_path_01, "null");
    if (strcasecmp(file_path_01, "null") == 0) {
        file_path_01[0] = '\0';
    }

    if (tmp_type == CC_DUMP_SRC_TYPE_IN_OUT
            || tmp_type == CC_DUMP_SRC_TYPE_OUT_IN) {
        memset(file_path_02, '\0', PROPERTY_VALUE_MAX);
        property_get("audio.dumpdata.path2", file_path_02, "null");
        if (strcasecmp(file_path_02, "null") == 0) {
            file_path_02[0] = '\0';
        }
    }

    if (gDumpDataFd1 < 0 && file_path_01[0] != '\0') {
        if (access(file_path_01, 0) == 0) {
            gDumpDataFd1 = open(file_path_01, O_RDWR | O_SYNC);
            if (gDumpDataFd1 < 0) {
                ALOGE("%s, Open device file \"%s\" error: %s.\n", __FUNCTION__,
                        file_path_01, strerror(errno));
            }
        } else {
            gDumpDataFd1 = open(file_path_01, O_WRONLY | O_CREAT | O_EXCL,
                    S_IRUSR | S_IWUSR);
            if (gDumpDataFd1 < 0) {
                ALOGE("%s, Create device file \"%s\" error: %s.\n",
                        __FUNCTION__, file_path_01, strerror(errno));
            }
        }
    }

    if (gDumpDataFd2 < 0 && file_path_02[0] != '\0'
            && (tmp_type == CC_DUMP_SRC_TYPE_IN_OUT
                    || tmp_type == CC_DUMP_SRC_TYPE_OUT_IN)) {
        if (access(file_path_02, 0) == 0) {
            gDumpDataFd2 = open(file_path_02, O_RDWR | O_SYNC);
            if (gDumpDataFd2 < 0) {
                ALOGE("%s, Open device file \"%s\" error: %s.\n", __FUNCTION__,
                        file_path_02, strerror(errno));
            }
        } else {
            gDumpDataFd2 = open(file_path_02, O_WRONLY | O_CREAT | O_EXCL,
                    S_IRUSR | S_IWUSR);
            if (gDumpDataFd2 < 0) {
                ALOGE("%s, Create device file \"%s\" error: %s.\n",
                        __FUNCTION__, file_path_02, strerror(errno));
            }
        }
    }

    if (tmp_type == CC_DUMP_SRC_TYPE_IN_OUT) {
        if (aud_src_type == CC_DUMP_SRC_TYPE_INPUT && gDumpDataFd1 >= 0) {
            write(gDumpDataFd1, data_buf, size);
        } else if (aud_src_type == CC_DUMP_SRC_TYPE_OUTPUT
                && gDumpDataFd2 >= 0) {
            write(gDumpDataFd2, data_buf, size);
        }
    } else if (tmp_type == CC_DUMP_SRC_TYPE_OUT_IN) {
        if (aud_src_type == CC_DUMP_SRC_TYPE_OUTPUT && gDumpDataFd1 >= 0) {
            write(gDumpDataFd1, data_buf, size);
        } else if (aud_src_type == CC_DUMP_SRC_TYPE_INPUT
                && gDumpDataFd2 >= 0) {
            write(gDumpDataFd2, data_buf, size);
        }
    } else {
        if (gDumpDataFd1 >= 0) {
            write(gDumpDataFd1, data_buf, size);
        }
    }
}
