#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "../../../common/tools_interface.h"
#include "../../video/config.h"
#include "hisi.h"

/*
 *
 *      sensor and mipi
 *
 *
 */
HI_S32 hisi_init_mipi(vi_sensor_info_t *config) {
    HI_S32 ret = HI_SUCCESS;
    HI_S32 fd;
    combo_dev_attr_t stcomboDevAttr;
    sns_clk_source_t sns_dev = 0;

    fd = open(config->device, O_RDWR);
    if (fd < 0) {
        log_goke(DEBUG_WARNING, "open hi_mipi dev failed");
        return HI_FAILURE;
    }
    ret = ioctl(fd, MIPI_SET_HS_MODE, &config->lane_divide_mode);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MIPI_SET_HS_MODE failed");
        goto EXIT;
    }
    ret = ioctl(fd, MIPI_ENABLE_MIPI_CLOCK, &config->mipi_device);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "MIPI_ENABLE_CLOCK %d failed", &config->mipi_device);
        goto EXIT;
    }
    ret = ioctl(fd, MIPI_RESET_MIPI, &config->mipi_device);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "RESET_MIPI %d failed", &config->mipi_device);
        goto EXIT;
    }
    sns_dev = 0;
    ret = ioctl(fd, MIPI_ENABLE_SENSOR_CLOCK, &sns_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MIPI_ENABLE_SENSOR_CLOCK failed");
        goto EXIT;
    }
    ret = ioctl(fd, MIPI_RESET_SENSOR, &sns_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MIPI_RESET_SENSOR failed");
        goto EXIT;
    }
    memcpy_s(&stcomboDevAttr, sizeof(combo_dev_attr_t), &SENSOR_GC2063_CHN1_ATTR,
             sizeof(combo_dev_attr_t));
    stcomboDevAttr.devno = config->mipi_device;
    log_goke(DEBUG_WARNING, "============= MipiDev %d, SetMipiAttr", config->mipi_device);
    ret = ioctl(fd, MIPI_SET_DEV_ATTR, &stcomboDevAttr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "MIPI_SET_DEV_ATTR failed");
        goto EXIT;
    }
    ret = ioctl(fd, MIPI_UNRESET_MIPI, &config->mipi_device);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "UNRESET_MIPI %d failed", config->mipi_device);
        goto EXIT;
    }
    ret = ioctl(fd, MIPI_UNRESET_SENSOR, &sns_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MIPI_UNRESET_SENSOR failed");
        goto EXIT;
    }
    close(fd);
    return HI_SUCCESS;
    EXIT:
    close(fd);
    return HI_FAILURE;
}

HI_S32 hisi_start_mipi(vi_sensor_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_mipi(vi_sensor_info_t *config) {
    return HI_SUCCESS;
}


HI_S32 hisi_uninit_mipi(vi_sensor_info_t *config) {
    HI_S32 ret = HI_SUCCESS;
    HI_S32 fd;
    sns_clk_source_t sns_dev;

    fd = open(config->device, O_RDWR);
    if (fd < 0) {
        log_goke(DEBUG_WARNING, "open hi_mipi dev failed");
        return HI_FAILURE;
    }
    ret = ioctl(fd, MIPI_RESET_SENSOR, &sns_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MIPI_RESET_SENSOR failed");
        goto EXIT;
    }
    ret = ioctl(fd, MIPI_RESET_MIPI, &config->mipi_device);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "RESET_MIPI %d failed", &config->mipi_device);
        goto EXIT;
    }
    sns_dev = 0;
    ret = ioctl(fd, MIPI_DISABLE_SENSOR_CLOCK, &sns_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MIPI_DISABLE_SENSOR_CLOCK failed");
        goto EXIT;
    }
    ret = ioctl(fd, MIPI_DISABLE_MIPI_CLOCK, &config->mipi_device);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "MIPI_DISABLE_CLOCK %d failed", config->mipi_device);
        goto EXIT;
    }
    close(fd);
    return HI_SUCCESS;
    EXIT:
    close(fd);
    return HI_FAILURE;
}
/*
 *
 *
 *      device
 *
 */
