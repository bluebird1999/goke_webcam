#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include "../../../common/tools_interface.h"
#include "hisi.h"

HI_S32 hisi_get_gop_attr(VENC_GOP_MODE_E enGopMode,VENC_GOP_ATTR_S *pstGopAttr)
{
    switch(enGopMode)
    {
        case VENC_GOPMODE_NORMALP:
            pstGopAttr->enGopMode  = VENC_GOPMODE_NORMALP;
            pstGopAttr->stNormalP.s32IPQpDelta = 2;
            break;
        case VENC_GOPMODE_SMARTP:
            pstGopAttr->enGopMode  = VENC_GOPMODE_SMARTP;
            pstGopAttr->stSmartP.s32BgQpDelta  = 4;
            pstGopAttr->stSmartP.s32ViQpDelta  = 2;
            pstGopAttr->stSmartP.u32BgInterval =  90;
            break;

        case VENC_GOPMODE_DUALP:
            pstGopAttr->enGopMode  = VENC_GOPMODE_DUALP;
            pstGopAttr->stDualP.s32IPQpDelta  = 4;
            pstGopAttr->stDualP.s32SPQpDelta  = 2;
            pstGopAttr->stDualP.u32SPInterval = 3;
            break;

        case VENC_GOPMODE_BIPREDB:
            pstGopAttr->enGopMode  = VENC_GOPMODE_BIPREDB;
            pstGopAttr->stBipredB.s32BQpDelta  = -2;
            pstGopAttr->stBipredB.s32IPQpDelta = 3;
            pstGopAttr->stBipredB.u32BFrmNum   = 2;
            log_goke(DEBUG_WARNING,"BIPREDB and ADVSMARTP not support lowdelay!");
            return HI_FAILURE;
            break;

        default:
            log_goke(DEBUG_WARNING,"not support the gop mode ");
            return HI_FAILURE;
            break;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_close_re_encoder(VENC_CHN VencChn)
{
    HI_S32 ret;
    VENC_RC_PARAM_S stRcParam;
    VENC_CHN_ATTR_S stChnAttr;

    ret = HI_MPI_VENC_GetChnAttr(VencChn,&stChnAttr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"GetChnAttr failed");
        return HI_FAILURE;
    }

    ret = HI_MPI_VENC_GetRcParam(VencChn,&stRcParam);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"GetRcParam failed");
        return HI_FAILURE;
    }

    if(VENC_RC_MODE_H264CBR == stChnAttr.stRcAttr.enRcMode) {
        stRcParam.stParamH264Cbr.s32MaxReEncodeTimes = 0;
    }
    else if(VENC_RC_MODE_H264VBR == stChnAttr.stRcAttr.enRcMode) {
        stRcParam.stParamH264Vbr.s32MaxReEncodeTimes = 0;
    }
    else if(VENC_RC_MODE_H265CBR == stChnAttr.stRcAttr.enRcMode) {
        stRcParam.stParamH264Cbr.s32MaxReEncodeTimes = 0;
    }
    else if(VENC_RC_MODE_H265VBR == stChnAttr.stRcAttr.enRcMode) {
        stRcParam.stParamH264Vbr.s32MaxReEncodeTimes = 0;
    }
    else {
        return HI_SUCCESS;
    }
    ret = HI_MPI_VENC_SetRcParam(VencChn,&stRcParam);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"SetRcParam failed");
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_create_video_encoder_channel(video_config_t *config, VENC_CHN VencChn, VENC_GOP_ATTR_S *pstGopAttr)
{
    HI_S32 ret;
    VENC_CHN_ATTR_S        stVencChnAttr;
    VENC_ATTR_JPEG_S       stJpegAttr;
    HI_U32                 u32StatTime;
    HI_U32                 u32FrameRate;

    /******************************************
     step 1:  Create Venc Channel
    ******************************************/
    stVencChnAttr.stVencAttr.enType          = config->profile.stream[VencChn].type;
    stVencChnAttr.stVencAttr.u32MaxPicWidth  = config->profile.stream[VencChn].width;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = config->profile.stream[VencChn].height;
    stVencChnAttr.stVencAttr.u32PicWidth     = config->profile.stream[VencChn].width;
    stVencChnAttr.stVencAttr.u32PicHeight    = config->profile.stream[VencChn].height;

    if (config->profile.stream[VencChn].type == PT_MJPEG || config->profile.stream[VencChn].type == PT_JPEG) {
        stVencChnAttr.stVencAttr.u32BufSize      = ALIGN_UP(config->profile.stream[VencChn].width, 16) *
                                                ALIGN_UP(config->profile.stream[VencChn].height, 16);
    }
    else {
        stVencChnAttr.stVencAttr.u32BufSize      = ALIGN_UP(config->profile.stream[VencChn].width *
                                                config->profile.stream[VencChn].height * 3 / 2, 64);/*stream buffer size*/
    }
    stVencChnAttr.stVencAttr.u32Profile      = config->profile.stream[VencChn].profile;
    stVencChnAttr.stVencAttr.bByFrame        = HI_TRUE;/*get stream mode is slice mode or frame mode?*/

    if(VENC_GOPMODE_SMARTP == pstGopAttr->enGopMode) {
        u32StatTime = pstGopAttr->stSmartP.u32BgInterval/config->profile.stream[VencChn].gop;
    }
    else {
        u32StatTime = 1;
    }

    u32FrameRate = config->profile.stream[VencChn].frame_rate;
    switch (config->profile.stream[VencChn].type) {
        case PT_H265: {
            if (RC_MODE_CBR == config->profile.stream[VencChn].rc_mode ) {
                VENC_H265_CBR_S    stH265Cbr;
                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
                stH265Cbr.u32Gop            = config->profile.stream[VencChn].gop;
                stH265Cbr.u32StatTime       = u32StatTime; /* stream rate statics time(s) */
                stH265Cbr.u32SrcFrameRate   = u32FrameRate; /* input (vi) frame rate */
                stH265Cbr.fr32DstFrameRate  = u32FrameRate; /* target frame rate */
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH265Cbr.u32BitRate = 1024 + 512*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH265Cbr.u32BitRate = 1024 * 2 + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH265Cbr.u32BitRate = 1024 * 2 + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH265Cbr.u32BitRate = 1024 * 3 + 3072*u32FrameRate/30;
                        break;
                    case PIC_3840x2160:
                        stH265Cbr.u32BitRate = 1024 * 5  + 5120*u32FrameRate/30;
                        break;
                    default : //other small resolution
                        stH265Cbr.u32BitRate = 256;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH265Cbr, &stH265Cbr, sizeof(VENC_H265_CBR_S));
            }
            else if (RC_MODE_FIXQP == config->profile.stream[VencChn].rc_mode) {
                VENC_H265_FIXQP_S    stH265FixQp;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265FIXQP;
                stH265FixQp.u32Gop              = 30;
                stH265FixQp.u32SrcFrameRate     = u32FrameRate;
                stH265FixQp.fr32DstFrameRate    = u32FrameRate;
                stH265FixQp.u32IQp              = 25;
                stH265FixQp.u32PQp              = 30;
                stH265FixQp.u32BQp              = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH265FixQp, &stH265FixQp, sizeof(VENC_H265_FIXQP_S));
            }
            else if (RC_MODE_VBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H265_VBR_S    stH265Vbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
                stH265Vbr.u32Gop           = config->profile.stream[VencChn].gop;
                stH265Vbr.u32StatTime      = u32StatTime;
                stH265Vbr.u32SrcFrameRate  = u32FrameRate;
                stH265Vbr.fr32DstFrameRate = u32FrameRate;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH265Vbr.u32MaxBitRate = 1024 + 512*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH265Vbr.u32MaxBitRate = 1024 * 2 + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH265Vbr.u32MaxBitRate = 1024 * 2 + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH265Vbr.u32MaxBitRate = 1024 * 3 + 3072*u32FrameRate/30;
                        break;
                    default :
                        stH265Vbr.u32MaxBitRate    = 1024 * 15 + 2048*u32FrameRate/30;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH265Vbr, &stH265Vbr, sizeof(VENC_H265_VBR_S));
            }
            else if(RC_MODE_AVBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H265_AVBR_S    stH265AVbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265AVBR;
                stH265AVbr.u32Gop         = config->profile.stream[VencChn].gop;
                stH265AVbr.u32StatTime    = u32StatTime;
                stH265AVbr.u32SrcFrameRate  = u32FrameRate;
                stH265AVbr.fr32DstFrameRate = u32FrameRate;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH265AVbr.u32MaxBitRate = 1024 + 512*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH265AVbr.u32MaxBitRate = 1024 * 2 + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH265AVbr.u32MaxBitRate = 1024 * 2 + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH265AVbr.u32MaxBitRate = 1024 * 3 + 3072*u32FrameRate/30;
                        break;
                    case PIC_3840x2160:
                        stH265AVbr.u32MaxBitRate = 1024 * 5  + 5120*u32FrameRate/30;
                        break;
                    default :
                        stH265AVbr.u32MaxBitRate    = 1024 * 15 + 2048*u32FrameRate/30;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH265AVbr, &stH265AVbr, sizeof(VENC_H265_AVBR_S));
            }
            else if(RC_MODE_QVBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H265_QVBR_S    stH265QVbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265QVBR;
                stH265QVbr.u32Gop         = config->profile.stream[VencChn].gop;
                stH265QVbr.u32StatTime    = u32StatTime;
                stH265QVbr.u32SrcFrameRate  = u32FrameRate;
                stH265QVbr.fr32DstFrameRate = u32FrameRate;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH265QVbr.u32TargetBitRate = 1024 + 512*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH265QVbr.u32TargetBitRate= 1024 * 2 + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH265QVbr.u32TargetBitRate = 1024 * 2 + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH265QVbr.u32TargetBitRate = 1024 * 3 + 3072*u32FrameRate/30;
                        break;
                    case PIC_3840x2160:
                        stH265QVbr.u32TargetBitRate = 1024 * 5  + 5120*u32FrameRate/30;
                        break;
                    case PIC_4000x3000:
                        stH265QVbr.u32TargetBitRate = 1024 * 10 + 5120*u32FrameRate/30;
                        break;
                    case PIC_7680x4320:
                        stH265QVbr.u32TargetBitRate = 1024 * 20 + 5120*u32FrameRate/30;
                        break;
                    default :
                        stH265QVbr.u32TargetBitRate    = 1024 * 15 + 2048*u32FrameRate/30;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH265QVbr, &stH265QVbr, sizeof(VENC_H265_QVBR_S));
            }
            else if(RC_MODE_CVBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H265_CVBR_S    stH265CVbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265CVBR;
                stH265CVbr.u32Gop         = config->profile.stream[VencChn].gop;
                stH265CVbr.u32StatTime    = u32StatTime;
                stH265CVbr.u32SrcFrameRate  = u32FrameRate;
                stH265CVbr.fr32DstFrameRate = u32FrameRate;
                stH265CVbr.u32LongTermStatTime  = 1;
                stH265CVbr.u32ShortTermStatTime = u32StatTime;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH265CVbr.u32MaxBitRate = 1024 + 512*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024  + 512*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 256;
                        break;
                    case PIC_720P:
                        stH265CVbr.u32MaxBitRate         = 1024 * 3 + 1024*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024 * 2 + 1024*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 512;
                        break;
                    case PIC_1080P:
                        stH265CVbr.u32MaxBitRate         = 1024 * 2 + 2048*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024 * 2 + 2048*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 1024;
                        break;
                    case PIC_2592x1944:
                        stH265CVbr.u32MaxBitRate         = 1024 * 4 + 3072*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024 * 3 + 3072*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 1024*2;
                        break;
                    case PIC_3840x2160:
                        stH265CVbr.u32MaxBitRate         = 1024 * 8  + 5120*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024 * 5  + 5120*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 1024*3;
                        break;
                    case PIC_4000x3000:
                        stH265CVbr.u32MaxBitRate         = 1024 * 12  + 5120*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024 * 10 + 5120*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 1024*4;
                        break;
                    case PIC_7680x4320:
                        stH265CVbr.u32MaxBitRate         = 1024 * 24  + 5120*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024 * 20 + 5120*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 1024*6;
                        break;
                    default :
                        stH265CVbr.u32MaxBitRate         = 1024 * 24  + 5120*u32FrameRate/30;
                        stH265CVbr.u32LongTermMaxBitrate = 1024 * 15 + 2048*u32FrameRate/30;
                        stH265CVbr.u32LongTermMinBitrate = 1024*5;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH265CVbr, &stH265CVbr, sizeof(VENC_H265_CVBR_S));
            }
            else if(RC_MODE_QPMAP == config->profile.stream[VencChn].rc_mode) {
                VENC_H265_QPMAP_S    stH265QpMap;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H265QPMAP;
                stH265QpMap.u32Gop           = config->profile.stream[VencChn].gop;
                stH265QpMap.u32StatTime      = u32StatTime;
                stH265QpMap.u32SrcFrameRate  = u32FrameRate;
                stH265QpMap.fr32DstFrameRate = u32FrameRate;
                stH265QpMap.enQpMapMode      = VENC_RC_QPMAP_MODE_MEANQP;
                memcpy(&stVencChnAttr.stRcAttr.stH265QpMap, &stH265QpMap, sizeof(VENC_H265_QPMAP_S));
            }
            else {
                log_goke(DEBUG_WARNING,"%s,%d,enRcMode(%d) not support",__FUNCTION__,__LINE__,config->profile.stream[VencChn].rc_mode);
                return HI_FAILURE;
            }
            stVencChnAttr.stVencAttr.stAttrH265e.bRcnRefShareBuf = config->profile.enc_shared_buff;
        }
            break;
        case PT_H264: {
            if (RC_MODE_CBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H264_CBR_S    stH264Cbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
                stH264Cbr.u32Gop                = config->profile.stream[VencChn].gop; /*the interval of IFrame*/
                stH264Cbr.u32StatTime           = u32StatTime; /* stream rate statics time(s) */
                stH264Cbr.u32SrcFrameRate       = u32FrameRate; /* input (vi) frame rate */
                stH264Cbr.fr32DstFrameRate      = u32FrameRate; /* target frame rate */
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH264Cbr.u32BitRate = 512 + 1024*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH264Cbr.u32BitRate = 1024  + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH264Cbr.u32BitRate = 1024  + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH264Cbr.u32BitRate = 1024  + 3072*u32FrameRate/30;
                        break;
                    case PIC_3840x2160:
                        stH264Cbr.u32BitRate = 1024 + 5120*u32FrameRate/30;
                        break;
                    default :  //other small resolution
                        stH264Cbr.u32BitRate = 512;
                        break;
                }

                memcpy(&stVencChnAttr.stRcAttr.stH264Cbr, &stH264Cbr, sizeof(VENC_H264_CBR_S));
            }
            else if (RC_MODE_FIXQP == config->profile.stream[VencChn].rc_mode) {
                VENC_H264_FIXQP_S    stH264FixQp;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
                stH264FixQp.u32Gop           = 30;
                stH264FixQp.u32SrcFrameRate  = u32FrameRate;
                stH264FixQp.fr32DstFrameRate = u32FrameRate;
                stH264FixQp.u32IQp           = 25;
                stH264FixQp.u32PQp           = 30;
                stH264FixQp.u32BQp           = 32;
                memcpy(&stVencChnAttr.stRcAttr.stH264FixQp, &stH264FixQp, sizeof(VENC_H264_FIXQP_S));
            }
            else if (RC_MODE_VBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H264_VBR_S    stH264Vbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
                stH264Vbr.u32Gop           = config->profile.stream[VencChn].gop;
                stH264Vbr.u32StatTime      = u32StatTime;
                stH264Vbr.u32SrcFrameRate  = u32FrameRate;
                stH264Vbr.fr32DstFrameRate = u32FrameRate;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH264Vbr.u32MaxBitRate = 512 + 1024*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH264Vbr.u32MaxBitRate = 512   + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH264Vbr.u32MaxBitRate = 512   + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH264Vbr.u32MaxBitRate = 512   + 3072*u32FrameRate/30;
                        break;
                    case PIC_3840x2160:
                        stH264Vbr.u32MaxBitRate = 512   + 5120*u32FrameRate/30;
                        break;
                    default :
                        stH264Vbr.u32MaxBitRate = 512  + 2048*u32FrameRate/30;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH264Vbr, &stH264Vbr, sizeof(VENC_H264_VBR_S));
            }
            else if (RC_MODE_AVBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H264_VBR_S    stH264AVbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264AVBR;
                stH264AVbr.u32Gop           = config->profile.stream[VencChn].gop;
                stH264AVbr.u32StatTime      = u32StatTime;
                stH264AVbr.u32SrcFrameRate  = u32FrameRate;
                stH264AVbr.fr32DstFrameRate = u32FrameRate;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH264AVbr.u32MaxBitRate = 1024 + 512*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH264AVbr.u32MaxBitRate = 1024 * 2   + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH264AVbr.u32MaxBitRate = 1024 * 2   + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH264AVbr.u32MaxBitRate = 1024 * 3   + 3072*u32FrameRate/30;
                        break;
                    case PIC_3840x2160:
                        stH264AVbr.u32MaxBitRate = 1024 * 5   + 5120*u32FrameRate/30;
                        break;
                    case PIC_4000x3000:
                        stH264AVbr.u32MaxBitRate = 1024 * 10  + 5120*u32FrameRate/30;
                        break;
                    case PIC_7680x4320:
                        stH264AVbr.u32MaxBitRate = 1024 * 20  + 5120*u32FrameRate/30;
                        break;
                    default :
                        stH264AVbr.u32MaxBitRate = 1024 * 15  + 2048*u32FrameRate/30;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH264AVbr, &stH264AVbr, sizeof(VENC_H264_AVBR_S));
            }
            else if (RC_MODE_QVBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H264_QVBR_S    stH264QVbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264QVBR;
                stH264QVbr.u32Gop           = config->profile.stream[VencChn].gop;
                stH264QVbr.u32StatTime      = u32StatTime;
                stH264QVbr.u32SrcFrameRate  = u32FrameRate;
                stH264QVbr.fr32DstFrameRate = u32FrameRate;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH264QVbr.u32TargetBitRate = 1024 + 512*u32FrameRate/30;
                        break;
                    case PIC_360P:
                        stH264QVbr.u32TargetBitRate = 1024 * 2   + 1024*u32FrameRate/30;
                        break;
                    case PIC_720P:
                        stH264QVbr.u32TargetBitRate = 1024 * 2   + 1024*u32FrameRate/30;
                        break;
                    case PIC_1080P:
                        stH264QVbr.u32TargetBitRate = 1024 * 2   + 2048*u32FrameRate/30;
                        break;
                    case PIC_2592x1944:
                        stH264QVbr.u32TargetBitRate = 1024 * 3   + 3072*u32FrameRate/30;
                        break;
                    case PIC_3840x2160:
                        stH264QVbr.u32TargetBitRate = 1024 * 5   + 5120*u32FrameRate/30;
                        break;
                    case PIC_4000x3000:
                        stH264QVbr.u32TargetBitRate = 1024 * 10  + 5120*u32FrameRate/30;
                        break;
                    case PIC_7680x4320:
                        stH264QVbr.u32TargetBitRate = 1024 * 20  + 5120*u32FrameRate/30;
                        break;
                    default :
                        stH264QVbr.u32TargetBitRate = 1024 * 15  + 2048*u32FrameRate/30;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH264QVbr, &stH264QVbr, sizeof(VENC_H264_QVBR_S));
            }
            else if(RC_MODE_CVBR == config->profile.stream[VencChn].rc_mode) {
                VENC_H264_CVBR_S    stH264CVbr;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264CVBR;
                stH264CVbr.u32Gop         = config->profile.stream[VencChn].gop;
                stH264CVbr.u32StatTime    = u32StatTime;
                stH264CVbr.u32SrcFrameRate  = u32FrameRate;
                stH264CVbr.fr32DstFrameRate = u32FrameRate;
                stH264CVbr.u32LongTermStatTime  = 1;
                stH264CVbr.u32ShortTermStatTime = u32StatTime;
                switch (config->profile.stream[VencChn].size) {
                    case PIC_VGA:
                        stH264CVbr.u32MaxBitRate         = 1024 + 512*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 + 512*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 256;
                        break;
                    case PIC_720P:
                        stH264CVbr.u32MaxBitRate         = 1024 * 3 + 1024*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 * 2 + 1024*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 512;
                        break;
                    case PIC_1080P:
                        stH264CVbr.u32MaxBitRate         = 1024 * 2 + 2048*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 * 2 + 2048*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 1024;
                        break;
                    case PIC_2592x1944:
                        stH264CVbr.u32MaxBitRate         = 1024 * 4 + 3072*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 * 3 + 3072*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 1024*2;
                        break;
                    case PIC_3840x2160:
                        stH264CVbr.u32MaxBitRate         = 1024 * 8  + 5120*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 * 5  + 5120*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 1024*3;
                        break;
                    case PIC_4000x3000:
                        stH264CVbr.u32MaxBitRate         = 1024 * 12  + 5120*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 * 10 + 5120*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 1024*4;
                        break;
                    case PIC_7680x4320:
                        stH264CVbr.u32MaxBitRate         = 1024 * 24  + 5120*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 * 20 + 5120*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 1024*6;
                        break;
                    default :
                        stH264CVbr.u32MaxBitRate         = 1024 * 24  + 5120*u32FrameRate/30;
                        stH264CVbr.u32LongTermMaxBitrate = 1024 * 15 + 2048*u32FrameRate/30;
                        stH264CVbr.u32LongTermMinBitrate = 1024*5;
                        break;
                }
                memcpy(&stVencChnAttr.stRcAttr.stH264CVbr, &stH264CVbr, sizeof(VENC_H264_CVBR_S));
            }
            else if(RC_MODE_QPMAP == config->profile.stream[VencChn].rc_mode) {
                VENC_H264_QPMAP_S    stH264QpMap;

                stVencChnAttr.stRcAttr.enRcMode = VENC_RC_MODE_H264QPMAP;
                stH264QpMap.u32Gop           = config->profile.stream[VencChn].gop;
                stH264QpMap.u32StatTime      = u32StatTime;
                stH264QpMap.u32SrcFrameRate  = u32FrameRate;
                stH264QpMap.fr32DstFrameRate = u32FrameRate;
                memcpy(&stVencChnAttr.stRcAttr.stH264QpMap, &stH264QpMap, sizeof(VENC_H264_QPMAP_S));
            }
            else {
                log_goke(DEBUG_WARNING,"%s,%d,enRcMode(%d) not support",__FUNCTION__,__LINE__,config->profile.stream[VencChn].rc_mode);
                return HI_FAILURE;
            }
            stVencChnAttr.stVencAttr.stAttrH264e.bRcnRefShareBuf = config->profile.enc_shared_buff;
        }
            break;
        case PT_JPEG: {
            stJpegAttr.bSupportDCF = HI_FALSE;
            stJpegAttr.stMPFCfg.u8LargeThumbNailNum = 0;
            stJpegAttr.enReceiveMode = VENC_PIC_RECEIVE_SINGLE;
            memcpy(&stVencChnAttr.stVencAttr.stAttrJpege, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));
        }
            break;
        default:
            log_goke(DEBUG_WARNING,"cann't support this enType (%d) in this version!",config->profile.stream[VencChn].type);
            return HI_ERR_VENC_NOT_SUPPORT;
    }

    if(PT_MJPEG == config->profile.stream[VencChn].type || PT_JPEG == config->profile.stream[VencChn].type ) {
        stVencChnAttr.stGopAttr.enGopMode  = VENC_GOPMODE_NORMALP;
        stVencChnAttr.stGopAttr.stNormalP.s32IPQpDelta = 0;
    }
    else {
        memcpy(&stVencChnAttr.stGopAttr,pstGopAttr,sizeof(VENC_GOP_ATTR_S));
        if((VENC_GOPMODE_BIPREDB == pstGopAttr->enGopMode)&&(PT_H264 == config->profile.stream[VencChn].type)) {
            if(0 == stVencChnAttr.stVencAttr.u32Profile) {
                stVencChnAttr.stVencAttr.u32Profile = 1;
                log_goke(DEBUG_WARNING,"H.264 base profile not support BIPREDB, so change profile to main profile");
            }
        }

        if((VENC_RC_MODE_H264QPMAP == stVencChnAttr.stRcAttr.enRcMode) || 
                (VENC_RC_MODE_H265QPMAP == stVencChnAttr.stRcAttr.enRcMode)) {
            if(VENC_GOPMODE_ADVSMARTP == pstGopAttr->enGopMode) {
                stVencChnAttr.stGopAttr.enGopMode = VENC_GOPMODE_SMARTP;
                log_goke(DEBUG_WARNING,"advsmartp not support QPMAP, so change gopmode to smartp");
            }
        }
    }

    ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"HI_MPI_VENC_CreateChn [%d] faild with %#x! ===", \
                   VencChn, ret);
        return ret;
    }

    ret = hisi_close_re_encoder(VencChn);
    if (HI_SUCCESS != ret) {
        HI_MPI_VENC_DestroyChn(VencChn);
        return ret;
    }

    return HI_SUCCESS;
}

