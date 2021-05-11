#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/prctl.h>

#include "libavformat/avformat.h"

#include "mi_vdec.h"
#include "mi_vdec_datatype.h"
#include "mi_common.h"
#include "mi_common_datatype.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_panel_datatype.h"
#include "mi_panel.h"
#include "mi_disp_datatype.h"
#include "mi_disp.h"
#include "mi_ao.h"

#include "sstar_disp.h"


#define STCHECKRESULT(result)\
    if (result != MI_SUCCESS)\
    {\
        printf("[%s %d]exec function failed\n", __FUNCTION__, __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%s %d)exec function pass\n", __FUNCTION__,__LINE__);\
    }

#define VDEC_CHN_ID     0

#define AUDIO_CHN       0
#define MI_AUDIO_SAMPLE_PER_FRAME 1024
#define DMA_BUF_SIZE_8K     (8000)
#define DMA_BUF_SIZE_16K    (16000)
#define DMA_BUF_SIZE_32K    (32000)
#define DMA_BUF_SIZE_48K    (48000)


#define AUDIO_DEV       0

#define VDEC_INPUT_WIDTH     1920
#define VDEC_INPUT_HEIGHT    1080

#define VDEC_OUTPUT_WIDTH     1024
#define VDEC_OUTPUT_HEIGHT    600

typedef struct WAVE_FORMAT
{
    signed short wFormatTag;
    signed short wChannels;
    unsigned int dwSamplesPerSec;
    unsigned int dwAvgBytesPerSec;
    signed short wBlockAlign;
    signed short wBitsPerSample;
} WaveFormat_t;

typedef struct WAVEFILEHEADER
{
    char chRIFF[4];
    unsigned int  dwRIFFLen;
    char chWAVE[4];
    char chFMT[4];
    unsigned int  dwFMTLen;
    WaveFormat_t wave;
    char chDATA[4];
    unsigned int  dwDATALen;
} WaveFileHeader_t;

static MI_S32 g_s32NeedSize;
static MI_U32 u32DmaBufSize;
static MI_S32 s32SoundLayout;
static MI_S32 s32SampleRate;
static MI_BOOL bExit = FALSE;
MI_VDEC_CodecType_e _eCodecType = E_MI_VDEC_CODEC_TYPE_H264;

int sstar_vdec_init(void)
{
    //init vdec
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_VDEC_OutputPortAttr_t stOutputPortAttr;
    MI_VDEC_CHN stVdecChn = VDEC_CHN_ID;
    MI_VDEC_InitParam_t stVdecInitParam;

    memset(&stVdecInitParam, 0, sizeof(MI_VDEC_InitParam_t));
    stVdecInitParam.bDisableLowLatency = false; //false: 不带B帧, true: 带B帧
    STCHECKRESULT(MI_VDEC_InitDev(&stVdecInitParam));

    memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
    stVdecChnAttr.eCodecType   = _eCodecType;
    stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 5;
    stVdecChnAttr.eVideoMode   = E_MI_VDEC_VIDEO_MODE_FRAME;
    stVdecChnAttr.u32BufSize   = 1 * 1920 * 1080;
    stVdecChnAttr.u32PicWidth  = inVdecWidth;
    stVdecChnAttr.u32PicHeight = inVdecHeight;
    stVdecChnAttr.eDpbBufMode  = E_MI_VDEC_DPB_MODE_NORMAL;
    stVdecChnAttr.u32Priority  = 0;

    STCHECKRESULT(MI_VDEC_CreateChn(stVdecChn, &stVdecChnAttr));
    STCHECKRESULT(MI_VDEC_StartChn(stVdecChn));

    memset(&stOutputPortAttr, 0, sizeof(MI_VDEC_OutputPortAttr_t));
    stOutputPortAttr.u16Width  = OutVdecWidth;
    stOutputPortAttr.u16Height = OutVdecHeight;
    STCHECKRESULT(MI_VDEC_SetOutputPortAttr(0, &stOutputPortAttr));

    MI_SYS_ChnPort_t  stChnPort;
    memset(&stChnPort, 0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId    = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId  = 0;
    stChnPort.u32ChnId  = stVdecChn;
    stChnPort.u32PortId = 0;

    STCHECKRESULT(MI_SYS_SetChnOutputPortDepth(&stChnPort, 0, 5));

    //bind vdec 2 disp
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32PortId = 0;

    stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32ChnId = 0;
    stDstChnPort.u32PortId = 0;

    STCHECKRESULT(MI_SYS_BindChnPort(&stSrcChnPort, &stDstChnPort, 30, 30));

    if(bRota)
    {
        MI_DISP_RotateConfig_t stRotateConfig;
        memset(&stRotateConfig, 0, sizeof(MI_DISP_RotateConfig_t));
        stRotateConfig.eRotateMode      = E_MI_DISP_ROTATE_90;

        MI_DISP_SetVideoLayerRotateMode(0, &stRotateConfig);
    }

    return 0;
}

int sstar_vdec_deInit(void)
{
    //Unbind vdec 2 disp
    MI_SYS_ChnPort_t stSrcChnPort;
    MI_SYS_ChnPort_t stDstChnPort;

    stSrcChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stSrcChnPort.u32DevId = 0;
    stSrcChnPort.u32ChnId = 0;
    stSrcChnPort.u32PortId = 0;

    stDstChnPort.eModId = E_MI_MODULE_ID_DISP;
    stDstChnPort.u32DevId = 0;
    stDstChnPort.u32ChnId = 0;
    stDstChnPort.u32PortId = 0;

    STCHECKRESULT(MI_SYS_UnBindChnPort(&stSrcChnPort, &stDstChnPort));

    STCHECKRESULT(MI_VDEC_StopChn(0));
    STCHECKRESULT(MI_VDEC_DestroyChn(0));
    STCHECKRESULT(MI_VDEC_DeInitDev());

    return 0;
}

int sstar_ao_deinit(void)
{
    MI_AUDIO_DEV AoDevId = AUDIO_DEV;
    MI_AO_CHN AoChn = AUDIO_CHN;

    /* disable ao channel of */
    MI_AO_DisableChn(AoDevId, AoChn);

    /* disable ao device */
    MI_AO_Disable(AoDevId);

    return MI_SUCCESS;
}

int sstar_ao_init(void)
{
    MI_AUDIO_Attr_t stSetAttr;
    MI_AUDIO_Attr_t stGetAttr;
    MI_AUDIO_DEV AoDevId = AUDIO_DEV;
    MI_AO_CHN AoChn = AUDIO_CHN;

    MI_S32 s32SetVolumeDb;
    MI_S32 s32GetVolumeDb;

    //set Ao Attr struct
    memset(&stSetAttr, 0, sizeof(MI_AUDIO_Attr_t));
    stSetAttr.eBitwidth = E_MI_AUDIO_BIT_WIDTH_16;
    stSetAttr.eWorkmode = E_MI_AUDIO_MODE_I2S_MASTER;
    stSetAttr.u32FrmNum = 6;
    stSetAttr.u32PtNumPerFrm = MI_AUDIO_SAMPLE_PER_FRAME;

    if(s32SoundLayout == 2) {
        stSetAttr.u32ChnCnt = 2;
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_STEREO;  // 立体声
    }
    else if(s32SoundLayout == 1) {
        stSetAttr.u32ChnCnt = 1;
        stSetAttr.eSoundmode = E_MI_AUDIO_SOUND_MODE_MONO;    // 单声道
    }

    //stSetAttr.eSamplerate = E_MI_AUDIO_SAMPLE_RATE_48000;   // 采样率
    stSetAttr.eSamplerate = s32SampleRate;
    printf("set ao sound layout [%d], sample rate [%d]\n", s32SoundLayout, s32SampleRate);

    g_s32NeedSize = MI_AUDIO_SAMPLE_PER_FRAME * 2 * (stSetAttr.u32ChnCnt);
    if (E_MI_AUDIO_SAMPLE_RATE_8000 == stSetAttr.eSamplerate) {
        u32DmaBufSize = DMA_BUF_SIZE_8K;;
    }
    else if (E_MI_AUDIO_SAMPLE_RATE_16000 == stSetAttr.eSamplerate) {
        u32DmaBufSize = DMA_BUF_SIZE_16K;
    }
    else if (E_MI_AUDIO_SAMPLE_RATE_32000 == stSetAttr.eSamplerate) {
        u32DmaBufSize = DMA_BUF_SIZE_32K;
    }
    else if (E_MI_AUDIO_SAMPLE_RATE_48000 == stSetAttr.eSamplerate) {
        u32DmaBufSize = DMA_BUF_SIZE_48K;
    }

    if (stSetAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_STEREO) {
        if (g_s32NeedSize > (u32DmaBufSize / 4)) {
            g_s32NeedSize = u32DmaBufSize / 4;
        }
    }
    else if (stSetAttr.eSoundmode == E_MI_AUDIO_SOUND_MODE_MONO) {
        if (g_s32NeedSize > (u32DmaBufSize / 8)) {
            g_s32NeedSize = u32DmaBufSize / 8;
        }
    }

    /* set ao public attr*/
    MI_AO_SetPubAttr(AoDevId, &stSetAttr);

    /* get ao device*/
    MI_AO_GetPubAttr(AoDevId, &stGetAttr);

    /* enable ao device */
    MI_AO_Enable(AoDevId);

    /* enable ao channel of device*/
    MI_AO_EnableChn(AoDevId, AoChn);

    /* if test AO Volume */
    s32SetVolumeDb = 0;
    MI_AO_SetVolume(AoDevId, s32SetVolumeDb);   // 音量[-60dB ~ 30dB]

    /* get AO volume */
    MI_AO_GetVolume(AoDevId, &s32GetVolumeDb);

    return MI_SUCCESS;
}

static void * sstar_audio_thread(void* arg)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_AUDIO_Frame_t stAoSendFrame;
    MI_U8 u8TempBuf[MI_AUDIO_SAMPLE_PER_FRAME * 4];
    WaveFileHeader_t g_stWavHeaderInput;

    char *wave_file = (char *)arg;
    printf("try to open wave file : %s\n", wave_file);

    int g_AoReadFd = open((const char *)wave_file, O_RDONLY, 0666);
    if(g_AoReadFd <= 0) {
        printf("open input file failed!!!\n");
        return NULL;
    }

    s32Ret = read(g_AoReadFd, &g_stWavHeaderInput, sizeof(WaveFileHeader_t));
    if (s32Ret < 0) {
        printf("read wav header failed!!!\n");
        return NULL;
    }
    printf("wave file's audio channel layout = %d\n", g_stWavHeaderInput.wave.wChannels);
    printf("wave file's sample rate = %d\n", g_stWavHeaderInput.wave.dwSamplesPerSec);

    // 拉高公版声音功放IO
    system("echo 12 > /sys/class/gpio/export");
    system("echo out > /sys/class/gpio/gpio12/direction");
    system("echo 1 > /sys/class/gpio/gpio12/value");

    while(!bExit)
    {
        s32Ret = read(g_AoReadFd, &u8TempBuf, g_s32NeedSize);
        if(s32Ret != g_s32NeedSize) {
            lseek(g_AoReadFd, sizeof(WaveFileHeader_t), SEEK_SET);
            s32Ret = read(g_AoReadFd, &u8TempBuf, g_s32NeedSize);
            if (s32Ret < 0) {
                printf("input file does not has enough data!!!\n");
                break;
            }
        }
        // 送数据到AO
        memset(&stAoSendFrame, 0x0, sizeof(MI_AUDIO_Frame_t));
        stAoSendFrame.u32Len = s32Ret;          // pcm 数据长度
        stAoSendFrame.apVirAddr[0] = u8TempBuf; // pcm 数据地址
        stAoSendFrame.apVirAddr[1] = NULL;

        do{
            s32Ret = MI_AO_SendFrame(AUDIO_DEV, AUDIO_CHN, &stAoSendFrame, -1);
        }while(s32Ret == MI_AO_ERR_NOBUF);

        if(s32Ret != MI_SUCCESS)
        {
            printf("[Warning]: MI_AO_SendFrame fail, error is 0x%x: \n", s32Ret);
        }
    }

    if (g_AoReadFd > 0) {
        close(g_AoReadFd);
    }

    return NULL;
}

int main (int argc, char **argv)
{
    const char *filename;
    const char *audio_file = NULL;
    pthread_t tid_audio;
    pthread_t tid_video;

    if(argc < 7)
    {
        printf("Useage: ./SsPlayer file.h264 OutX OutY outDispwidth outDispheight bRota\n");
        return -1;
    }

    if (argc > 7 && argc < 10) {
        printf("Useage: ./SsPlayer file.h264 OutX OutY outDispwidth outDispheight bRota file.wav soundLayout sampleRate\n");
        return -1;
    }

    filename = argv[1];
    inVdecWidth  = VDEC_INPUT_WIDTH;
    inVdecHeight = VDEC_INPUT_HEIGHT;//VDEC输入宽高无须用户根据视频宽高设置,最大1080P即可
    OutVdecWidth  = atoi(argv[4]);
    OutVdecHeight = atoi(argv[5]);//VDEC输出宽高与DISP输出宽高设置一致即可
    inDispWidth  = OutVdecWidth;
    inDispHeight = OutVdecHeight;
    OutX = atoi(argv[2]);
    OutY = atoi(argv[3]);
    OutDispWidth  = atoi(argv[4]);
    OutDispHeight = atoi(argv[5]);
    
    bRota = atoi(argv[6]);

    if (argc > 7 && argv[7]) {
        audio_file = argv[7];
        s32SoundLayout = atoi(argv[8]);
        s32SampleRate  = atoi(argv[9]);
    }

    MI_DISP_PubAttr_t stDispPubAttr;
    stDispPubAttr.eIntfType = E_MI_DISP_INTF_LCD;
    stDispPubAttr.eIntfSync = E_MI_DISP_OUTPUT_USER;
    sstar_disp_init(&stDispPubAttr);

    //init SDK
    sstar_vdec_init();

    if (audio_file) {
        sstar_ao_init();
        pthread_create(&tid_audio, NULL, sstar_audio_thread, (void *)audio_file);
    }

    AVFormatContext *pFormatCtx;
PLAY:
    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        printf("%s %d fail!\n",__FUNCTION__,__LINE__);
        return 0;
    }

    AVPacket *packet = av_malloc(sizeof(AVPacket));

    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        MI_VDEC_VideoStream_t stVdecStream;
        MI_U32 s32Ret;

        stVdecStream.pu8Addr      = packet->data;
        stVdecStream.u32Len       = packet->size;
        stVdecStream.u64PTS       = packet->pts;
        stVdecStream.bEndOfFrame  = 1;
        stVdecStream.bEndOfStream = 0;
        usleep(30*1000);
		printf("packet->data=0x%llx, packet->pts=%lld packet->dts=%lld packet->size=%d\n", (unsigned long long)packet->data,packet->pts,packet->dts,packet->size);
        //printf("data: %x,%x,%x,%x,%x\n",stVdecStream.pu8Addr[0],stVdecStream.pu8Addr[1],stVdecStream.pu8Addr[2],stVdecStream.pu8Addr[3],stVdecStream.pu8Addr[4]);
        if(MI_SUCCESS != (s32Ret = MI_VDEC_SendStream(0, &stVdecStream, 20)))
        {
            printf("[%s %d]MI_VDEC_SendStream failed!\n", __FUNCTION__, __LINE__);
            break;
        } 

        av_packet_unref(packet);
    }

    av_packet_free(&packet);

    if(!bExit)
    {
        avformat_close_input(&pFormatCtx);
        printf("input q exit,input c continue play\n");
        if(getchar() == 'q')
        {
            bExit = TRUE;
            printf("exit\n");
            goto EXIT;
        }
        goto PLAY;
    }
EXIT:
    if (audio_file) {
        pthread_join(tid_audio, NULL);
        sstar_ao_deinit();
    }

    sstar_disp_Deinit(&stDispPubAttr);
    sstar_vdec_deInit();

    return 0;
}