HI_S32 hisi_init_device(vi_info_t *config) {
    HI_S32 ret;
    VI_DEV dev;
    VI_DEV_ATTR_S stViDevAttr;

    dev = config->device_info.device;
    memcpy_s(&stViDevAttr, sizeof(VI_DEV_ATTR_S), &DEV_ATTR_GC2063_BASE, sizeof(VI_DEV_ATTR_S));
    stViDevAttr.au32ComponentMask[0] = 0xFFC00000;
    stViDevAttr.stWDRAttr.enWDRMode = config->device_info.wdr_mode;
    if (VI_PARALLEL_VPSS_OFFLINE == config->pipe_info.mast_pipe_mode ||
        VI_PARALLEL_VPSS_PARALLEL == config->pipe_info.mast_pipe_mode) {
        stViDevAttr.enDataRate = DATA_RATE_X2;
    }

    ret = MPI_VI_SetDevAttr(dev, &stViDevAttr);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "HI_MPI_VI_SetDevAttr failed with %#x!", ret);
        return HI_FAILURE;
    }
    ret = MPI_VI_EnableDev(config->device_info.device);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "HI_MPI_VI_EnableDev failed with %#x!", ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_start_device(vi_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_device(vi_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_device(vi_info_t *config) {
    HI_S32 ret;
    ret = MPI_VI_DisableDev(config->device_info.device);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "HI_MPI_VI_DisableDev failed with %#x!", ret);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}
/*
 *
 *      isp
 *
 *
 */
HI_S32 hisi_register_sensor_callback(vi_info_t *config, ISP_DEV IspDev, HI_U32 u32SnsId) {
    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    const ISP_SNS_OBJ_S *pstSnsObj;
    HI_S32 ret = -1;

    pstSnsObj = &stSnsGc2053Obj;
    if (HI_NULL == pstSnsObj) {
        log_goke(DEBUG_WARNING, "sensor %d not exist!", u32SnsId);
        return HI_FAILURE;
    }

    stAeLib.s32Id = IspDev;
    stAwbLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
//    strncpy(stAfLib.acLibName, HI_AF_LIB_NAME, sizeof(HI_AF_LIB_NAME));

    if (pstSnsObj->pfnRegisterCallback != HI_NULL) {
        ret = pstSnsObj->pfnRegisterCallback(IspDev, &stAeLib, &stAwbLib);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "sensor_register_callback failed with %#x!", ret);
            return ret;
        }
    } else {
        log_goke(DEBUG_WARNING, "sensor_register_callback failed with HI_NULL");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_unregister_sensor_callback(ISP_DEV IspDev) {
    ALG_LIB_S stAeLib;
    ALG_LIB_S stAwbLib;
    HI_U32 u32SnsId;
    const ISP_SNS_OBJ_S *pstSnsObj;
    HI_S32 ret = -1;

    pstSnsObj = &stSnsGc2053Obj;
    if (HI_NULL == pstSnsObj) {
        log_goke(DEBUG_WARNING, "sensor %d not exist!", u32SnsId);
        return HI_FAILURE;
    }

    stAeLib.s32Id = IspDev;
    stAwbLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    //   strncpy(stAfLib.acLibName, HI_AF_LIB_NAME, sizeof(HI_AF_LIB_NAME));

    if (pstSnsObj->pfnUnRegisterCallback != HI_NULL) {
        ret = pstSnsObj->pfnUnRegisterCallback(IspDev, &stAeLib, &stAwbLib);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "sensor_unregister_callback failed with %#x!", ret);
            return ret;
        }
    } else {
        log_goke(DEBUG_WARNING, "sensor_unregister_callback failed with HI_NULL");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_register_aelib_callback(ISP_DEV IspDev) {
    ALG_LIB_S stAeLib;
    int ret;
    stAeLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    ret = MPI_AE_Register(IspDev, &stAeLib);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "aelib register call back");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_unregister_aelib_callback(ISP_DEV IspDev) {
    ALG_LIB_S stAeLib;
    int ret;
    stAeLib.s32Id = IspDev;
    strncpy(stAeLib.acLibName, HI_AE_LIB_NAME, sizeof(HI_AE_LIB_NAME));
    ret = MPI_AE_UnRegister(IspDev, &stAeLib);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "aelib unregister call back");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_register_awblib_callback(ISP_DEV IspDev) {
    ALG_LIB_S stAwbLib;
    int ret;
    stAwbLib.s32Id = IspDev;
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    ret = MPI_AWB_Register(IspDev, &stAwbLib);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "awblib register call back");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_unregister_awblib_callback(ISP_DEV IspDev) {
    ALG_LIB_S stAwbLib;
    int ret;
    stAwbLib.s32Id = IspDev;
    strncpy(stAwbLib.acLibName, HI_AWB_LIB_NAME, sizeof(HI_AWB_LIB_NAME));
    ret = MPI_AWB_UnRegister(IspDev, &stAwbLib);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "awblib unregister call back");
    }
    return HI_SUCCESS;
}

