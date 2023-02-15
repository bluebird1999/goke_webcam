#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/prctl.h>
#include <unistd.h>
#include "../../../common/tools_interface.h"
#include "../../video/config.h"
#include "hisi.h"

HI_S32 hisi_get_pic_size(picture_size_t enPicSize, SIZE_S *pstSize) {
    switch (enPicSize) {
        case PIC_CIF:   /* 352 * 288 */
            pstSize->u32Width = 352;
            pstSize->u32Height = 288;
            break;

        case PIC_360P:   /* 640 * 360 */
            pstSize->u32Width = 640;
            pstSize->u32Height = 360;
            break;

        case PIC_VGA:   /* 640 * 480 */
            pstSize->u32Width = 640;
            pstSize->u32Height = 480;
            break;
        case PIC_640x360:   /* 640 * 360 */
            pstSize->u32Width = 640;
            pstSize->u32Height = 360;
            break;

        case PIC_D1_PAL:   /* 720 * 576 */
            pstSize->u32Width = 720;
            pstSize->u32Height = 576;
            break;

        case PIC_D1_NTSC:   /* 720 * 480 */
            pstSize->u32Width = 720;
            pstSize->u32Height = 480;
            break;

        case PIC_720P:   /* 1280 * 720 */
            pstSize->u32Width = 1280;
            pstSize->u32Height = 720;
            break;

        case PIC_1080P:  /* 1920 * 1080 */
            pstSize->u32Width = 1920;
            pstSize->u32Height = 1080;
            break;

        case PIC_2304x1296:  /* 2304 * 1296 */
            pstSize->u32Width = 2304;
            pstSize->u32Height = 1296;
            break;

        case PIC_2592x1520:
            pstSize->u32Width = 2592;
            pstSize->u32Height = 1520;
            break;

        case PIC_2592x1944:
            pstSize->u32Width = 2592;
            pstSize->u32Height = 1944;
            break;

        case PIC_2592x1536:
            pstSize->u32Width = 2592;
            pstSize->u32Height = 1536;
            break;

        case PIC_2688x1520:
            pstSize->u32Width = 2688;
            pstSize->u32Height = 1520;
            break;

        case PIC_2716x1524:
            pstSize->u32Width = 2716;
            pstSize->u32Height = 1524;
            break;

        case PIC_3840x2160:
            pstSize->u32Width = 3840;
            pstSize->u32Height = 2160;
            break;

        case PIC_3000x3000:
            pstSize->u32Width = 3000;
            pstSize->u32Height = 3000;
            break;

        case PIC_4000x3000:
            pstSize->u32Width = 4000;
            pstSize->u32Height = 3000;
            break;

        case PIC_4096x2160:
            pstSize->u32Width = 4096;
            pstSize->u32Height = 2160;
            break;

        case PIC_7680x4320:
            pstSize->u32Width = 7680;
            pstSize->u32Height = 4320;
            break;
        case PIC_3840x8640:
            pstSize->u32Width = 3840;
            pstSize->u32Height = 8640;
            break;
        case PIC_2688x1536:
            pstSize->u32Width = 2688;
            pstSize->u32Height = 1536;
            break;
        case PIC_2688x1944:
            pstSize->u32Width = 2688;
            pstSize->u32Height = 1944;
            break;
        default:
            return HI_FAILURE;
    }

    return HI_SUCCESS;
}

HI_U32 hisi_get_vpss_wrap_buf_line(video_config_t *config) {
    HI_U32 vpssWrapBufLine = 0;
    VPSS_VENC_WRAP_PARAM_S wrapParam;

    memset(&wrapParam, 0, sizeof(VPSS_VENC_WRAP_PARAM_S));
    wrapParam.bAllOnline = (config->vpss.vi_vpss_mode == VI_ONLINE_VPSS_ONLINE) ? 1 : 0;
    wrapParam.u32FrameRate = config->vi.sensor_info.framerate;
    wrapParam.u32FullLinesStd = config->vi.sensor_info.full_line_height;
    wrapParam.stLargeStreamSize.u32Width = config->profile.stream[ID_HIGH].width;
    wrapParam.stLargeStreamSize.u32Height = config->profile.stream[ID_HIGH].height;
    wrapParam.stSmallStreamSize.u32Width = config->profile.stream[ID_LOW].width;
    wrapParam.stSmallStreamSize.u32Height = config->profile.stream[ID_LOW].height;

    if (HI_MPI_SYS_GetVPSSVENCWrapBufferLine(&wrapParam, &vpssWrapBufLine) != HI_SUCCESS) {
        log_goke( DEBUG_WARNING,
                "Error:Current BigStream(%dx%d@%d fps) and SmallStream(%dx%d@%d fps) not support Ring!== return 0x%x(0x%x)",
                wrapParam.stLargeStreamSize.u32Width, wrapParam.stLargeStreamSize.u32Height, wrapParam.u32FrameRate,
                wrapParam.stSmallStreamSize.u32Width, wrapParam.stSmallStreamSize.u32Height, wrapParam.u32FrameRate,
                HI_MPI_SYS_GetVPSSVENCWrapBufferLine(&wrapParam, &vpssWrapBufLine), HI_ERR_SYS_NOT_SUPPORT);
        vpssWrapBufLine = 0;
    }
    return vpssWrapBufLine;
}

