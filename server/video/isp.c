#include "isp.h"
#include "../../server/device/gk_gpio.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static int last_day_night_mode = 0;

static ISO_LEVEL iso_level_get(HI_S32 iso)
{
    ISO_LEVEL level;

    //iso最低100，所以最低档为100 ~200,不包含200
    if(iso < 200)
    {
        level = ISO_LEVEL_0;
    }
    else if(iso < 400)
    {
        level = ISO_LEVEL_1;
    }
    else if(iso < 800)
    {
        level = ISO_LEVEL_2;
    }
    else if(iso < 1600)
    {
        level = ISO_LEVEL_3;
    }
    else if(iso < 3200)
    {
        level = ISO_LEVEL_4;
    }
    else if(iso < 6400)
    {
        level = ISO_LEVEL_5;
    }
    else if(iso < 12800)
    {
        level = ISO_LEVEL_6;
    }
    else if(iso < 25600)
    {
        level = ISO_LEVEL_7;
    }
    else if(iso < 51200)
    {
        level = ISO_LEVEL_8;
    }
    else if(iso < 102400)
    {
        level = ISO_LEVEL_9;
    }
    else if(iso < 204800)
    {
        level = ISO_LEVEL_10;
    }
    else if(iso < 409600)
    {
        level = ISO_LEVEL_11;
    }
    else if(iso < 819200)
    {
        level = ISO_LEVEL_12;
    }
    else if(iso < 1638400)
    {
        level = ISO_LEVEL_13;
    }
    else if(iso < 3276800)
    {
        level = ISO_LEVEL_14;
    }
    else
    {
        level = ISO_LEVEL_15;
    }

    return level;
}

int video_isp_proc(void *arg) {
    ISP_DEV IspDev = 0;
    int iso;
    ISO_LEVEL iso_level = ISO_LEVEL_BUTT;
    int mode = -1,framerate = 30,ii,framerate_last = -1;
    float tmp;
    int ret;

    VENC_CHN_PARAM_S venc_param;
    ISP_EXP_INFO_S stExpInfo;

    pthread_mutex_lock(&mutex);
    ret = HI_MPI_ISP_QueryExposureInfo(IspDev, &stExpInfo);
    tmp = (stExpInfo.u32AGain / 1024.0) * (stExpInfo.u32DGain / 1024.0) * (stExpInfo.u32ISPDGain / 1024.0);
    iso = tmp * 100;
    iso_level = iso_level_get(iso);
    switch(iso_level)
    {
        case ISO_LEVEL_0:
        case ISO_LEVEL_1:
        {
            framerate = 30;
        }
            break;
        case ISO_LEVEL_2:
        {
        }
            break;
        case ISO_LEVEL_3:
        {
            framerate = 20;
        }
            break;
        case ISO_LEVEL_4:
        {
        }
            break;
            break;
        case ISO_LEVEL_5:
        {
            framerate = 15;
        }
            break;
        case ISO_LEVEL_6:
        {

        }
            break;
        case ISO_LEVEL_7:

        {
            framerate = 10;
        }
            break;
        case ISO_LEVEL_8:
        {
        }
            break;
        case ISO_LEVEL_9:

        {
            framerate = 7;
        }
            break;
        case ISO_LEVEL_10:
        {
        }
            break;
        default:
        {
            framerate = 5;
        }
            break;
    }

    if(_config_.day_night_mode == 2)
    {
        if(iso_level > ISO_LEVEL_4) {//切换到黑夜模式
            if( last_day_night_mode!=1 ) {
                last_day_night_mode = 1;
                gk_ircut_nightmode();

                for (ii = 0; ii < 3; ii++) {
                    HI_MPI_VENC_GetChnParam(ii, &venc_param);
                    venc_param.bColor2Grey = 1;
                    HI_MPI_VENC_SetChnParam(ii, &venc_param);
                }
                usleep(200000);
                gk_ircut_release();
                log_goke( DEBUG_INFO, "switched to night mode ,framerate=%d, iso level = %d ", framerate, iso_level);
            }
        } else if(iso_level <= ISO_LEVEL_3)
        {//切换到白天模式
            if( last_day_night_mode!=0 ) {
                last_day_night_mode = 0;

                gk_ircut_daymode();
                for (ii = 0; ii < 3; ii++) {
                    HI_MPI_VENC_GetChnParam(ii, &venc_param);
                    venc_param.bColor2Grey = 0;
                    HI_MPI_VENC_SetChnParam(ii, &venc_param);
                }
                usleep(200000);
                gk_ircut_release();
                log_goke( DEBUG_INFO, "switched to day mode ,framerate=%d, iso level = %d", framerate, iso_level);
            }
        }
    }

    pthread_mutex_unlock(&mutex);
    usleep(500*1000);

    return 0;
}