HI_S32 hisi_init_video_encoder(video_config_t *config, int stream)
{
    HI_S32 ret;
    VENC_RECV_PIC_PARAM_S  stRecvParam;
    VENC_GOP_ATTR_S pstGopAttr;
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;
    VPSS_CHN VpssChn = config->profile.stream[stream].vpss_chn;
    /**/
    ret = hisi_get_gop_attr( config->profile.stream[VencChn].gop_mode, &pstGopAttr);
    if (HI_SUCCESS != ret) {
        log_goke( DEBUG_WARNING, "Venc Get GopAttr for %#x!", ret);
        return ret;
    }
    /******************************************
     step 1:  Creat Encode Chnl
    ******************************************/
    ret = hisi_create_video_encoder_channel(config,VencChn,&pstGopAttr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"SAMPLE_COMM_VENC_Creat faild with%#x! ", ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_start_video_encoder(video_config_t *config, int stream)
{
    HI_S32 ret;
    VENC_RECV_PIC_PARAM_S  stRecvParam;
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;
    VPSS_CHN VpssChn = config->profile.stream[stream].vpss_chn;
    /******************************************
     step 2:  Start Recv Venc Pictures
    ******************************************/
    stRecvParam.s32RecvPicNum = -1;
    ret = HI_MPI_VENC_StartRecvFrame(VencChn,&stRecvParam);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"HI_MPI_VENC_StartRecvPic faild with%#x! ", ret);
        return HI_FAILURE;
    }
    /******************************************
     step 2:  bind
    ******************************************/
    ret = hisi_bind_vpss_venc(config->vpss.group, VpssChn, VencChn);
    if (HI_SUCCESS != ret) {
        log_goke( DEBUG_WARNING, "Venc bind Vpss failed for %#x!", ret);
        HI_MPI_VENC_DestroyChn(VencChn);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_stop_video_encoder(video_config_t *config, int stream)
{
    HI_S32 ret;
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;
    VPSS_CHN VpssChn = config->profile.stream[stream].vpss_chn;
    //unbind
    ret = hisi_unbind_vpss_venc(config->vpss.group, VpssChn, VencChn);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"HI_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!", \
                   VencChn, ret);
        return HI_FAILURE;
    }
    //stop
    ret = HI_MPI_VENC_StopRecvFrame(VencChn);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"HI_MPI_VENC_StopRecvPic vechn[%d] failed with %#x!", \
                   VencChn, ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_video_encoder(video_config_t *config, int stream )
{
    HI_S32 ret;
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;

    ret = HI_MPI_VENC_DestroyChn(VencChn);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"HI_MPI_VENC_DestroyChn vechn[%d] failed with %#x!", \
                   VencChn, ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_init_snap_encoder(video_config_t *config, int stream, HI_BOOL bSupportDCF)
{
    HI_S32 ret;
    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;

    /******************************************
     step 1:  Create Venc Channel
    ******************************************/
    stVencChnAttr.stVencAttr.enType             = PT_JPEG;
    stVencChnAttr.stVencAttr.u32MaxPicWidth     = config->profile.stream[VencChn].width;
    stVencChnAttr.stVencAttr.u32MaxPicHeight    = config->profile.stream[VencChn].height;
    stVencChnAttr.stVencAttr.u32PicWidth        = config->profile.stream[VencChn].width;
    stVencChnAttr.stVencAttr.u32PicHeight       = config->profile.stream[VencChn].height;
    stVencChnAttr.stVencAttr.u32BufSize         = ALIGN_UP(config->profile.stream[VencChn].width, 16) *
                                                  ALIGN_UP(config->profile.stream[VencChn].height,16);
    stVencChnAttr.stVencAttr.bByFrame           = HI_TRUE;/*get stream mode is field mode  or frame mode*/
    stVencChnAttr.stVencAttr.stAttrJpege.bSupportDCF = bSupportDCF;
    //stVencChnAttr.stVencAttr.stAttrJpege.bSupportXMP = HI_FALSE;
    stVencChnAttr.stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;
    stVencChnAttr.stVencAttr.stAttrJpege.enReceiveMode                = VENC_PIC_RECEIVE_SINGLE;
    stVencChnAttr.stVencAttr.u32Profile = 0;

    ret = HI_MPI_VENC_CreateChn(VencChn, &stVencChnAttr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"HI_MPI_VENC_CreateChn [%d] faild with %#x!", \
                   VencChn, ret);
        return ret;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_start_snap_encoder(video_config_t *config, int stream)
{
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;
    VPSS_CHN VpssChn = config->profile.stream[stream].vpss_chn;
    //start

    //bind
    hisi_bind_vpss_venc(config->vpss.group, VpssChn, VencChn);
    return HI_SUCCESS;
}

HI_S32 hisi_stop_snap_encoder(video_config_t *config, int stream)
{
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;
    VPSS_CHN VpssChn = config->profile.stream[stream].vpss_chn;
    //unbind
    hisi_unbind_vpss_venc(config->vpss.group, VpssChn, VencChn);
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_snap_encoder(video_config_t *config, int stream )
{
    HI_S32 ret;
    VENC_CHN VencChn = config->profile.stream[stream].venc_chn;

    ret = HI_MPI_VENC_DestroyChn(VencChn);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING,"HI_MPI_VENC_DestroyChn vechn[%d] failed with %#x!", \
                   VencChn, ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}