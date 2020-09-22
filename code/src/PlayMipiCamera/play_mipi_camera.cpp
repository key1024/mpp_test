#include "play_mipi_camera.h"

static void* play_thread(void* data)
{
    ((PlayMipiCamera*)data)->PlayThread();
    return nullptr;
}

PlayMipiCamera::PlayMipiCamera()
{
    m_sensor_type = SONY_IMX335_MIPI_4M_30FPS_12BIT;
    m_width = 2592;
    m_height = 1520;
    m_venc_chn = 0;
}

PlayMipiCamera::~PlayMipiCamera()
{
}

int PlayMipiCamera::Init()
{
    // 系统初始化
    VB_CONFIG_S stVbConf;
    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));
    // 最大缓存池的个数
    // 申请两个大小不同的缓存池，用来分别存储原始图像和压缩后的图像，达到节省内存的目的
    // mpp底层可以根据需要自动申请最合适的内存池
    stVbConf.u32MaxPoolCnt = 2;
    stVbConf.astCommPool[0].u64BlkSize = COMMON_GetPicBufferSize(m_width, m_height, 
                                PIXEL_FORMAT_YVU_SEMIPLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,DEFAULT_ALIGN);
    stVbConf.astCommPool[0].u32BlkCnt    = 4;
    stVbConf.astCommPool[1].u64BlkSize = COMMON_GetPicBufferSize(1920, 1080, 
                                PIXEL_FORMAT_YVU_SEMIPLANAR_420, DATA_BITWIDTH_8, COMPRESS_MODE_NONE,DEFAULT_ALIGN);
    stVbConf.astCommPool[1].u32BlkCnt    = 5;
    HI_S32 s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        printf("SAMPLE_COMM_SYS_Init failed!\n");
        return -1;
    }

    int ret = initVi();
    if(ret != 0)
    {
        SAMPLE_COMM_SYS_Exit();
        return -1;
    }

    ret = initVpss();
    if(ret != 0)
    {
        uninitVi();
        SAMPLE_COMM_SYS_Exit();
        return -1;
    }

    ret = initVenc();
    if(ret != 0)
    {
        uninitVpss();
        uninitVi();
        SAMPLE_COMM_SYS_Exit();
        return -1;
    }

    s32Ret = SAMPLE_COMM_VI_Bind_VPSS(m_vi_config.astViInfo[0].stPipeInfo.aPipe[0], 
                                        m_vi_config.astViInfo[0].stChnInfo.ViChn, m_vpss_grp);
    if(s32Ret != HI_SUCCESS)
    {
        UnInit();
        printf("VI Bind VPSS err for %#x!\n", s32Ret);
        return -1;
    }

    s32Ret = SAMPLE_COMM_VPSS_Bind_VENC(m_vpss_grp, m_vpss_chn, m_venc_chn);
    if(s32Ret != HI_SUCCESS)
    {
        UnInit();
        printf("VPSS Bind VENC err for %#x!\n", s32Ret);
        return -1;
    }

    pthread_create(&m_play_tid, NULL, play_thread, this);

    return 0;
}

int PlayMipiCamera::UnInit()
{
    uninitVenc();
    uninitVpss();
    uninitVi();
    SAMPLE_COMM_SYS_Exit();

    return 0;
}

