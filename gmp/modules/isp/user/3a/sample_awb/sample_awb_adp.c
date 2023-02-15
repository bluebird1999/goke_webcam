/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#include <stdio.h>
#include <string.h>

#include "ot_mpi_isp.h"
#include "isp_vreg.h"
#include "sample_awb_adp.h"
#include "sample_awb_ext_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

/****************************************************************************
 * MACRO DEFINITION                                                         *
 ****************************************************************************/


/****************************************************************************
 * EXTERN VARIABLES                                                         *
 ****************************************************************************/


/****************************************************************************
 * GLOBAL VARIABLES                                                         *
 ****************************************************************************/

SAMPLE_AWB_CTX_S g_astAwbCtx[MAX_AWB_LIB_NUM] = {{0}};

GK_S32 SAMPLE_AWB_ExtRegsInitialize(GK_S32 s32Handle)
{
    GK_U8 u8Id;
    u8Id = AWB_GET_EXTREG_ID(s32Handle);

    ext_system_wb_type_write(u8Id, ISP_EXT_SYSTEM_WB_TYPE_DEFAULT);

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_ReadExtRegs(GK_S32 s32Handle)
{
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;
    GK_U8 u8Id;
    GK_U8 u8WbType;

    pstAwbCtx = AWB_GET_CTX(s32Handle);
    u8Id = AWB_GET_EXTREG_ID(s32Handle);

    /* read the extregs to the global variables */

    u8WbType = ext_system_wb_type_read(u8Id);

    pstAwbCtx->u8WbType = u8WbType;

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_UpdateExtRegs(GK_S32 s32Handle)
{
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;
    GK_U8 u8Id;

    pstAwbCtx = AWB_GET_CTX(s32Handle);
    u8Id = AWB_GET_EXTREG_ID(s32Handle);

    /* update the global variables to the extregs */

    ext_system_wb_detect_temp_write(u8Id, pstAwbCtx->u16DetectTemp);

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_IspRegsInit(GK_S32 s32Handle)
{
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;
    GK_S32 i;

    ISP_AWB_RAW_STAT_ATTR_S *pstRawStatAttr = GK_NULL;

    pstAwbCtx = AWB_GET_CTX(s32Handle);



    pstRawStatAttr = &pstAwbCtx->stAwbResult.stRawStatAttr;
    pstRawStatAttr->u16MeteringWhiteLevelAwb = 0xFFF;
    pstRawStatAttr->u16MeteringBlackLevelAwb = 0x08;
    pstRawStatAttr->u16MeteringCrRefMaxAwb = 0x130;
    pstRawStatAttr->u16MeteringCrRefMinAwb = 0x40;
    pstRawStatAttr->u16MeteringCbRefMaxAwb = 0x120;
    pstRawStatAttr->u16MeteringCbRefMinAwb = 0x40;

    pstRawStatAttr->bStatCfgUpdate  = GK_TRUE;


    for (i = 0; i < 4; i++) {
        pstAwbCtx->stAwbResult.au32WhiteBalanceGain[i] =
            pstAwbCtx->stSnsDft.au16GainOffset[i] << 8;
    }

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_Calculate(GK_S32 s32Handle)
{
    /* user's AWB alg implementor */
    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_Init(GK_S32 s32Handle, const ISP_AWB_PARAM_S *pstAwbParam, ISP_AWB_RESULT_S *pstAwbResult)
{
    GK_S32 s32Ret;
    VI_PIPE ViPipe;
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;
    GK_U8 u8Id;

    AWB_CHECK_HANDLE_ID(s32Handle);
    pstAwbCtx = AWB_GET_CTX(s32Handle);
    u8Id = AWB_GET_EXTREG_ID(s32Handle);
    ViPipe = pstAwbCtx->IspBindDev;

    AWB_CHECK_POINTER(pstAwbParam);
    AWB_CHECK_POINTER(pstAwbResult);

    /* do something ... like check the sensor id, init the global variables...
     * and so on
     */
    /* Commonly, create a virtual regs to communicate, also user can design...
     * the new commuincation style.
     */

    s32Ret = VReg_Init(ViPipe, AWB_LIB_VREG_BASE(u8Id), ALG_LIB_VREG_SIZE);
    if (s32Ret != GK_SUCCESS) {
        AWB_TRACE(MODULE_DBG_ERR, "Awb lib(%d) vreg init failed!\n", s32Handle);
        return s32Ret;
    }

    SAMPLE_AWB_ExtRegsInitialize(s32Handle);

    /* maybe you need to init the virtual regs. */

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_Run(GK_S32 s32Handle, const ISP_AWB_INFO_S *pstAwbInfo,
                      ISP_AWB_RESULT_S *pstAwbResult, GK_S32 s32Rsv)
{
    GK_S32  i;
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;

    AWB_CHECK_HANDLE_ID(s32Handle);
    pstAwbCtx = AWB_GET_CTX(s32Handle);

    AWB_CHECK_POINTER(pstAwbInfo);
    AWB_CHECK_POINTER(pstAwbResult);

    AWB_CHECK_POINTER(pstAwbInfo->pstAwbStat1);

    pstAwbCtx->u32FrameCnt = pstAwbInfo->u32FrameCnt;

    /* init isp regs in the first frame. the regs in stStatAttr may need to be configed a few times. */
    if (pstAwbCtx->u32FrameCnt == 1) {
        SAMPLE_AWB_IspRegsInit(s32Handle);
    }

    /* do something ... */
    if (pstAwbCtx->u32FrameCnt % 2 == 0) {
        /* record the statistics in pstAwbInfo, and then use the statistics to calculate, no need to call any other api */
        memcpy_s(&pstAwbCtx->stAwbInfo, sizeof(ISP_AWB_INFO_S), pstAwbInfo, sizeof(ISP_AWB_INFO_S));

        /* maybe need to read the virtual regs to check whether someone changes the configs. */
        SAMPLE_AWB_ReadExtRegs(s32Handle);

        SAMPLE_AWB_Calculate(s32Handle);

        /* maybe need to write some configs to the virtual regs. */
        SAMPLE_AWB_UpdateExtRegs(s32Handle);

        /* pls fill the result after calculate, the firmware will config the regs for awb algorithm. */
        for (i = 0; i < 9; i++) {
            pstAwbCtx->stAwbResult.au16ColorMatrix[i] = 0x0;
        }

        /* the unit of the result is not fixed, just map with the isp_awb.c, modify the unit together. */
        pstAwbCtx->stAwbResult.au16ColorMatrix[0] = 0x100;
        pstAwbCtx->stAwbResult.au16ColorMatrix[4] = 0x100;
        pstAwbCtx->stAwbResult.au16ColorMatrix[8] = 0x100;

        pstAwbCtx->stAwbResult.au32WhiteBalanceGain[0] = 0x100 << 8;
        pstAwbCtx->stAwbResult.au32WhiteBalanceGain[1] = 0x100 << 8;
        pstAwbCtx->stAwbResult.au32WhiteBalanceGain[2] = 0x100 << 8;
        pstAwbCtx->stAwbResult.au32WhiteBalanceGain[3] = 0x100 << 8;
    }

    /* record result */
    memcpy_s(pstAwbResult, sizeof(ISP_AWB_RESULT_S), &pstAwbCtx->stAwbResult, sizeof(ISP_AWB_RESULT_S));



    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_Ctrl(GK_S32 s32Handle, GK_U32 u32Cmd, GK_VOID *pValue)
{
    AWB_CHECK_HANDLE_ID(s32Handle);

    AWB_CHECK_POINTER(pValue);

    switch (u32Cmd) {
            /* system ctrl */
        case ISP_WDR_MODE_SET :
            /* do something ... */
            break;
        case ISP_AWB_ISO_SET :
            /* do something ... */
            break;
            /* awb ctrl, define the customer's ctrl cmd, if needed ... */
        default :
            break;
    }

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AWB_Exit(GK_S32 s32Handle)
{
    GK_S32 s32Ret;
    GK_U8 u8Id;
    VI_PIPE ViPipe;
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;

    AWB_CHECK_HANDLE_ID(s32Handle);
    pstAwbCtx = AWB_GET_CTX(s32Handle);
    u8Id = AWB_GET_EXTREG_ID(s32Handle);
    ViPipe = pstAwbCtx->IspBindDev;

    /* do something ... */
    /* if created the virtual regs, need to destory virtual regs. */
    s32Ret = VReg_Exit(ViPipe, AWB_LIB_VREG_BASE(u8Id), ALG_LIB_VREG_SIZE);
    if (s32Ret != GK_SUCCESS) {
        AWB_TRACE(MODULE_DBG_ERR, "Awb lib(%d) vreg exit failed!\n", s32Handle);
        return s32Ret;
    }

    return GK_SUCCESS;
}

GK_S32 MPI_AWB_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
    ISP_AWB_REGISTER_S stRegister;
    GK_S32 s32Ret = GK_SUCCESS;

    AWB_CHECK_DEV(ViPipe);
    AWB_CHECK_POINTER(pstAwbLib);
    AWB_CHECK_HANDLE_ID(pstAwbLib->s32Id);
    AWB_CHECK_LIB_NAME(pstAwbLib->acLibName);

    stRegister.stAwbExpFunc.pfn_awb_init  = SAMPLE_AWB_Init;
    stRegister.stAwbExpFunc.pfn_awb_run   = SAMPLE_AWB_Run;
    stRegister.stAwbExpFunc.pfn_awb_ctrl  = SAMPLE_AWB_Ctrl;
    stRegister.stAwbExpFunc.pfn_awb_exit  = SAMPLE_AWB_Exit;
    s32Ret = MPI_ISP_AWBLibRegCallBack(ViPipe, pstAwbLib, &stRegister);
    if (s32Ret != GK_SUCCESS) {
        AWB_TRACE(MODULE_DBG_ERR, "awb register failed!\n");
    }

    return s32Ret;
}

GK_S32 MPI_AWB_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib)
{
    GK_S32 s32Ret = GK_SUCCESS;

    AWB_CHECK_DEV(ViPipe);
    AWB_CHECK_POINTER(pstAwbLib);
    AWB_CHECK_HANDLE_ID(pstAwbLib->s32Id);
    AWB_CHECK_LIB_NAME(pstAwbLib->acLibName);

    s32Ret = MPI_ISP_AWBLibUnRegCallBack(ViPipe, pstAwbLib);
    if (s32Ret != GK_SUCCESS) {
        AWB_TRACE(MODULE_DBG_ERR, "awb unregister failed!\n");
    }

    return s32Ret;
}


GK_S32 MPI_AWB_SensorRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
                                    AWB_SENSOR_REGISTER_S *pstRegister)
{
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;
    GK_S32  s32Handle;

    AWB_CHECK_DEV(ViPipe);
    AWB_CHECK_POINTER(pstAwbLib);
    AWB_CHECK_POINTER(pstRegister);

    s32Handle = pstAwbLib->s32Id;
    AWB_CHECK_HANDLE_ID(s32Handle);
    AWB_CHECK_LIB_NAME(pstAwbLib->acLibName);

    pstAwbCtx = AWB_GET_CTX(s32Handle);

    if (pstRegister->stSnsExp.pfn_cmos_get_awb_default != GK_NULL) {
        pstRegister->stSnsExp.pfn_cmos_get_awb_default(ViPipe, &pstAwbCtx->stSnsDft);
    }

    memcpy_s(&pstAwbCtx->stSnsRegister, sizeof(AWB_SENSOR_REGISTER_S), pstRegister, sizeof(AWB_SENSOR_REGISTER_S));
    memcpy_s(&pstAwbCtx->stSnsAttrInfo, sizeof(ISP_SNS_ATTR_INFO_S), pstSnsAttrInfo, sizeof(ISP_SNS_ATTR_INFO_S));

    pstAwbCtx->bSnsRegister = GK_TRUE;

    return GK_SUCCESS;
}

GK_S32 MPI_AWB_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAwbLib, SENSOR_ID SensorId)
{
    SAMPLE_AWB_CTX_S *pstAwbCtx = GK_NULL;
    GK_S32  s32Handle;

    AWB_CHECK_DEV(ViPipe);
    AWB_CHECK_POINTER(pstAwbLib);

    s32Handle = pstAwbLib->s32Id;
    AWB_CHECK_HANDLE_ID(s32Handle);
    AWB_CHECK_LIB_NAME(pstAwbLib->acLibName);

    pstAwbCtx = AWB_GET_CTX(s32Handle);

    memset_s(&pstAwbCtx->stSnsDft, sizeof(AWB_SENSOR_DEFAULT_S), 0, sizeof(AWB_SENSOR_DEFAULT_S));
    memset_s(&pstAwbCtx->stSnsRegister, sizeof(AWB_SENSOR_REGISTER_S), 0, sizeof(AWB_SENSOR_REGISTER_S));
    pstAwbCtx->stSnsAttrInfo.eSensorId = 0;
    pstAwbCtx->bSnsRegister = GK_FALSE;

    return GK_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
