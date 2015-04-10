#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <android/log.h>
#include <cutils/properties.h>
#include <pthread.h>

#include "DDP_media_source.h"
#include "aml_audio.h"

extern struct circle_buffer android_out_buffer;
extern struct circle_buffer DDP_out_buffer;

namespace android {

#define LOG_TAG "DDP_Media_Source"

//-------------------------------DDP Extractor----------------------------------

#define DOLBY_DDPDEC_51_MULTICHANNEL_ENDPOINT 

#ifdef DOLBY_DDPDEC51_MULTICHANNEL_GENERIC

static char cMaxChansProp[]="media.tegra.max.out.channels";
static char cChanMapProp[]="media.tegra.out.channel.map";

#elif defined DOLBY_DDPDEC_51_MULTICHANNEL_ENDPOINT

#ifndef DOLBY_DS1_UDC
static char cEndpointProp[]="media.multich.support.info";
#else
static char cEndpointProp[]="dolby.audio.sink.info";
#endif
static char cInfoFrameByte[]="dolby.audio.sink.channel.allocation";

const char* endpoints[] = { "hdmi2",
                            "hdmi6",
                            "hdmi8",
                            "headset",
                            "speaker",
                            //"bluetooth",//or other supported audio output device
                            "invalid" //this is the default value we will get if not set
                          };
const int EndpointConfig[][4] = {{DDP_OUTMIX_LTRT, GBL_COMP_LINE, 0, 2},        //HDMI2
                                  {DDP_OUTMIX_STREAM, GBL_COMP_LINE, 0, 6},      //HDMI6
                                  {DDP_OUTMIX_STREAM, GBL_COMP_LINE, 0, 6},      //HDMI8//as ddpdec51 supports max 6 ch.
                                  {DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //HEADSET
                                  {DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //SPEAKER
                                  //{DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //BLUETOOTH or other audio device setting
                                  {DDP_OUTMIX_LTRT, GBL_COMP_RF, 1, 2},          //INVALID/DEFAULT
                                 };
#endif 

static DSPerr bsod_init(DSPshort *  p_pkbuf, DSPshort pkbitptr,BSOD_BSTRM *p_bstrm)
{
    p_bstrm->p_pkbuf = p_pkbuf;
    p_bstrm->pkbitptr = pkbitptr;
    p_bstrm->pkdata = *p_pkbuf;
#if defined(DEBUG)
    p_bstrm->p_start_pkbuf = p_pkbuf;
#endif /* defined(DEBUG) */
    return 0;
}

static DSPerr bsod_unprj(BSOD_BSTRM *p_bstrm,DSPshort *p_data,  DSPshort numbits)   
{
    /* declare local variables */
    DSPushort data;
    *p_data = (DSPshort)((p_bstrm->pkdata << p_bstrm->pkbitptr) & gbl_msktab[numbits]);
    p_bstrm->pkbitptr += numbits;
    if (p_bstrm->pkbitptr >= GBL_BITSPERWRD)
    {
        p_bstrm->p_pkbuf++;
        p_bstrm->pkdata = *p_bstrm->p_pkbuf;
        p_bstrm->pkbitptr -= GBL_BITSPERWRD;
        data = (DSPushort)p_bstrm->pkdata;
        *p_data |= ((data >> (numbits - p_bstrm->pkbitptr)) & gbl_msktab[numbits]);
    }
    *p_data = (DSPshort)((DSPushort)(*p_data) >> (GBL_BITSPERWRD - numbits));
    return 0;
}

//at least need:56bit(=7 bytes)
static int Get_DD_Parameters(void *buf, int *sample_rate, int *frame_size, int *ChNum)
{
    int numch=0;
    BSOD_BSTRM bstrm;
    BSOD_BSTRM *p_bstrm=&bstrm;
    short tmp=0,acmod,lfeon,fscod,frmsizecod;
    bsod_init((short*)buf,0,p_bstrm);
    
   /* Unpack bsi fields */
    bsod_unprj(p_bstrm, &tmp, 16);
    if (tmp!= GBL_SYNCWRD)
    {
        ALOGI("Invalid synchronization word");
        return -1;
    }
    bsod_unprj(p_bstrm, &tmp, 16);
    bsod_unprj(p_bstrm, &fscod, 2);
    if (fscod == GBL_MAXFSCOD)
    {
        ALOGI("Invalid sampling rate code");
        return -1;
    }

    if (fscod == 0)      *sample_rate = 48000;
    else if (fscod == 1) *sample_rate = 44100;
    else if (fscod == 2) *sample_rate = 32000;
    
    bsod_unprj(p_bstrm, &frmsizecod, 6);
    if (frmsizecod >= GBL_MAXDDDATARATE)
    {
        ALOGI("Invalid frame size code");
        return -1;
    }

    *frame_size=2*(gbl_frmsizetab[fscod][frmsizecod]);
    
    bsod_unprj(p_bstrm, &tmp, 5);
    if (!BSI_ISDD(tmp))
    {
        ALOGI("Unsupported bitstream id");
        return -1;
    }

    bsod_unprj(p_bstrm, &tmp, 3);
    bsod_unprj(p_bstrm, &acmod, 3);

    if ((acmod!= GBL_MODE10) && (acmod& 0x1))
    {
        bsod_unprj(p_bstrm, &tmp, 2);
    }
    if (acmod& 0x4)
    {
        bsod_unprj(p_bstrm, &tmp, 2);
    }
    
    if (acmod == GBL_MODE20)
    {
        bsod_unprj(p_bstrm,&tmp, 2);
    }
    bsod_unprj(p_bstrm, &lfeon, 1);


    /* Set up p_bsi->bse_acmod and lfe on derived variables */
    numch = gbl_chanary[acmod];

    char cEndpoint[256]={0}; //PROPERTY_VALUE_MAX
    property_get(cEndpointProp, cEndpoint, "invalid");
    const int nEndpoints = sizeof(endpoints)/sizeof(*endpoints);
    int activeEndpoint = (nEndpoints - 1); //default to invalid
        
    for(int i=0; i < nEndpoints; i++)
    {
        if( strcmp( cEndpoint, endpoints[i]) == 0 ) 
        {
            activeEndpoint = i;
            break;
        }
    }
    
    if(DDP_OUTMIX_STREAM == (ddp_downmix_config_t)EndpointConfig[activeEndpoint][0])
    {
        if(numch >= 3)  numch = 8;
        else             numch = 2;
    }else{
        numch = 2;
    }
    
    *ChNum=numch;
    //ALOGI("DEBUG[%s %d]:numch=%d,sr=%d,frs=%d",__FUNCTION__,__LINE__,*ChNum,*sample_rate,*frame_size);
    return 0;
}

//at least need:40bit(=5 bytes)
static int Get_DDP_Parameters(void *buf, int *sample_rate, int *frame_size, int *ChNum)
{
    int numch=0;
    BSOD_BSTRM bstrm;
    BSOD_BSTRM *p_bstrm=&bstrm;
    short tmp=0,acmod,lfeon,strmtyp;
    
    bsod_init((short*)buf,0,p_bstrm);
    
    /* Unpack bsi fields */
    bsod_unprj(p_bstrm, &tmp, 16);
    if (tmp!= GBL_SYNCWRD)
    {
        ALOGI("Invalid synchronization word");
        return -1;
    }

    bsod_unprj(p_bstrm, &strmtyp, 2);
    bsod_unprj(p_bstrm, &tmp, 3);
    bsod_unprj(p_bstrm, &tmp, 11);
    
    *frame_size=2*(tmp+1);
    if(strmtyp!=0 && strmtyp!=2){
        return -1;
    }
    bsod_unprj(p_bstrm, &tmp, 2);

    if (tmp== 0x3)
    {
        ALOGI("Half sample rate unsupported");
        return -1;
    }else{
        if (tmp == 0)     *sample_rate = 48000;
        else if (tmp == 1) *sample_rate = 44100;
        else if (tmp == 2) *sample_rate = 32000;
        
        bsod_unprj(p_bstrm, &tmp, 2);
    }

    bsod_unprj(p_bstrm, &acmod, 3);
    bsod_unprj(p_bstrm, &lfeon, 1);

    /* Set up p_bsi->bse_acmod and lfe on derived variables */
    numch = gbl_chanary[acmod];
    
    char cEndpoint[256]={0};//PROPERTY_VALUE_MAX
    property_get(cEndpointProp, cEndpoint, "invalid");
    const int nEndpoints = sizeof(endpoints)/sizeof(*endpoints);
    int activeEndpoint = (nEndpoints - 1); //default to invalid
        
    for(int i=0; i < nEndpoints; i++)
    {
        if( strcmp( cEndpoint, endpoints[i]) == 0 ) 
        {
            activeEndpoint = i;
            break;
        }
    }
    
    if (activeEndpoint == (nEndpoints - 1))
    {
        //ALOGI("Active Endpoint not defined - using default");
    }
    
    if(DDP_OUTMIX_STREAM == (ddp_downmix_config_t)EndpointConfig[activeEndpoint][0])
    {
        if(numch >= 3)  numch = 8;
        else             numch = 2;
    }else{
        numch = 2;
    }
    *ChNum=numch;
    //ALOGI("DEBUG[%s %d]:numch=%d,sr=%d,frs=%d",__FUNCTION__,__LINE__,*ChNum,*sample_rate,*frame_size);
    return 0;

}

static DSPerr bsod_skip(BSOD_BSTRM  *p_bstrm, DSPshort  numbits)
{
    /* check input arguments */
    p_bstrm->pkbitptr += numbits;
    while (p_bstrm->pkbitptr >= GBL_BITSPERWRD)
    {
        p_bstrm->p_pkbuf++;
        p_bstrm->pkdata = *p_bstrm->p_pkbuf;
        p_bstrm->pkbitptr -= GBL_BITSPERWRD;
    }
    
    return 0;
}

static DSPerr bsid_getbsid(BSOD_BSTRM *p_inbstrm,   DSPshort *p_bsid)   
{
    /* Declare local variables */
    BSOD_BSTRM  bstrm;

    /* Initialize local bitstream using input bitstream */
    bsod_init(p_inbstrm->p_pkbuf, p_inbstrm->pkbitptr, &bstrm);

    /* Skip ahead to bitstream-id */
    bsod_skip(&bstrm, BSI_BSID_BITOFFSET);

    /* Unpack bitstream-id */
    bsod_unprj(&bstrm, p_bsid, 5);

    /* If not a DD+ bitstream and not a DD bitstream, return error */
    if (!BSI_ISDDP(*p_bsid) && !BSI_ISDD(*p_bsid))
    {
        ALOGI("Unsupported bitstream id");
        return -1;
    }

    return 0;
}

static int Get_Parameters(void *buf,int *sample_rate, int *frame_size, int *ChNum)
{  
    BSOD_BSTRM bstrm;
    BSOD_BSTRM *p_bstrm=&bstrm;
    DSPshort    bsid;
    int chnum=0;
    uint8_t ptr8[PTR_HEAD_SIZE];
    
    memcpy(ptr8,buf,PTR_HEAD_SIZE);

    //ALOGI("LZG->ptr_head:0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
    //     ptr8[0],ptr8[1],ptr8[2], ptr8[3],ptr8[4],ptr8[5] );
    if((ptr8[0]==0x0b) && (ptr8[1]==0x77) )
    {
       int i;
       uint8_t tmp;
       for(i=0;i<PTR_HEAD_SIZE;i+=2)
       {
          tmp=ptr8[i];
          ptr8[i]=ptr8[i+1];
          ptr8[i+1]=tmp;
       }
    }
    
    bsod_init((short*)ptr8,0,p_bstrm);
    int ret = bsid_getbsid(p_bstrm, &bsid);
    if(ret < 0){
        return -1;
    }
    
    if (BSI_ISDDP(bsid))
    {
        Get_DDP_Parameters(ptr8,sample_rate, frame_size, ChNum);
    }else if (BSI_ISDD(bsid)){
        Get_DD_Parameters(ptr8,sample_rate, frame_size, ChNum);
    }
    
    return 0;
}

//----------------------------DDP Media Source----------------------------------------------

DDP_Media_Source::DDP_Media_Source(void)   
{   
    ALOGI("[%s: %d]\n",__FUNCTION__,__LINE__);
    mStarted = false;
    mMeta = new MetaData;
    mGroup = NULL;
    mSample_rate = 0;
    mChNum = 0;
    mFrame_size = 0;
    mStop_ReadBuf_Flag = 0; //0:start 1:stop
}

DDP_Media_Source::~DDP_Media_Source() {}

int DDP_Media_Source::GetSampleRate()
{
    ALOGI("[DDP_Media_Source::%s: %d]\n",__FUNCTION__,__LINE__);
    return mSample_rate;
}
int DDP_Media_Source::GetChNum()
{
    ALOGI("[DDP_Media_Source::%s: %d]\n",__FUNCTION__,__LINE__);
    return mChNum;
}

int DDP_Media_Source::Get_Stop_ReadBuf_Flag()
{
    return mStop_ReadBuf_Flag;
}

int DDP_Media_Source::Set_Stop_ReadBuf_Flag(int Stop)
{
    //ALOGI("[DDP_Media_Source::%s: %d]\n",__FUNCTION__,__LINE__);
    mStop_ReadBuf_Flag = Stop;
    return 0;
}

sp<MetaData> DDP_Media_Source::getFormat() {
    ALOGI("[DDP_Media_Source::%s: %d]\n",__FUNCTION__,__LINE__);
    return mMeta;
}

status_t DDP_Media_Source::start(MetaData *params)
{
    ALOGI("[DDP_Media_Source::%s: %d]\n",__FUNCTION__,__LINE__);
    mGroup = new MediaBufferGroup;
    mGroup->add_buffer(new MediaBuffer(4096));
    mStarted = true;
    return OK;  
}

status_t DDP_Media_Source::stop()
{
    ALOGI("[DDP_Media_Source::%s: %d]\n",__FUNCTION__,__LINE__);
    delete mGroup;
    mGroup = NULL;
    mStarted = false;
    return OK;
}

int DDP_Media_Source::MediaSourceRead_buffer(unsigned char *buffer,int size)
{
   int readcnt = 0;
   int sleep_time = 0;
   if(mStop_ReadBuf_Flag == 1)
   {
       ALOGI("[DDP_Media_Source::%s] ddp mediasource stop!\n ", __FUNCTION__);
       return 0;
   }
   while((readcnt < size)&&(mStop_ReadBuf_Flag == 0))
   {
       readcnt = buffer_read(&android_out_buffer, (char*)buffer, size);
       if(readcnt < 0){
           sleep_time++;
           usleep(1000); //1ms
       }
       if((sleep_time > 0) && (sleep_time%100==0)){
           ALOGI("[%s] Can't get data from audiobuffer,wait for %d ms\n ", __FUNCTION__,sleep_time);
       }
       if(sleep_time > 1000){//wait for max 10s to get audio data
           ALOGE("[%s] time out to read audio buffer data! wait for 1s\n ", __FUNCTION__);
           return -1;
       }
   }
   return readcnt;
}

status_t DDP_Media_Source::read(MediaBuffer **out, const ReadOptions *options)
{
    *out = NULL;
    unsigned char ptr_head[PTR_HEAD_SIZE]={0};
    int readedbytes;
    int SyncFlag=0;
    
resync: 
    mFrame_size = 0;
    SyncFlag = 0;
    
    if (mStop_ReadBuf_Flag == 1){
         ALOGI("Stop Flag is set, stop read_buf [%s %d]",__FUNCTION__,__LINE__);
         return ERROR_END_OF_STREAM;
    }
    
    if(MediaSourceRead_buffer(&ptr_head[0], 6)<6)
    {
        readedbytes=6;
        ALOGI("WARNING:read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
        return ERROR_END_OF_STREAM;
    }
    
    if (mStop_ReadBuf_Flag == 1){
         ALOGI("Stop Flag is set, stop read_buf [%s %d]",__FUNCTION__,__LINE__);
         return ERROR_END_OF_STREAM;
    }

    while(!SyncFlag)
    {
         int i;
         for(i=0;i<=4;i++)
         {
             if((ptr_head[i]==0x0b && ptr_head[i+1]==0x77 )||(ptr_head[i]==0x77 && ptr_head[i+1]==0x0b ))
             {
                memcpy(&ptr_head[0],&ptr_head[i],6-i);
                if(MediaSourceRead_buffer(&ptr_head[6-i],i)<i){
                    readedbytes=i;
                    ALOGI("WARNING: read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
                    return ERROR_END_OF_STREAM;
                }
                
                if (mStop_ReadBuf_Flag == 1){
                    ALOGI("Stop Flag is set, stop read_buf [%s %d]",__FUNCTION__,__LINE__);
                    return ERROR_END_OF_STREAM;
                }
                SyncFlag=1;
                break;
             }
         }
         
         if(SyncFlag==0)
         {
             ptr_head[0]=ptr_head[5];
             if(MediaSourceRead_buffer(&ptr_head[1],5)<5)
             {
             	readedbytes=5;
                ALOGI("WARNING: fpread_buffer read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
                return ERROR_END_OF_STREAM;
             }
             
             if (mStop_ReadBuf_Flag == 1){
                ALOGI("Stop Flag is set, stop read_buf [%s %d]",__FUNCTION__,__LINE__);
                return ERROR_END_OF_STREAM;
            }
         }
    }
    if(MediaSourceRead_buffer(&ptr_head[6], PTR_HEAD_SIZE-6)<(PTR_HEAD_SIZE-6))
    {
        readedbytes=PTR_HEAD_SIZE-6;
        ALOGI("WARNING: fpread_buffer read %d bytes failed [%s %d]!\n",readedbytes,__FUNCTION__,__LINE__);
        return ERROR_END_OF_STREAM;
    }
    
    Get_Parameters(ptr_head, &mSample_rate, &mFrame_size, &mChNum);
    if((mFrame_size == 0)||(mFrame_size < PTR_HEAD_SIZE) ||(mChNum == 0)||(mSample_rate == 0))
    {  
       goto resync;
    }

    MediaBuffer *buffer;
    status_t err = mGroup->acquire_buffer(&buffer);
    if (err != OK) {
        return err;
    }
    memcpy((unsigned char*)(buffer->data()),ptr_head,PTR_HEAD_SIZE);
    if (MediaSourceRead_buffer((unsigned char*)(buffer->data())+PTR_HEAD_SIZE,mFrame_size-PTR_HEAD_SIZE) 
                                    != (mFrame_size-PTR_HEAD_SIZE)) {
        ALOGI("[%s %d]stream read failed:bytes_req/%d\n",__FUNCTION__,__LINE__,(mFrame_size-PTR_HEAD_SIZE)); 
        buffer->release();
        buffer = NULL;
        return ERROR_END_OF_STREAM;
    }

    buffer->set_range(0, mFrame_size);
    buffer->meta_data()->setInt64(kKeyTime, 0);
    buffer->meta_data()->setInt32(kKeyIsSyncFrame, 1);
    
    *out = buffer;
    return OK;
}

//-------------------------------OMX codec------------------------------------------------

const char *MEDIA_MIMETYPE_AUDIO_AC3 = "audio/ac3";
Aml_OMX_Codec::Aml_OMX_Codec(void) 
{
	ALOGI("[Aml_OMX_Codec::%s: %d]\n",__FUNCTION__,__LINE__);
    m_codec = NULL;
    status_t m_OMXClientConnectStatus = m_OMXClient.connect();
    lock_init();
    locked();
    buf_decode_offset = 0;
    buf_decode_offset_pre = 0;
	
    if(m_OMXClientConnectStatus != OK){
        ALOGE("Err:omx client connect error\n");
    }else{
        const char *mine_type = NULL;
        mine_type = MEDIA_MIMETYPE_AUDIO_AC3;
        m_OMXMediaSource = new DDP_Media_Source();
        sp<MetaData> metadata = m_OMXMediaSource->getFormat();
        metadata->setCString(kKeyMIMEType, mine_type);
        m_codec = OMXCodec::Create(
                        m_OMXClient.interface(),
                        metadata,
                        false, // createEncoder
                        m_OMXMediaSource,
                        0,
                        0); 
        
        if (m_codec != NULL){   
            ALOGI("OMXCodec::Create success %s %d \n",__FUNCTION__,__LINE__);
        }else{
            ALOGE("Err: OMXCodec::Create failed %s %d \n",__FUNCTION__,__LINE__);
        }
    }
    unlocked();
}


Aml_OMX_Codec::~Aml_OMX_Codec(){}

status_t Aml_OMX_Codec::read(unsigned char *buf, unsigned *size, int *exit)
{
    MediaBuffer *srcBuffer;
    status_t status;
    
    m_OMXMediaSource->Set_Stop_ReadBuf_Flag(*exit);
   
    if(*exit)
    {
        ALOGI("NOTE:exit flag enabled! [%s %d] \n",__FUNCTION__,__LINE__);
        *size=0;
        return OK;
    }
   
    status=  m_codec->read(&srcBuffer,NULL);
     
    if(srcBuffer==NULL)
    {    
        *size=0;
        return OK;
    }
        
    *size = srcBuffer->range_length();

    if(status == OK && (*size != 0) ){
        memcpy((unsigned char *)buf, (unsigned char *)srcBuffer->data()+srcBuffer->range_offset(), *size);
        srcBuffer->set_range(srcBuffer->range_offset() + (*size),srcBuffer->range_length() - (*size));
        srcBuffer->meta_data()->findInt64(kKeyTime, &buf_decode_offset);
    }
    
    if (srcBuffer->range_length() == 0) {		
         srcBuffer->release();
         srcBuffer = NULL;	 
    }
   
    return OK;
}

status_t Aml_OMX_Codec::start()
{
    ALOGI("[Aml_OMX_Codec::%s %d] enter!\n",__FUNCTION__,__LINE__);
    status_t status = m_codec->start();
    if (status != OK)
    {
        ALOGE("Err:OMX client can't start OMX decoder! status=%d (0x%08x)\n", (int)status, (int)status);
        m_codec.clear();
    }
    return status;
}

void  Aml_OMX_Codec::stop()
{
    ALOGI("[Aml_OMX_Codec::%s %d] enter!\n",__FUNCTION__,__LINE__);
    if(m_codec != NULL){
       if(m_OMXMediaSource->Get_Stop_ReadBuf_Flag())          
           m_OMXMediaSource->Set_Stop_ReadBuf_Flag(1);
       m_codec->pause();
       m_codec->stop();
       wp<MediaSource> tmp = m_codec;
	   m_codec.clear();
       while (tmp.promote() != NULL) {
           ALOGI("[Aml_OMX_Codec::%s %d]wait m_codec free OK!\n",__FUNCTION__,__LINE__);
           usleep(1000);
       }
       m_OMXClient.disconnect();
       m_OMXMediaSource.clear();
    }else
       ALOGE("m_codec==NULL, m_codec->stop() failed! [%s %d] \n",__FUNCTION__,__LINE__);
}

void Aml_OMX_Codec::pause()
{
    ALOGI("[Aml_OMX_Codec::%s %d] \n",__FUNCTION__,__LINE__);
    if(m_codec != NULL)
        m_codec->pause();
    else
        ALOGE("m_codec==NULL, m_codec->pause() failed! [%s %d] \n",__FUNCTION__,__LINE__);
}

int Aml_OMX_Codec::GetDecBytes()
{
    int used_len=0;
    used_len = buf_decode_offset - buf_decode_offset_pre;
    buf_decode_offset_pre = buf_decode_offset;
    return used_len;
}

void Aml_OMX_Codec::lock_init()
{ 
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&lock, &attr);
    pthread_mutexattr_destroy(&attr);
}
void Aml_OMX_Codec::locked()
{   
    pthread_mutex_lock(&lock);
}
void Aml_OMX_Codec::unlocked()
{
    pthread_mutex_unlock(&lock);
}

//--------------------------------OMX_Codec_local API-------------------------------------------------

Aml_OMX_Codec *arm_omx_codec = NULL;

void omx_codec_pause()
{
    if(arm_omx_codec != NULL){
        arm_omx_codec->locked();
        arm_omx_codec->pause();
        arm_omx_codec->unlocked();
    }else
        ALOGE("arm_omx_codec==NULL  arm_omx_codec->pause failed! %s %d \n",__FUNCTION__,__LINE__);
}

void omx_codec_read(unsigned char *buf,unsigned *size,int *exit)
{        
     if(arm_omx_codec != NULL){
         arm_omx_codec->locked();
         arm_omx_codec->read(buf,size,exit);
         arm_omx_codec->unlocked();
     }else
         ALOGE("arm_omx_codec==NULL  arm_omx_codec->read failed! %s %d \n",__FUNCTION__,__LINE__);
}

int omx_codec_get_declen()
{
    int declen=0;
    if(arm_omx_codec != NULL){
        arm_omx_codec->locked();
        declen = arm_omx_codec->GetDecBytes();
        arm_omx_codec->unlocked();
    }else{
        ALOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_declen() return 0! %s %d \n",__FUNCTION__,__LINE__);
    }
    return declen;
}

int omx_codec_get_FS()
{  
    if(arm_omx_codec != NULL){
        return arm_omx_codec->m_OMXMediaSource->GetSampleRate();
      
    }else{
    	ALOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_FS() return 0! %s %d \n",__FUNCTION__,__LINE__);
    	return 0;
    }
}

int omx_codec_get_Nch()
{  
    if(arm_omx_codec != NULL){
    	return arm_omx_codec->m_OMXMediaSource->GetChNum();
    }else{
        ALOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_get_Nch() return 0! %s %d \n",__FUNCTION__,__LINE__);
        return 0;
    }
}

//--------------------------------------Decoder ThreadLoop--------------------------------------------
static pthread_mutex_t decode_dev_op_mutex = PTHREAD_MUTEX_INITIALIZER;
static int decode_ThreadExitFlag = 0; //0:exit from thread; 1:thread looping
static int decode_ThreadStopFlag = 1; //0:start; 1: stop
static pthread_t decode_ThreadID = 0;

void *decode_threadloop(void *args){
	unsigned int outlen = 0;
	int write_sucessed = 1;
	int ret = 0;
	decode_ThreadStopFlag = 0;
	decode_ThreadExitFlag = 1;
	unsigned char tmp[8192];
	
	while((decode_ThreadStopFlag+1)%2){
		if(write_sucessed == 1){
			outlen = 0;
			omx_codec_read(&tmp[0], &outlen, &(decode_ThreadStopFlag));
			ALOGD("%s, out length = %d \n", __FUNCTION__,outlen);
		}
		if(outlen > 0){
			ret = buffer_write(&DDP_out_buffer, (char *)(&tmp[0]), outlen);
			if(ret < 0){
				write_sucessed = 0;
				usleep(10*1000); //10ms
			}else{
				write_sucessed = 1;
			}
		}
    }

	decode_ThreadStopFlag = 1;
	decode_ThreadExitFlag = 0;
	ALOGD("%s, exiting...\n", __FUNCTION__);
	return NULL;
}

static int start_decode_thread_omx(void){
    pthread_attr_t attr;
    struct sched_param param;
    int ret = 0;

	ALOGI("[%s %d] enter!\n",__FUNCTION__,__LINE__);
    pthread_mutex_lock(&decode_dev_op_mutex);
    pthread_attr_init(&attr);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);
    param.sched_priority = sched_get_priority_max(SCHED_RR);
    pthread_attr_setschedparam(&attr, &param);
    decode_ThreadStopFlag = 1;
    decode_ThreadExitFlag = 0;
    ret = pthread_create(&decode_ThreadID, &attr, &decode_threadloop, NULL);
    pthread_attr_destroy(&attr);
    if (ret != 0) {
        ALOGE("%s, Create thread fail!\n", __FUNCTION__);
        pthread_mutex_unlock(&decode_dev_op_mutex);
        return -1;
    }
    pthread_mutex_unlock(&decode_dev_op_mutex);
    ALOGD("[%s] exiting...\n", __FUNCTION__);
    return 0;
}

static int stop_decode_thread_omx(void)
{
    int i = 0, tmp_timeout_count = 1000;

    ALOGI("[%s %d] enter!\n",__FUNCTION__,__LINE__);
    pthread_mutex_lock(&decode_dev_op_mutex);

    decode_ThreadStopFlag = 1;
    while (1){
        if(decode_ThreadExitFlag == 0) break;
        if (i >= tmp_timeout_count) break;
        i++;
        usleep(10 * 1000);
    }
	if (i >= tmp_timeout_count) {
        ALOGE("%s, Timeout: we have try %d ms, but the aml audio thread's exec flag is still(%d)!!!\n",
              __FUNCTION__, tmp_timeout_count*10, decode_ThreadExitFlag);
    } else {
        ALOGD("%s, kill decode thread success after try %d ms.\n",__FUNCTION__, i*10);
    }

    pthread_join(decode_ThreadID, NULL);
    decode_ThreadID = 0;

    ALOGD("%s, aml audio close success.\n", __FUNCTION__);
    pthread_mutex_unlock(&decode_dev_op_mutex);
    return 0;
}

//-------------------------------------external OMX_codec_api-----------------------------------------
extern "C" {
int omx_codec_init(void)
{
    int ret = 0;
    arm_omx_codec = new android::Aml_OMX_Codec();
    if(arm_omx_codec == NULL){
        ALOGE("Err:arm_omx_codec_init failed!\n");
		return -1;
    }

	arm_omx_codec->locked();
    ret = arm_omx_codec->start();
    arm_omx_codec->unlocked();
	if(ret < 0){
		goto Exit;
	}
	
	ret = start_decode_thread_omx();
	if(ret == 0) return 0;
Exit:
	arm_omx_codec->stop();
	delete arm_omx_codec;
    arm_omx_codec = NULL;
    return -1;
}

void omx_codec_close(void)
{
     int ret=0;
	 stop_decode_thread_omx();
     if(arm_omx_codec != NULL){
         arm_omx_codec->locked();
         arm_omx_codec->stop();
         arm_omx_codec->unlocked();
         delete arm_omx_codec;
         arm_omx_codec = NULL;
     }else{
         ALOGI("NOTE:arm_omx_codec==NULL arm_omx_codec_close() do nothing! %s %d \n",__FUNCTION__,__LINE__);
     }
}

}

};