void PlayMipiCamera::PlayThread()
{
    printf("视频流推送线程启动\n");

    HI_S32 fd = HI_MPI_VENC_GetFd(m_venc_chn);
    if(fd < 0)
    {
        printf("HI_MPI_VENC_GetFd failed with %#x!\n", m_venc_chn);
        return;
    }

    void* data = malloc(1920*1080*3/2);

    int max_fd = fd;
    fd_set read_fds;
    FD_ZERO(&read_fds);
    while (true)
    {
        FD_SET(fd, &read_fds);
        timeval timeout_val;
        timeout_val.tv_sec = 1;
        timeout_val.tv_usec = 0;
        int ret = select(max_fd+1, &read_fds, NULL, NULL, &timeout_val);
        if(ret < 0)
        {
            printf("select failed!\n");
            break;
        }
        else if(ret == 0)
        {
            printf("get venc stream time out\n");
            continue;
        }
        else
        {
            if(FD_ISSET(fd, &read_fds))
            {
                VENC_CHN_STATUS_S status;
                ret = HI_MPI_VENC_QueryStatus(m_venc_chn, &status);
                if (ret != HI_SUCCESS)
                {
                    printf("HI_MPI_VENC_QueryStatus chn[%d] failed with %#x!\n", m_venc_chn, ret);
                    break;
                }

                if(status.u32CurPacks == 0)
                {
                    printf("NOTE: Current  frame is NULL!\n");
                    continue;
                }

                VENC_STREAM_S stream;
                memset(&stream, 0, sizeof(stream));
                stream.pstPack = (VENC_PACK_S*)malloc(sizeof(VENC_PACK_S) * status.u32CurPacks);
                if (NULL == stream.pstPack)
                {
                    printf("malloc stream pack failed!\n");
                    break;
                }
                stream.u32PackCount = status.u32CurPacks;
                ret = HI_MPI_VENC_GetStream(m_venc_chn, &stream, HI_TRUE);
                if (HI_SUCCESS != ret)
                {
                    free(stream.pstPack);
                    printf("HI_MPI_VENC_GetStream failed with %#x!\n", ret);
                    break;
                }

                // 推送视频流
                int data_len = 0;
                for(int i = 0 ; i < stream.u32PackCount; i++)
                {
                    data_len += (stream.pstPack[i].u32Len-stream.pstPack[i].u32Offset);
                }

                void* tmp = data;
                for(int i = 0 ; i < stream.u32PackCount; i++)
                {
                    memcpy(tmp, stream.pstPack[i].pu8Addr+stream.pstPack[i].u32Offset, stream.pstPack[i].u32Len-stream.pstPack[i].u32Offset);
                    tmp = tmp + (stream.pstPack[i].u32Len-stream.pstPack[i].u32Offset);
                }
                m_push_steam.PushOneFrame(data, data_len);    

                ret = HI_MPI_VENC_ReleaseStream(m_venc_chn, &stream);
                if (ret != HI_SUCCESS)
                {
                    printf("HI_MPI_VENC_ReleaseStream failed!\n");
                    free(stream.pstPack);
                    break;
                }

                free(stream.pstPack);
                stream.pstPack = NULL;
            }
        }
    }
}

// vi模块初始化
int PlayMipiCamera::initVi()
{
    memset(&m_vi_config, 0, sizeof(SAMPLE_VI_CONFIG_S));

    m_vi_config.s32WorkingViNum = 1;
    m_vi_config.as32WorkingViId[0] = 0;
    // sensor
    m_vi_config.astViInfo[0].stSnsInfo.enSnsType = m_sensor_type;
    m_vi_config.astViInfo[0].stSnsInfo.MipiDev = 0;
    m_vi_config.astViInfo[0].stSnsInfo.s32BusId = 0;
    m_vi_config.astViInfo[0].stSnsInfo.s32SnsId = 0;
    // dev
    m_vi_config.astViInfo[0].stDevInfo.ViDev = 0;
    m_vi_config.astViInfo[0].stDevInfo.enWDRMode = WDR_MODE_NONE;
    // pipe
    m_vi_config.astViInfo[0].stPipeInfo.aPipe[0] = 0;
    m_vi_config.astViInfo[0].stPipeInfo.aPipe[1] = -1;
    m_vi_config.astViInfo[0].stPipeInfo.enMastPipeMode = VI_OFFLINE_VPSS_ONLINE;
    m_vi_config.astViInfo[0].stPipeInfo.bMultiPipe = HI_FALSE;
    m_vi_config.astViInfo[0].stPipeInfo.bVcNumCfged = HI_FALSE;
    // channel
    m_vi_config.astViInfo[0].stChnInfo.ViChn = 0;
    m_vi_config.astViInfo[0].stChnInfo.enDynamicRange = DYNAMIC_RANGE_SDR8;
    m_vi_config.astViInfo[0].stChnInfo.enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;
    m_vi_config.astViInfo[0].stChnInfo.enVideoFormat = VIDEO_FORMAT_LINEAR;
    m_vi_config.astViInfo[0].stChnInfo.enCompressMode = COMPRESS_MODE_NONE;

    HI_S32 s32Ret = SAMPLE_COMM_VI_StartVi(&m_vi_config);
    if (HI_SUCCESS != s32Ret)
    {
        printf("SAMPLE_COMM_VI_StartVi failed with %d!\n", s32Ret);
        return -1;
    }

    return 0;
}

int PlayMipiCamera::uninitVi()
{
    SAMPLE_COMM_VI_StopVi(&m_vi_config);
    return 0;
}

