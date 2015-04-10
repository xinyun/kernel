#ifndef __DDP_MEDIA_SOURCE_H__
#define __DDP_MEDIA_SOURCE_H__

#include  "MediaSource.h"
#include  "DataSource.h"
#include  "MediaBufferGroup.h"
#include  "MetaData.h"
#include  "OMXCodec.h"
#include  "OMX_Index.h"
#include  "OMX_Core.h"
#include  "OMXClient.h"

namespace android {

#define     DSPshort short  
#define     DSPerr   short  
#define     DSPushort  unsigned short                   /*!< DSP unsigned integer */
#define     GBL_BYTESPERWRD         2                   /*!< Bytes per packed word */
#define     GBL_BITSPERWRD          (GBL_BYTESPERWRD*8) /*!< Bytes per packed word */
#define     GBL_SYNCWRD             ((DSPshort)0x0b77)  /*!< AC-3 frame sync word */
#define     GBL_MAXFSCOD            3                   /*!< Number of defined sample rates */
#define     GBL_MAXDDDATARATE       38                  /*!< Number of defined data rates */
#define     BSI_BSID_STD            8                   /*!< Standard ATSC A/52 bit-stream ID */
#define     BSI_ISDD(bsid)          ((bsid) <= BSI_BSID_STD)
#define     GBL_MAXCHANCFGS         8                   /*!< Maximum number of channel configs */
#define     BSI_BSID_AXE            16                  /*!< Annex E bitstream ID */
#define     BSI_ISDDP(bsid)         ((bsid) <= BSI_BSID_AXE && (bsid) > 10)
#define     BSI_BSID_BITOFFSET      40                  /*!< Used to skip ahead to bit-stream ID */
#define     PTR_HEAD_SIZE           7//20


/*brief Bit Stream Operations module decode-side state variable structure */
typedef struct
{
    DSPshort       *p_pkbuf;           /*!< Pointer to bitstream buffer */
    DSPshort        pkbitptr;          /*!< Bit count within bitstream word */
    DSPshort        pkdata;            /*!< Current bitstream word */
#if defined(DEBUG)
    const DSPshort  *p_start_pkbuf;    /*!< Pointer to beginning of bitstream buffer */
#endif /* defined(DEBUG) */
} BSOD_BSTRM;


const DSPshort gbl_chanary[GBL_MAXCHANCFGS] = { 2, 1, 2, 3, 3, 4, 4, 5 };
/* audio coding modes */
enum { GBL_MODE11=0, GBL_MODE_RSVD=0, GBL_MODE10, GBL_MODE20,
        GBL_MODE30, GBL_MODE21, GBL_MODE31, GBL_MODE22, GBL_MODE32 };

const DSPushort gbl_msktab[] =
{
    0x0000, 0x8000, 0xc000, 0xe000,
    0xf000, 0xf800, 0xfc00, 0xfe00,
    0xff00, 0xff80, 0xffc0, 0xffe0,
    0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff
};


/*! Words per frame table based on sample rate and data rate codes */
const DSPshort gbl_frmsizetab[GBL_MAXFSCOD][GBL_MAXDDDATARATE] =
{
    /* 48kHz */
    {   64, 64, 80, 80, 96, 96, 112, 112,
        128, 128, 160, 160, 192, 192, 224, 224,
        256, 256, 320, 320, 384, 384, 448, 448,
        512, 512, 640, 640, 768, 768, 896, 896,
        1024, 1024, 1152, 1152, 1280, 1280 },
    /* 44.1kHz */
    {   69, 70, 87, 88, 104, 105, 121, 122,
        139, 140, 174, 175, 208, 209, 243, 244,
        278, 279, 348, 349, 417, 418, 487, 488,
        557, 558, 696, 697, 835, 836, 975, 976,
        1114, 1115, 1253, 1254, 1393, 1394 },
    /* 32kHz */
    {   96, 96, 120, 120, 144, 144, 168, 168,
        192, 192, 240, 240, 288, 288, 336, 336,
        384, 384, 480, 480, 576, 576, 672, 672,
        768, 768, 960, 960, 1152, 1152, 1344, 1344,
        1536, 1536, 1728, 1728, 1920, 1920 } 
};

typedef enum ddp_downmix_config_t {
    DDP_OUTMIX_AUTO,
    DDP_OUTMIX_LTRT,
    DDP_OUTMIX_LORO,
    DDP_OUTMIX_EXTERNAL,
    DDP_OUTMIX_STREAM
} ddp_downmix_config_t;

/* compression mode */
enum {  GBL_COMP_CUSTOM_0=0, 
        GBL_COMP_CUSTOM_1, 
        GBL_COMP_LINE, 
        GBL_COMP_RF };

//--------------------------------------------------------------------------------
class DDP_Media_Source : public MediaSource
{
public:
    DDP_Media_Source(void);
    
    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();
    virtual status_t read(MediaBuffer **buffer, const ReadOptions *options = NULL);
    
    virtual int GetSampleRate();
    virtual int GetChNum();
    
    virtual int Get_Stop_ReadBuf_Flag();
    virtual int Set_Stop_ReadBuf_Flag(int StopFlag);
    virtual int MediaSourceRead_buffer(unsigned char *buffer,int size);

protected:
    virtual ~DDP_Media_Source();

private:
    
    bool mStarted;
    sp<MetaData>   mMeta;
    MediaBufferGroup *mGroup;   
    int mSample_rate;
    int mChNum;
    int mFrame_size;
    int mStop_ReadBuf_Flag;
    
    DDP_Media_Source(const DDP_Media_Source &);
    DDP_Media_Source &operator=(const DDP_Media_Source &);
};

//----------------------------------------------------------------------------------
class Aml_OMX_Codec
{
public:
    
    Aml_OMX_Codec(void);
    
    OMXClient             m_OMXClient;
    sp<DDP_Media_Source>  m_OMXMediaSource;
    sp<MediaSource>       m_codec;
    
    int                   read(unsigned char *buf, unsigned *size, int* exit);
    status_t              start();
    void                  pause();
    void                  stop();
    void                  lock_init();
    void                  locked();
    void                  unlocked();
    int                   GetDecBytes();

    int64_t               buf_decode_offset;
    int64_t               buf_decode_offset_pre;
    pthread_mutex_t       lock;

    ~Aml_OMX_Codec();   
};

}

#endif
