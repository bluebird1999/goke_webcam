#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "../../../common/tools_interface.h"
#include "../../video/config.h"
#include "hisi.h"

HI_S32 hisi_init_vpss(video_config_t *config)
{
    VPSS_CHN VpssChn;
    HI_S32 ret;
    HI_S32 j;
    HI_BOOL bWrapEn;
    VPSS_CHN_BUF_WRAP_S stVpssChnBufWrap;
    VPSS_LDCV3_ATTR_S    stLDCV3Attr;

    //config
    ret = HI_MPI_VPSS_CreateGrp(config->vpss.group, &config->vpss.group_info);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING,"HI_MPI_VPSS_CreateGrp(grp:%d) failed with %#x!", config->vpss.group, ret);
        return HI_FAILURE;
    }

    if( config->vpss.ldc ) {
        stLDCV3Attr.bEnable = 1;
        stLDCV3Attr.stAttr.enViewType = LDC_VIEW_TYPE_ALL;   //LDC_VIEW_TYPE_CROP;
        stLDCV3Attr.stAttr.s32CenterXOffset = 0;
        stLDCV3Attr.stAttr.s32CenterYOffset = 0;
        stLDCV3Attr.stAttr.s32DistortionRatio = 200;
        stLDCV3Attr.stAttr.s32MinRatio = 0;
    }
    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
        if(HI_TRUE == config->vpss.enabled[j]) {
            bWrapEn = (j==ID_HIGH)? config->vpss.buff_wrap : 0;
            VpssChn = j;
            ret = HI_MPI_VPSS_SetChnAttr(config->vpss.group, VpssChn, &config->vpss.channel_info[VpssChn]);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING,"VpssChn:%d;HI_MPI_VPSS_SetChnAttr failed with %#x",VpssChn, ret);
                return HI_FAILURE;
            }
            if (bWrapEn) {
                config->vpss.wrap_buffer_line = hisi_get_vpss_wrap_buf_line(config);
                stVpssChnBufWrap.u32WrapBufferSize = VPSS_GetWrapBufferSize(config->profile.stream[ID_HIGH].width,
                                                                            config->profile.stream[ID_HIGH].height,
                                                                            config->vpss.wrap_buffer_line,
                                                                            config->vpss.group_info.enPixelFormat,
                                                                            DATA_BITWIDTH_8,
                                                                            COMPRESS_MODE_NONE,
                                                                            DEFAULT_ALIGN);
                stVpssChnBufWrap.bEnable = 1;
                stVpssChnBufWrap.u32BufLine = config->vpss.wrap_buffer_line;
                ret = HI_MPI_VPSS_SetChnBufWrapAttr(config->vpss.group, VpssChn, &stVpssChnBufWrap);
                if (ret != HI_SUCCESS) {
                    log_goke( DEBUG_WARNING, "HI_MPI_VPSS_SetChnBufWrapAttr Chn %d failed with %#x", VpssChn, ret);
                    return HI_FAILURE;
                }
            }
            if( config->vpss.ldc ) {
 //               ret = HI_MPI_VPSS_SetChnLDCV3Attr(config->vpss.group, VpssChn, &stLDCV3Attr);
                if (HI_SUCCESS != ret) {
                    log_goke( DEBUG_WARNING, "HI_MPI_VPSS_SetChnLDCV3Attr failed witfh %x\n", ret);
                    return HI_FAILURE;
                }
            }
        }
    }
    return HI_SUCCESS;
}

HI_S32 hisi_start_vpss(video_config_t *config)
{
    HI_S32 ret;
    int j;

    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
        if (HI_TRUE == config->vpss.enabled[j]) {
            ret = HI_MPI_VPSS_EnableChn(config->vpss.group, j);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING, "HI_MPI_VPSS_EnableChn failed with %#x", ret);
                return HI_FAILURE;
            }
        }
    }
    ret = HI_MPI_VPSS_StartGrp(config->vpss.group);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING,"HI_MPI_VPSS_StartGrp failed with %#x", ret);
        return HI_FAILURE;
    }
    //bind vi and vpss
    ret = hisi_bind_vi_vpss(config->vi.pipe_info.pipe[0], config->vi.channel_info.channel,
                            config->vpss.group);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "vi bind vpss failed. ret: 0x%x !", ret);
        return ret;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_stop_vpss(video_config_t *config)
{
    HI_S32 j;
    HI_S32 ret = HI_SUCCESS;
    VPSS_CHN VpssChn;
    //unbind
    ret = hisi_unbind_vi_vpss(config->vi.pipe_info.pipe[0],
                              config->vi.channel_info.channel, config->vpss.group);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING,"failed with %#x!", ret);
        return HI_FAILURE;
    }
    //stop
    for (j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
        if(HI_TRUE == config->vpss.enabled[j]) {
            VpssChn = j;
            ret = HI_MPI_VPSS_DisableChn(config->vpss.group, VpssChn);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING,"failed with %#x!", ret);
                return HI_FAILURE;
            }
        }
    }

    ret = HI_MPI_VPSS_StopGrp(config->vpss.group);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING,"failed with %#x!", ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_vpss(video_config_t *config)
{
    HI_S32 ret;
    ret = HI_MPI_VPSS_DestroyGrp(config->vpss.group);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING,"failed with %#x!", ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}