HI_S32 hisi_bind_isp_sensor(ISP_DEV IspDev, HI_U32 u32SnsId, sensor_type_t enSnsType, HI_S8 s8SnsDev) {
    ISP_SNS_COMMBUS_U uSnsBusInfo;
    ISP_SNS_TYPE_E enBusType;
    const ISP_SNS_OBJ_S *pstSnsObj;
    HI_S32 ret;

    pstSnsObj = &stSnsGc2053Obj;
    if (HI_NULL == pstSnsObj) {
        log_goke(DEBUG_WARNING, "sensor %d not exist!", u32SnsId);
        return HI_FAILURE;
    }

    enBusType = ISP_SNS_I2C_TYPE;
    uSnsBusInfo.s8I2cDev = s8SnsDev;
    if (HI_NULL != pstSnsObj->pfnSetBusInfo) {
        ret = pstSnsObj->pfnSetBusInfo(IspDev, uSnsBusInfo);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "set sensor bus info failed with %#x!", ret);
            return ret;
        }
    } else {
        log_goke(DEBUG_WARNING, "not support set sensor bus info");
        return HI_FAILURE;
    }
    return ret;
}

HI_S32 hisi_init_isp(vi_info_t *config) {
    HI_S32 i;
    HI_BOOL bNeedPipe;
    HI_S32 ret = HI_SUCCESS;
    VI_PIPE ViPipe;
    HI_U32 u32SnsId;
    ISP_PUB_ATTR_S stPubAttr;
    VI_PIPE_ATTR_S stPipeAttr;
    ISP_CTRL_PARAM_S stIspCtrlParam;

//    MPI_ISP_Exit(ViPipe);

//    ret = MPI_ISP_GetCtrlParam(config->pipe_info.pipe[0], &stIspCtrlParam);
//    if (HI_SUCCESS != ret) {
//        log_goke(DEBUG_WARNING, "HI_MPI_ISP_GetCtrlParam failed with %d!", ret);
//        return ret;
//    }
//
//    stIspCtrlParam.u32StatIntvl = config->sensor_info.framerate / 30;
//    if (stIspCtrlParam.u32StatIntvl == 0) {
//        stIspCtrlParam.u32StatIntvl = 1;
//    }
//
//    ret = MPI_ISP_SetCtrlParam(config->pipe_info.pipe[0], &stIspCtrlParam);
//    if (HI_SUCCESS != ret) {
//        log_goke(DEBUG_WARNING, "HI_MPI_ISP_SetCtrlParam failed with %d!", ret);
//        return ret;
//    }

    memcpy_s(&stPipeAttr, sizeof(VI_PIPE_ATTR_S),
             &PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
    if (VI_PIPE_BYPASS_BE == stPipeAttr.enPipeBypassMode) {
        return HI_SUCCESS;
    }

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++) {
        if (config->pipe_info.pipe[i] >= 0 && config->pipe_info.pipe[i] < VI_MAX_PIPE_NUM) {
            ViPipe = config->pipe_info.pipe[0];
            u32SnsId = config->sensor_info.sensor_id;

            memcpy(&stPubAttr, &ISP_PUB_ATTR_GC2063, sizeof(ISP_PUB_ATTR_S));
            stPubAttr.enWDRMode = config->device_info.wdr_mode;
            if (WDR_MODE_NONE == config->device_info.wdr_mode) {
                bNeedPipe = HI_TRUE;
            } else {
                bNeedPipe = (i > 0) ? HI_FALSE : HI_TRUE;
            }
            if (HI_TRUE != bNeedPipe) {
                continue;
            }

            ret = hisi_register_sensor_callback(config, ViPipe, u32SnsId);
            if (HI_SUCCESS != ret) {
                log_goke(DEBUG_WARNING, "register sensor %d to ISP %d failed", u32SnsId, ViPipe);
                hisi_uninit_isp(ViPipe);
                return HI_FAILURE;
            }

            if (((config->snap_info.double_pipe) && (config->snap_info.snap_pipe == ViPipe))
                || (config->pipe_info.multi_pipe && i > 0)) {
                ret = hisi_bind_isp_sensor(ViPipe, u32SnsId, config->sensor_info.sensor_type, -1);
                if (HI_SUCCESS != ret) {
                    log_goke(DEBUG_WARNING, "register sensor %d bus id %d failed", u32SnsId,
                             config->sensor_info.bus_id);
                    hisi_uninit_isp(ViPipe);
                    return HI_FAILURE;
                }
            } else {
                ret = hisi_bind_isp_sensor(ViPipe, u32SnsId, config->sensor_info.sensor_type,
                                           config->sensor_info.bus_id);
                if (HI_SUCCESS != ret) {
                    log_goke(DEBUG_WARNING, "register sensor %d bus id %d failed", u32SnsId,
                             config->sensor_info.bus_id);
                    hisi_uninit_isp(ViPipe);
                    return HI_FAILURE;
                }
            }
            ret = hisi_register_aelib_callback(ViPipe);
            if (HI_SUCCESS != ret) {
                log_goke(DEBUG_WARNING, "SAMPLE_COMM_ISP_Aelib_Callback failed");
                hisi_uninit_isp(ViPipe);
                return HI_FAILURE;
            }

            ret = hisi_register_awblib_callback(ViPipe);
            if (HI_SUCCESS != ret) {
                log_goke(DEBUG_WARNING, "SAMPLE_COMM_ISP_Awblib_Callback failed");
                hisi_uninit_isp(ViPipe);
                return HI_FAILURE;
            }
            ret = MPI_ISP_MemInit(ViPipe);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING, "Init Ext memory failed with %#x!", ret);
                hisi_uninit_isp(ViPipe);
                return HI_FAILURE;
            }

            ret = MPI_ISP_SetPubAttr(ViPipe, &stPubAttr);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING, "SetPubAttr failed with %#x!", ret);
                hisi_uninit_isp(ViPipe);
                return HI_FAILURE;
            }

            ret = MPI_ISP_Init(ViPipe);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING, "ISP Init failed with %#x!", ret);
                hisi_uninit_isp(ViPipe);
                return HI_FAILURE;
            }
        }
    }
    return ret;
}

