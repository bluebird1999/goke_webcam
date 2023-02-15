/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#include <stdio.h>
#include <string.h>

#include "ot_mpi_isp.h"
#include "isp_vreg.h"
#include "sample_ae_adp.h"
#include "sample_ae_ext_config.h"
#include "sample_ae_mpi.h"


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
SAMPLE_AE_CTX_S g_astAeCtx_sample[MAX_AE_LIB_NUM] = {{0}};


/****************************************************************************
 * EXTERN FUNCTION DECLARATION                                              *
 ****************************************************************************/


/****************************************************************************
 * INTERNAL FUNCTION DECLARATION                                            *
 ****************************************************************************/


/****************************************************************************
 * AE FUNCTION                                                             *
 *            ---- Assistant Function Area                                  *
 ****************************************************************************/

GK_S32 SAMPLE_AE_ExtRegsInitialize(GK_S32 s32Handle)
{
    GK_U8 u8Id = AE_GET_EXTREG_ID(s32Handle);

    /* set ext registers as a default value */
    ext_system_ae_mode_write(u8Id, ISP_EXT_SYSTEM_AE_MODE_DEFAULT);

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AE_ReadExtRegs(GK_S32 s32Handle)
{
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    GK_U8 u8Id = AE_GET_EXTREG_ID(s32Handle);

    pstAeCtx = AE_GET_CTX(s32Handle);

    /* read the extregs to the global variables */
    pstAeCtx->u8AeMode = ext_system_ae_mode_read(u8Id);

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AE_UpdateExtRegs(GK_S32 s32Handle)
{
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    GK_U8 u8Id = AE_GET_EXTREG_ID(s32Handle);

    pstAeCtx = AE_GET_CTX(s32Handle);

    /* update the global variables to the extregs */
    GK_U32 u32Iso;

    u32Iso = pstAeCtx->stAeResult.u32Iso;

    ext_system_query_exposure_again_write(u8Id, u32Iso);


    return GK_SUCCESS;
}

GK_S32 SAMPLE_AE_Calculate(GK_S32 s32Handle)
{
    /* user's AE alg implementor */
    return GK_SUCCESS;
}


GK_S32 SAMPLE_AE_IspRegsInit(GK_S32 s32Handle)
{
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    ISP_AE_STAT_ATTR_S *pstStatAttr = GK_NULL;

    GK_U8 au8WeightTable[15][17] = {
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1},
        {1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1},
        {1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
    };

    pstAeCtx = AE_GET_CTX(s32Handle);
    pstStatAttr = &pstAeCtx->stAeResult.stStatAttr;

    /* change the ae zone's weight table and histogram thresh, although there is
     * defalut value in isp.
     */
    memcpy(pstStatAttr->au8WeightTable, au8WeightTable, sizeof(au8WeightTable));

    pstStatAttr->bChange = GK_TRUE;

    return GK_SUCCESS;
}


GK_S32 SAMPLE_AE_Init(GK_S32 s32Handle, const ISP_AE_PARAM_S *pstAeParam)
{
    GK_S32 s32Ret;
    VI_PIPE ViPipe;
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    GK_U8 u8Id;

    AE_CHECK_HANDLE_ID(s32Handle);
    pstAeCtx = AE_GET_CTX(s32Handle);
    u8Id = AE_GET_EXTREG_ID(s32Handle);
    ViPipe = pstAeCtx->IspBindDev;

    AE_CHECK_POINTER(pstAeParam);

    /* do something ... like check the sensor id, init the global variables, and so on */
    /* Commonly, create a virtual regs to communicate, also user can design the new commuincation style. */
    s32Ret = VReg_Init(ViPipe, AE_LIB_VREG_BASE(u8Id), AE_VREG_SIZE);
    if (s32Ret != GK_SUCCESS) {
        AE_TRACE(MODULE_DBG_ERR, "Ae lib(%d) vreg init failed!\n", s32Handle);
        return s32Ret;
    }

    /* maybe you need to init the virtual regs. */
    SAMPLE_AE_ExtRegsInitialize(s32Handle);

    return GK_SUCCESS;
}



GK_S32 SAMPLE_AE_Run(GK_S32 s32Handle, const ISP_AE_INFO_S *pstAeInfo,
                     ISP_AE_RESULT_S *pstAeResult, GK_S32 s32Rsv)
{
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    VI_PIPE ViPipe;

    AE_CHECK_HANDLE_ID(s32Handle);
    pstAeCtx = AE_GET_CTX(s32Handle);
    ViPipe = pstAeCtx->IspBindDev;

    AE_CHECK_POINTER(pstAeInfo);
    AE_CHECK_POINTER(pstAeResult);

    AE_CHECK_POINTER(pstAeInfo->pstFEAeStat1);
    AE_CHECK_POINTER(pstAeInfo->pstFEAeStat2);
    AE_CHECK_POINTER(pstAeInfo->pstFEAeStat3);

    pstAeCtx->u32FrameCnt = pstAeInfo->u32FrameCnt;

    /* init isp regs in the first frame. the regs in stStatAttr may need to be...
     * ...configed a few times.
     */

    if (pstAeCtx->u32FrameCnt == 1) {
        SAMPLE_AE_IspRegsInit(s32Handle);
    }

    /* do AE alg per 2 frame */

    if (pstAeCtx->u32FrameCnt % 2 == 0) {
        /* record the statistics in pstAeInfo, and then use the statistics...
         * ...to calculate, no need to call any other api
         */
        memcpy(&pstAeCtx->stAeInfo, pstAeInfo, sizeof(ISP_AE_INFO_S));

        /* maybe need to read the virtual regs to check whether...
         * ...someone changes the configs.
         */
        SAMPLE_AE_ReadExtRegs(s32Handle);

        SAMPLE_AE_Calculate(s32Handle);

        /* pls fill the result after calculate, the firmware will config...
         * ...the regs for awb algorithm.
         */

        pstAeCtx->stAeResult.u32Iso = 0x100;
        pstAeCtx->stAeResult.u32IspDgain = 0x100;

        /* maybe need to write some configs to the virtual regs. */
        SAMPLE_AE_UpdateExtRegs(s32Handle);

        /* maybe need to call the sensor's register functions to config sensor */
        // update sensor gains
        if (pstAeCtx->stSnsRegister.stSnsExp.pfn_cmos_gains_update != GK_NULL) {
            GK_U32 u32Again = 0;
            GK_U32 u32Dgain = 0;

            pstAeCtx->stSnsRegister.stSnsExp.pfn_cmos_gains_update(ViPipe, u32Again, u32Dgain);
        }

        // update sensor exposure time etc.

    }

    /* record result */
    memcpy(pstAeResult, &pstAeCtx->stAeResult, sizeof(ISP_AE_RESULT_S));

    pstAeCtx->stAeResult.stStatAttr.bChange = GK_FALSE;

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AE_Ctrl(GK_S32 s32Handle, GK_U32 u32Cmd, GK_VOID *pValue)
{
    AE_CHECK_HANDLE_ID(s32Handle);

    AE_CHECK_POINTER(pValue);

    switch (u32Cmd) {
            /* system ctrl */
        case ISP_WDR_MODE_SET :
            /* do something ... */
            break;
        case ISP_AE_FPS_BASE_SET :
            /* do something ... */
            break;
            /* ae ctrl, define the customer's ctrl cmd, if needed ... */
        default :
            break;
    }

    return GK_SUCCESS;
}

GK_S32 SAMPLE_AE_Exit(GK_S32 s32Handle)
{
    GK_S32 s32Ret;
    VI_PIPE ViPipe;
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    GK_U8 u8Id;

    AE_CHECK_HANDLE_ID(s32Handle);
    pstAeCtx = AE_GET_CTX(s32Handle);
    u8Id = AE_GET_EXTREG_ID(s32Handle);
    ViPipe = pstAeCtx->IspBindDev;

    /* do something ... */
    /* if created the virtual regs, need to destory virtual regs. */
    s32Ret = VReg_Exit(ViPipe, AE_LIB_VREG_BASE(u8Id), AE_VREG_SIZE);
    if (s32Ret != GK_SUCCESS) {
        AE_TRACE(MODULE_DBG_ERR, "Ae lib(%d) vreg exit failed!\n", s32Handle);
        return s32Ret;
    }

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_AE_Register(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
    ISP_AE_REGISTER_S stRegister;
    GK_S32 s32Ret = GK_SUCCESS;
    GK_S32 s32Handle;
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;

    AE_CHECK_DEV(ViPipe);
    AE_CHECK_POINTER(pstAeLib);
    AE_CHECK_HANDLE_ID(pstAeLib->s32Id);
    AE_CHECK_LIB_NAME(pstAeLib->acLibName);

    s32Handle = pstAeLib->s32Id;
    pstAeCtx = AE_GET_CTX(s32Handle);
    pstAeCtx->IspBindDev = ViPipe;

    stRegister.stAeExpFunc.pfn_ae_init  = SAMPLE_AE_Init;
    stRegister.stAeExpFunc.pfn_ae_run   = SAMPLE_AE_Run;
    stRegister.stAeExpFunc.pfn_ae_ctrl  = SAMPLE_AE_Ctrl;
    stRegister.stAeExpFunc.pfn_ae_exit  = SAMPLE_AE_Exit;
    s32Ret = MPI_ISP_AELibRegCallBack(ViPipe, pstAeLib, &stRegister);
    if (s32Ret != GK_SUCCESS) {
        AE_TRACE(MODULE_DBG_ERR, "ae register failed!\n");
    }

    return s32Ret;
}

GK_S32 SAMPLE_MPI_AE_UnRegister(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib)
{
    GK_S32 s32Ret = GK_SUCCESS;

    AE_CHECK_DEV(ViPipe);
    AE_CHECK_POINTER(pstAeLib);
    AE_CHECK_HANDLE_ID(pstAeLib->s32Id);
    AE_CHECK_LIB_NAME(pstAeLib->acLibName);

    s32Ret = MPI_ISP_AELibUnRegCallBack(ViPipe, pstAeLib);
    if (s32Ret != GK_SUCCESS) {
        AE_TRACE(MODULE_DBG_ERR, "ae unregister failed!\n");
    }

    return s32Ret;
}

GK_S32 SAMPLE_MPI_AE_SensorRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, ISP_SNS_ATTR_INFO_S *pstSnsAttrInfo,
                                          AE_SENSOR_REGISTER_S *pstRegister)
{
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    GK_S32  s32Handle;

    AE_CHECK_DEV(ViPipe);
    AE_CHECK_POINTER(pstAeLib);
    AE_CHECK_POINTER(pstRegister);

    s32Handle = pstAeLib->s32Id;
    AE_CHECK_HANDLE_ID(s32Handle);
    AE_CHECK_LIB_NAME(pstAeLib->acLibName);

    pstAeCtx = AE_GET_CTX(s32Handle);

    /* get default value */
    if (pstRegister->stSnsExp.pfn_cmos_get_ae_default != GK_NULL) {
        pstRegister->stSnsExp.pfn_cmos_get_ae_default(ViPipe, &pstAeCtx->stSnsDft);
    }

    /* record information */
    memcpy(&pstAeCtx->stSnsRegister, pstRegister, sizeof(AE_SENSOR_REGISTER_S));
    memcpy(&pstAeCtx->stSnsAttrInfo, pstSnsAttrInfo, sizeof(ISP_SNS_ATTR_INFO_S));

    pstAeCtx->bSnsRegister = GK_TRUE;

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_AE_SensorUnRegCallBack(VI_PIPE ViPipe, ALG_LIB_S *pstAeLib, SENSOR_ID SensorId)
{
    SAMPLE_AE_CTX_S *pstAeCtx = GK_NULL;
    GK_S32  s32Handle;

    AE_CHECK_DEV(ViPipe);
    AE_CHECK_POINTER(pstAeLib);

    s32Handle = pstAeLib->s32Id;
    AE_CHECK_HANDLE_ID(s32Handle);
    AE_CHECK_LIB_NAME(pstAeLib->acLibName);

    pstAeCtx = AE_GET_CTX(s32Handle);

    memset(&pstAeCtx->stSnsDft, 0, sizeof(AE_SENSOR_DEFAULT_S));
    memset(&pstAeCtx->stSnsRegister, 0, sizeof(AE_SENSOR_REGISTER_S));
    pstAeCtx->stSnsAttrInfo.eSensorId = 0;
    pstAeCtx->bSnsRegister = GK_FALSE;

    return GK_SUCCESS;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