static HI_VOID hisi_init_vb(video_config_t *config) {
    HI_U32 blk_size;
    int count = 0;
    //high stream
    if( config->profile.stream[ID_HIGH].enable) {
        if (config->vpss.buff_wrap) {
            blk_size = VPSS_GetWrapBufferSize(config->profile.stream[ID_HIGH].width,
                                              config->profile.stream[ID_HIGH].height,
                                              config->vpss.wrap_buffer_line,
                                              config->vpss.group_info.enPixelFormat,
                                              DATA_BITWIDTH_8,
                                              config->vpss.channel_info[0].enCompressMode,
                                              DEFAULT_ALIGN);
            config->vb.astCommPool[count].u64BlkSize = blk_size;
            config->vb.astCommPool[count].u32BlkCnt = 1;
        } else {
            blk_size = ALIGN_UP(config->profile.stream[ID_HIGH].width *
                             config->profile.stream[ID_HIGH].height * 3 / 2, 64);
            config->vb.astCommPool[count].u64BlkSize = blk_size;
            config->vb.astCommPool[count].u32BlkCnt = 3;
        }
        count++;
//        //osd
//        if( _config_.osd_enable ) {
//            blk_size = ALIGN_UP(config->region.overlay[ID_HIGH].size * config->region.overlay[ID_HIGH].size / 2
//                                * MAX_OSD_LABLE_SIZE * 3 / 2, 64);
//            config->vb.astCommPool[count].u64BlkSize = blk_size;
//            config->vb.astCommPool[count].u32BlkCnt = 2;
//            count++;
//        }
    }
    //low stream
    if( config->profile.stream[ID_LOW].enable) {
        blk_size = ALIGN_UP(config->profile.stream[ID_LOW].width *
                            config->profile.stream[ID_LOW].height * 3 / 2, 64);
        config->vb.astCommPool[count].u64BlkSize = blk_size;
        config->vb.astCommPool[count].u32BlkCnt = 3;
        count++;
//        //osd
//        if( _config_.osd_enable ) {
//            blk_size = ALIGN_UP(config->region.overlay[ID_LOW].size * config->region.overlay[ID_LOW].size / 2
//                                * MAX_OSD_LABLE_SIZE * 3 / 2, 64);
//            config->vb.astCommPool[count].u64BlkSize = blk_size;
//            config->vb.astCommPool[count].u32BlkCnt = 2;
//            count++;
//        }
    }
    //snap stream
    if( config->profile.stream[ID_PIC].enable) {
//        blk_size = ALIGN_UP(config->profile.stream[ID_PIC].width *
//                            config->profile.stream[ID_PIC].height * 3 / 2, 64);
//        config->vb.astCommPool[count].u64BlkSize = blk_size;
//        config->vb.astCommPool[count].u32BlkCnt = 3;
//        count++;
        //osd
//        if( _config_.osd_enable ) {
//            blk_size = ALIGN_UP(config->region.overlay[ID_PIC].size * config->region.overlay[ID_PIC].size / 2
//                                 * MAX_OSD_LABLE_SIZE * 3 / 2, 64);
//            config->vb.astCommPool[count].u64BlkSize = blk_size;
//            config->vb.astCommPool[count].u32BlkCnt = 2;
//            count++;
//        }
    }
    //MD stream
    if( config->profile.stream[ID_MD].enable) {
        blk_size = ALIGN_UP(config->profile.stream[ID_MD].width *
                            config->profile.stream[ID_MD].height * 3 / 2, 64);
        config->vb.astCommPool[count].u64BlkSize = blk_size;
        config->vb.astCommPool[count].u32BlkCnt = 6;
        count++;
    }
    config->vb.u32MaxPoolCnt = count;
}

HI_S32 hisi_init_sys(video_config_t *config) {
    HI_S32 ret = HI_FAILURE;
    HI_U32 blk_size;

    hisi_uninit_sys();
    if( config->vpss.buff_wrap ) {
        config->vpss.wrap_buffer_line = hisi_get_vpss_wrap_buf_line(config);
        if (config->vpss.wrap_buffer_line == 0) {
            return HI_FAILURE;
        }
    }
    hisi_init_vb(config);

    ret = HI_MPI_VB_SetConfig(&config->vb);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MPI_VB_SetConf failed");
                return HI_FAILURE;
    }

    ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MPI_VB_Init failed");
                return HI_FAILURE;
    }

    ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MPI_SYS_Init failed");
                HI_MPI_VB_Exit();
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_VOID hisi_uninit_sys(void) {
    HI_MPI_SYS_Exit();
    HI_MPI_VB_ExitModCommPool(VB_UID_VDEC);
    HI_MPI_VB_Exit();
    return;
}