HI_S32 hisi_start_isp(vi_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_isp(vi_info_t *config) {
    HI_S32 i;
    HI_BOOL bNeedPipe;
    VI_PIPE ViPipe;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++) {
        if (config->pipe_info.pipe[i] >= 0 && config->pipe_info.pipe[i] < VI_MAX_PIPE_NUM) {
            ViPipe = config->pipe_info.pipe[i];
            if (WDR_MODE_NONE == config->device_info.wdr_mode) {
                bNeedPipe = HI_TRUE;
            } else {
                bNeedPipe = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (HI_TRUE != bNeedPipe) {
                continue;
            }
            MPI_ISP_Exit(ViPipe);
        }
    }
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_isp(ISP_DEV IspDev) {
    hisi_unregister_awblib_callback(IspDev);
    hisi_unregister_aelib_callback(IspDev);
    hisi_unregister_sensor_callback(IspDev);
    return HI_SUCCESS;
}
/*
 *
 *
 *
 *  pipe
 *
 *
 *
 */
HI_S32 hisi_init_pipe(vi_info_t *config) {
    HI_S32 i, j;
    HI_S32 ret = HI_SUCCESS;
    VI_PIPE ViPipe;
    VI_PIPE_ATTR_S stPipeAttr = {0};
    HI_S32 s32PipeCnt = 0;
    VI_DEV_BIND_PIPE_S stDevBindPipe = {0};

    //bind pipe and dev first
    for (i = 0; i < WDR_MAX_PIPE_NUM; i++) {
        if (config->pipe_info.pipe[i] >= 0 && config->pipe_info.pipe[i] < VI_MAX_PIPE_NUM) {
            stDevBindPipe.PipeId[s32PipeCnt] = config->pipe_info.pipe[i];
            s32PipeCnt++;
            stDevBindPipe.u32Num = s32PipeCnt;
        }
    }
    ret = HI_MPI_VI_SetDevBindPipe(config->device_info.device, &stDevBindPipe);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "HI_MPI_VI_SetDevBindPipe failed with %#x!", ret);
        return HI_FAILURE;
    }
    //configure pipe
    for (i = 0; i < WDR_MAX_PIPE_NUM; i++) {
        if (config->pipe_info.pipe[i] >= 0 && config->pipe_info.pipe[i] < VI_MAX_PIPE_NUM) {
            ViPipe = config->pipe_info.pipe[i];
            memcpy_s(&stPipeAttr, sizeof(VI_PIPE_ATTR_S),
                     &PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR, sizeof(VI_PIPE_ATTR_S));
            if (HI_TRUE == config->pipe_info.isp_bypass) {
                stPipeAttr.bIspBypass = HI_TRUE;
                stPipeAttr.enPixFmt = config->pipe_info.pixel_format;
                stPipeAttr.enBitWidth = DATA_BITWIDTH_8;
            }
            stPipeAttr.enCompressMode = COMPRESS_MODE_NONE;
            if ((config->snap_info.snap) && (config->snap_info.double_pipe)
                && (ViPipe == config->snap_info.snap_pipe)) {
                ret = HI_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (ret != HI_SUCCESS) {
                    log_goke(DEBUG_WARNING, "HI_MPI_VI_CreatePipe failed with %#x!", ret);
                    goto EXIT;
                }
            } else {
                ret = HI_MPI_VI_CreatePipe(ViPipe, &stPipeAttr);
                if (ret != HI_SUCCESS) {
                    log_goke(DEBUG_WARNING, "HI_MPI_VI_CreatePipe failed with %#x!", ret);
                    return HI_FAILURE;
                }
                if (HI_TRUE == config->pipe_info.cnumber_configured) {
                    ret = HI_MPI_VI_SetPipeVCNumber(ViPipe, config->pipe_info.vc_number[i]);
                    if (ret != HI_SUCCESS) {
                        HI_MPI_VI_DestroyPipe(ViPipe);
                        log_goke(DEBUG_WARNING, "HI_MPI_VI_SetPipeVCNumber failed with %#x!", ret);
                        return HI_FAILURE;
                    }
                }
                ret = HI_MPI_VI_StartPipe(ViPipe);
                if (ret != HI_SUCCESS) {
                    HI_MPI_VI_DestroyPipe(ViPipe);
                    log_goke(DEBUG_WARNING, "HI_MPI_VI_StartPipe failed with %#x!\n", ret);
                    return HI_FAILURE;
                }
            }
        }
    }
    return ret;

    EXIT:
    for (j = 0; j < i; j++) {
        ViPipe = j;
        ret = HI_MPI_VI_StopPipe(ViPipe);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "HI_MPI_VI_StopPipe failed with %#x!", ret);
            return HI_FAILURE;
        }
        ret = HI_MPI_VI_DestroyPipe(ViPipe);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "HI_MPI_VI_StopPipe failed with %#x!", ret);
            return HI_FAILURE;
        }
    }

    return ret;
}