// vpss模块初始化
int PlayMipiCamera::initVpss()
{
    // 只使用一个group
    m_vpss_grp = 0;
    m_vpss_chn = 1;

    HI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {HI_FALSE,HI_FALSE,HI_FALSE}; //channel0,channel1 每个grup有4个chn
    abChnEnable[m_vpss_chn] = HI_TRUE;

    VPSS_GRP_ATTR_S stVpssGrpAttr;
    VPSS_CHN_ATTR_S stVpssChnAttr[VPSS_MAX_PHY_CHN_NUM];

    //设置vpss组属性,指接收数据时控制码率
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.enPixelFormat  = PIXEL_FORMAT_YVU_SEMIPLANAR_420; //YVU420
    stVpssGrpAttr.u32MaxW        = m_width;
    stVpssGrpAttr.u32MaxH        = m_height;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.bNrEn = HI_TRUE;
    stVpssGrpAttr.stNrAttr.enNrType       = VPSS_NR_TYPE_VIDEO;
    stVpssGrpAttr.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL;
    stVpssGrpAttr.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME;

    for(int i=0; i<VPSS_MAX_PHY_CHN_NUM; i++)
    {
        if(abChnEnable[i] == HI_TRUE)
        {
            stVpssChnAttr[i].u32Width                     = 1920;
            stVpssChnAttr[i].u32Height                    = 1080;
            stVpssChnAttr[i].enChnMode                    = VPSS_CHN_MODE_USER;
            stVpssChnAttr[i].enCompressMode               = COMPRESS_MODE_NONE;
            stVpssChnAttr[i].enDynamicRange               = stVpssGrpAttr.enDynamicRange;
            stVpssChnAttr[i].enPixelFormat                = stVpssGrpAttr.enPixelFormat;
            stVpssChnAttr[i].stFrameRate.s32SrcFrameRate  = stVpssGrpAttr.stFrameRate.s32SrcFrameRate;  //控制channel输出码率
            stVpssChnAttr[i].stFrameRate.s32DstFrameRate  = stVpssGrpAttr.stFrameRate.s32DstFrameRate;
            stVpssChnAttr[i].u32Depth                     = 3; // 这里不能为0,否则无法从vpss中获取图像
            stVpssChnAttr[i].bMirror                      = HI_FALSE;
            stVpssChnAttr[i].bFlip                        = HI_FALSE;
            stVpssChnAttr[i].enVideoFormat                = VIDEO_FORMAT_LINEAR;
            stVpssChnAttr[i].stAspectRatio.enMode         = ASPECT_RATIO_NONE;
        }
    }

    //已初始化的组属性以及channel属性，来创建grp和chn,并启动
    HI_S32 s32Ret = SAMPLE_COMM_VPSS_Start(m_vpss_grp, abChnEnable,&stVpssGrpAttr,stVpssChnAttr);
    if(s32Ret != HI_SUCCESS)
    {
        printf("start VPSS fail for %#x!\n", s32Ret);
        return -1;
    }

    return 0;
}

int PlayMipiCamera::uninitVpss()
{
    HI_BOOL abChnEnable[VPSS_MAX_PHY_CHN_NUM] = {HI_FALSE,HI_FALSE,HI_FALSE}; //channel0,channel1 每个grup有4个chn
    abChnEnable[m_vpss_chn] = HI_TRUE;
    SAMPLE_COMM_VPSS_Stop(m_vpss_grp, abChnEnable);

    return 0;
}

int PlayMipiCamera::initVenc()
{
    VENC_GOP_MODE_E enGopMode = VENC_GOPMODE_NORMALP;
    VENC_GOP_ATTR_S stGopAttr;
    HI_S32 s32Ret = SAMPLE_COMM_VENC_GetGopAttr(enGopMode,&stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("Venc Get GopAttr for %#x!\n", s32Ret);
        return -1;
    }

    s32Ret = SAMPLE_COMM_VENC_Start(m_venc_chn, PT_H264, PIC_1080P, SAMPLE_RC_CBR,0, HI_TRUE, &stGopAttr);
    if (HI_SUCCESS != s32Ret)
    {
        printf("Venc Start failed for %#x!\n", s32Ret);
        return -1;
    }
    return 0;
}
int PlayMipiCamera::uninitVenc()
{
    SAMPLE_COMM_VENC_Stop(m_venc_chn);
    return 0;
}