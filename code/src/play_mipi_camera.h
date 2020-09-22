#ifndef _PLAY_MIPI_CAMERA_H_
#define _PLAY_MIPI_CAMERA_H_

#include <pthread.h>
#include "sample_comm.h"
#include "push_stream.h"

class PlayMipiCamera
{
public:
    PlayMipiCamera();
    ~PlayMipiCamera();

    int Init();
    int UnInit();

    void PlayThread();

private:
    int initVi();
    int uninitVi();

    int initVpss();
    int uninitVpss();

    int initVenc();
    int uninitVenc();

    SAMPLE_SNS_TYPE_E   m_sensor_type;
    VPSS_GRP            m_vpss_grp;
    VPSS_CHN            m_vpss_chn;
    PIC_SIZE_E          m_pic_size_type;
    SIZE_S              m_pic_size;
    SAMPLE_VI_CONFIG_S  m_vi_config;
    VENC_CHN            m_venc_chn;
    int m_width;
    int m_height;
    pthread_t m_play_tid;
    PushStream m_push_steam;
};

#endif // end of _PLAY_MIPI_CAMERA_H_