HI_S32 hisi_start_pipe(vi_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_pipe(vi_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_pipe(vi_info_t *config) {
    HI_S32 ret, i;
    VI_PIPE ViPipe;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++) {
        if (config->pipe_info.pipe[i] >= 0 && config->pipe_info.pipe[i] < VI_MAX_PIPE_NUM) {
            ViPipe = config->pipe_info.pipe[i];
            ret = HI_MPI_VI_StopPipe(ViPipe);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING, "HI_MPI_VI_StopPipe failed with %#x!\n", ret);
                return HI_FAILURE;
            }
            ret = HI_MPI_VI_DestroyPipe(ViPipe);
            if (ret != HI_SUCCESS) {
                log_goke(DEBUG_WARNING, "HI_MPI_VI_StopPipe failed with %#x!", ret);
                return HI_FAILURE;
            }
        }
    }
    return HI_SUCCESS;
}
/*
 *
 *
 * channel
 *
 *
 */
HI_S32 hisi_init_channel(vi_info_t *config) {
    HI_S32 i;
    HI_BOOL bNeedChn;
    HI_S32 ret = HI_SUCCESS;
    VI_PIPE ViPipe;
    VI_CHN ViChn;
    VI_CHN_ATTR_S stChnAttr;
    VI_VPSS_MODE_E enMastPipeMode;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++) {
        if (config->pipe_info.pipe[i] >= 0 && config->pipe_info.pipe[i] < VI_MAX_PIPE_NUM) {
            ViPipe = config->pipe_info.pipe[i];
            ViChn = config->channel_info.channel;

            memcpy_s(&stChnAttr, sizeof(VI_CHN_ATTR_S), &CHN_ATTR_1920x1080_420_SDR8_LINEAR, sizeof(VI_CHN_ATTR_S));
            stChnAttr.enDynamicRange = config->channel_info.dynamic_range;
            stChnAttr.enVideoFormat = config->channel_info.video_format;
            stChnAttr.enPixelFormat = config->channel_info.pixel_format;
            stChnAttr.enCompressMode = config->channel_info.compress_mode;
            if (WDR_MODE_NONE == config->device_info.wdr_mode) {
                bNeedChn = HI_TRUE;
            } else {
                bNeedChn = (i > 0) ? HI_FALSE : HI_TRUE;
            }
            if (bNeedChn) {
                ret = HI_MPI_VI_SetChnAttr(ViPipe, ViChn, &stChnAttr);
                if (ret != HI_SUCCESS) {
                    log_goke(DEBUG_WARNING, "HI_MPI_VI_SetChnAttr failed with %#x!", ret);
                    return HI_FAILURE;
                }
            }
            enMastPipeMode = config->pipe_info.mast_pipe_mode;
            if (VI_OFFLINE_VPSS_OFFLINE == enMastPipeMode
                || VI_ONLINE_VPSS_OFFLINE == enMastPipeMode
                || VI_PARALLEL_VPSS_OFFLINE == enMastPipeMode) {
                ret = HI_MPI_VI_EnableChn(ViPipe, ViChn);
                if (ret != HI_SUCCESS) {
                    log_goke(DEBUG_WARNING, "HI_MPI_VI_EnableChn failed with %#x!", ret);
                    return HI_FAILURE;
                }
            }
        }
    }
    return ret;
}

