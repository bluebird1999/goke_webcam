/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#include <string.h>
#include <stdio.h>

#include "comm_isp.h"
#include "comm_3a.h"
#include "sample_ae_ext_config.h"
#include "isp_config.h"
#include "sample_ae_adp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


GK_S32 SAMPLE_MPI_ISP_SetExposureAttr(VI_PIPE ViPipe, const ISP_EXPOSURE_ATTR_S *pstExpAttr)
{
    ALG_LIB_S stAeLib;
    stAeLib.s32Id = 4;
    strncpy(stAeLib.acLibName, "ae_lib", sizeof("ae_lib"));

    if (pstExpAttr->stAuto.enAEMode >= AE_MODE_BUTT) {
        AE_TRACE(MODULE_DBG_ERR, "Invalid AE mode!\n");
        return ERR_CODE_ISP_ILLEGAL_PARAM;
    }

    ext_system_ae_mode_write((GK_U8)stAeLib.s32Id, (GK_U8)pstExpAttr->stAuto.enAEMode);


    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_GetExposureAttr(VI_PIPE ViPipe, ISP_EXPOSURE_ATTR_S *pstExpAttr)
{

    return GK_SUCCESS;
}


GK_S32 SAMPLE_MPI_ISP_SetWDRExposureAttr(VI_PIPE ViPipe, const ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_GetWDRExposureAttr(VI_PIPE ViPipe, ISP_WDR_EXPOSURE_ATTR_S *pstWDRExpAttr)
{

    return GK_SUCCESS;
}


GK_S32 SAMPLE_ISP_RouteCheck(GK_U8 u8Id, const ISP_AE_ROUTE_S *pstRoute)
{

    return GK_SUCCESS;
}


GK_S32 SAMPLE_MPI_ISP_SetAERouteAttr(VI_PIPE ViPipe, const ISP_AE_ROUTE_S *pstAERouteAttr)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_GetAERouteAttr(VI_PIPE ViPipe, ISP_AE_ROUTE_S *pstAERouteAttr)
{

    return GK_SUCCESS;
}


GK_S32 SAMPLE_MPI_ISP_QueryExposureInfo(VI_PIPE ViPipe, ISP_EXP_INFO_S *pstExpInfo)
{

    ALG_LIB_S stAeLib;
    stAeLib.s32Id = 4;
    strncpy(stAeLib.acLibName, "ae_lib", sizeof("ae_lib"));

    pstExpInfo->u32AGain = ext_system_query_exposure_again_read((GK_U8)stAeLib.s32Id);

    return GK_SUCCESS;

}


GK_S32 SAMPLE_MPI_ISP_SetIrisAttr(VI_PIPE ViPipe, const ISP_IRIS_ATTR_S *pstIrisAttr)
{


    return GK_SUCCESS;
}


GK_S32 SAMPLE_MPI_ISP_GetIrisAttr(VI_PIPE ViPipe, ISP_IRIS_ATTR_S *pstIrisAttr)
{

    return GK_SUCCESS;

}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