HI_S32 hisi_bind_vi_vpss(VI_PIPE ViPipe, VI_CHN ViChn, VPSS_GRP VpssGrp) {
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    int ret;

    stSrcChn.enModId = HI_ID_VI;
    stSrcChn.s32DevId = ViPipe;
    stSrcChn.s32ChnId = ViChn;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;
    ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if( ret != HI_SUCCESS ) {
        log_goke( DEBUG_WARNING, "HI_MPI_SYS_Bind(VI-VPSS)");
    }

    return HI_SUCCESS;
}

HI_S32 hisi_unbind_vi_vpss(VI_PIPE ViPipe, VI_CHN ViChn, VPSS_GRP VpssGrp) {
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    int ret;

    stSrcChn.enModId = HI_ID_VI;
    stSrcChn.s32DevId = ViPipe;
    stSrcChn.s32ChnId = ViChn;

    stDestChn.enModId = HI_ID_VPSS;
    stDestChn.s32DevId = VpssGrp;
    stDestChn.s32ChnId = 0;

    ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if( ret != HI_SUCCESS ) {
        log_goke( DEBUG_WARNING, "HI_MPI_SYS_UnBind(VI-VPSS)");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_bind_vpss_venc(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VENC_CHN VencChn) {
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    int ret;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = VpssGrp;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VencChn;

    ret = HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
    if( ret != HI_SUCCESS ) {
        log_goke( DEBUG_WARNING, "HI_MPI_SYS_Bind(VPSS-VENC)");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_unbind_vpss_venc(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VENC_CHN VencChn) {
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;
    int ret;

    stSrcChn.enModId = HI_ID_VPSS;
    stSrcChn.s32DevId = VpssGrp;
    stSrcChn.s32ChnId = VpssChn;

    stDestChn.enModId = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = VencChn;

    ret = HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
    if( ret != HI_SUCCESS ) {
        log_goke( DEBUG_WARNING, "HI_MPI_SYS_UnBind(VPSS-VENC)");
    }

    return HI_SUCCESS;
}

HI_S32 hisi_update_vi_vpss_mode(VI_VPSS_MODE_S *pstVIVPSSMode) {
    HI_S32 i = 0;

    if (VI_OFFLINE_VPSS_OFFLINE == pstVIVPSSMode->aenMode[0]) {
        for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
            if (VI_OFFLINE_VPSS_ONLINE == pstVIVPSSMode->aenMode[i]) {
                for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
                    pstVIVPSSMode->aenMode[i] = VI_OFFLINE_VPSS_ONLINE;
                }
                break;
            }
        }
    } else if (VI_OFFLINE_VPSS_ONLINE == pstVIVPSSMode->aenMode[0]) {
        for (i = 0; i < VI_MAX_PIPE_NUM; i++) {
            pstVIVPSSMode->aenMode[i] = pstVIVPSSMode->aenMode[0];
        }
    }
    return HI_SUCCESS;
}

HI_S32 hisi_set_vi_vpss_mode(vi_info_t *config) {
    HI_S32 ret;
    VI_PIPE ViPipe;
    VI_VPSS_MODE_S stVIVPSSMode;

    ret = HI_MPI_SYS_GetVIVPSSMode(&stVIVPSSMode);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "Get VI-VPSS mode Param failed with %#x!", ret);
        return HI_FAILURE;
    }

    ViPipe = config->pipe_info.pipe[0];
    stVIVPSSMode.aenMode[ViPipe] = config->pipe_info.mast_pipe_mode;
    if ((config->pipe_info.multi_pipe == HI_TRUE)
        || (VI_OFFLINE_VPSS_ONLINE == config->pipe_info.mast_pipe_mode)) {
        hisi_update_vi_vpss_mode(&stVIVPSSMode);
        ViPipe = config->pipe_info.pipe[1];
        if (ViPipe != -1) {
            stVIVPSSMode.aenMode[ViPipe] = config->pipe_info.mast_pipe_mode;
        }
    }

    if ((config->snap_info.snap) && (config->snap_info.double_pipe)) {
        ViPipe = config->pipe_info.pipe[1];
        if (ViPipe != -1) {
            stVIVPSSMode.aenMode[ViPipe] = config->snap_info.snap_pipe_mode;
        }
    }

    ret = HI_MPI_SYS_SetVIVPSSMode(&stVIVPSSMode);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "Set VI-VPSS mode Param failed with %#x!", ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}