HI_S32 hisi_start_channel(vi_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_channel(vi_info_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_channel(vi_info_t *config) {
    HI_S32 i;
    HI_BOOL bNeedChn;
    HI_S32 ret = HI_SUCCESS;
    VI_PIPE ViPipe;
    VI_CHN ViChn;
    VI_VPSS_MODE_E enMastPipeMode;

    for (i = 0; i < WDR_MAX_PIPE_NUM; i++) {
        if (config->pipe_info.pipe[i] >= 0 && config->pipe_info.pipe[i] < VI_MAX_PIPE_NUM) {
            ViPipe = config->pipe_info.pipe[i];
            ViChn = config->channel_info.channel;

            if (WDR_MODE_NONE == config->device_info.wdr_mode) {
                bNeedChn = HI_TRUE;
            } else {
                bNeedChn = (i > 0) ? HI_FALSE : HI_TRUE;
            }

            if (bNeedChn) {
                enMastPipeMode = config->pipe_info.mast_pipe_mode;
                if (VI_OFFLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_ONLINE_VPSS_OFFLINE == enMastPipeMode
                    || VI_PARALLEL_VPSS_OFFLINE == enMastPipeMode) {
                    ret = HI_MPI_VI_DisableChn(ViPipe, ViChn);
                    if (ret != HI_SUCCESS) {
                        log_goke(DEBUG_WARNING, "HI_MPI_VI_DisableChn failed with %#x!", ret);
                        return HI_FAILURE;
                    }
                }
            }
        }
    }
    return ret ;
}

/*
 *
 *
 *
 *
 *
 */
HI_S32 hisi_init_vi(vi_info_t *config, int *info) {
    HI_S32 ret = HI_SUCCESS;

    //mipi
    ret = hisi_init_mipi(&config->sensor_info);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "init mipi failed");
        return HI_FAILURE;
    }
    misc_set_bit(info, VI_INIT_START_MIPI);
    //dev
    ret = hisi_set_vi_vpss_mode(config);
    ret |= hisi_init_device(config);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "init device failed");
        return HI_FAILURE;
    }
    misc_set_bit(info, VI_INIT_START_DEVICE);
    //pipe
    ret = hisi_init_pipe(config);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "init pipe failed");
        return HI_FAILURE;
    }
    misc_set_bit(info, VI_INIT_START_PIPE);
    //channel
    ret = hisi_init_channel(config);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "init channel failed");
        return HI_FAILURE;
    }
    misc_set_bit(info, VI_INIT_START_CHANNEL);
    //isp
    ret = hisi_init_isp(config);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "init isp failed");
        return HI_FAILURE;
    }
    misc_set_bit(info, VI_INIT_START_ISP);
    return ret;
}

HI_S32 hisi_start_vi(vi_info_t *config, int *info) {
    HI_S32 ret = HI_SUCCESS;

    //mipi
    if (!misc_get_bit(*info, VI_INIT_START_MIPI)) {
        ret = hisi_start_mipi(config);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "start mipi failed");
            return HI_FAILURE;
        }
        misc_set_bit(info, VI_INIT_START_MIPI);
    }
    //dev
    if (!misc_get_bit(*info, VI_INIT_START_DEVICE)) {
        ret = hisi_start_device(config);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "start device failed");
            return HI_FAILURE;
        }
        misc_set_bit(info, VI_INIT_START_DEVICE);
    }
    //isp
    //pipe
    if (!misc_get_bit(*info, VI_INIT_START_PIPE)) {
        ret = hisi_start_pipe(config);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "start pipe failed");
            return HI_FAILURE;
        }
        misc_set_bit(info, VI_INIT_START_PIPE);
    }
    //channel
    if (!misc_get_bit(*info, VI_INIT_START_CHANNEL)) {
        ret = hisi_start_channel(config);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "start channel failed");
            return HI_FAILURE;
        }
        misc_set_bit(info, VI_INIT_START_CHANNEL);
    }
    return ret;
}

HI_S32 hisi_stop_vi(vi_info_t *config, int *info) {
    int ret = 0;
    if (misc_get_bit(info, VI_INIT_START_ISP)) {
        ret |= hisi_stop_isp(config);
        misc_clear_bit(info, VI_INIT_START_ISP);
    }
    if (misc_get_bit(info, VI_INIT_START_CHANNEL)) {
        ret |= hisi_stop_channel(config);
        misc_clear_bit(info, VI_INIT_START_CHANNEL);
    }
    if (misc_get_bit(info, VI_INIT_START_PIPE)) {
        ret |= hisi_stop_pipe(config);
        misc_clear_bit(info, VI_INIT_START_PIPE);
    }
    if (misc_get_bit(info, VI_INIT_START_DEVICE)) {
        ret |= hisi_stop_device(config);
        misc_clear_bit(info, VI_INIT_START_DEVICE);
    }
    if (misc_get_bit(info, VI_INIT_START_MIPI)) {
        ret |= hisi_stop_mipi(config);
        misc_clear_bit(info, VI_INIT_START_MIPI);
    }
    return ret;
}

HI_S32 hisi_uninit_vi(vi_info_t *config, int *info) {
    int ret = 0;
    if (misc_get_bit(info, VI_INIT_START_ISP)) {
        ret |= hisi_uninit_isp(config->device_info.device);
        misc_clear_bit(info, VI_INIT_START_ISP);
    }
    if (misc_get_bit(info, VI_INIT_START_CHANNEL)) {
        ret |= hisi_uninit_channel(config);
        misc_clear_bit(info, VI_INIT_START_CHANNEL);
    }
    if (misc_get_bit(info, VI_INIT_START_PIPE)) {
        ret |= hisi_uninit_pipe(config);
        misc_clear_bit(info, VI_INIT_START_PIPE);
    }
    if (misc_get_bit(info, VI_INIT_START_DEVICE)) {
        ret |= hisi_uninit_device(config);
        misc_clear_bit(info, VI_INIT_START_DEVICE);
    }
    if (misc_get_bit(info, VI_INIT_START_MIPI)) {
        ret |= hisi_uninit_mipi(config);
        misc_clear_bit(info, VI_INIT_START_MIPI);
    }
    return ret;
}