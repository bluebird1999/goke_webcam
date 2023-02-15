/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#include <string.h>
#include <stdio.h>

#include "comm_isp.h"
#include "comm_3a.h"
#include "sample_awb_ext_config.h"
#include "isp_config.h"
#include "sample_awb_adp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */


GK_S32 SAMPLE_MPI_ISP_SetWBAttr(VI_PIPE ViPipe, const ISP_WB_ATTR_S *pstWBAttr)
{
    ALG_LIB_S stAwbLib;

    AWB_CHECK_DEV(ViPipe);
    AWB_CHECK_POINTER(pstWBAttr);

    stAwbLib.s32Id = 0;
    strncpy(stAwbLib.acLibName, "awb_lib", sizeof("awb_lib"));

    if (OP_TYPE_BUTT <= pstWBAttr->enOpType) {
        printf("Invalid input of parameter enOpType!\n");
        return ERR_CODE_ISP_ILLEGAL_PARAM;
    }

    ext_system_wb_type_write((GK_U8)stAwbLib.s32Id, pstWBAttr->enOpType);

    return GK_SUCCESS;
}


GK_S32 SAMPLE_MPI_ISP_GetWBAttr(VI_PIPE ViPipe, ISP_WB_ATTR_S *pstWBAttr)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_SetAWBAttrEx(VI_PIPE ViPipe, ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_GetAWBAttrEx(VI_PIPE ViPipe, ISP_AWB_ATTR_EX_S *pstAWBAttrEx)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_SetCCMAttr(VI_PIPE ViPipe, const ISP_COLORMATRIX_ATTR_S *pstCCMAttr)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_GetCCMAttr(VI_PIPE ViPipe, ISP_COLORMATRIX_ATTR_S *pstCCMAttr)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_SetSaturationAttr(VI_PIPE ViPipe, const ISP_SATURATION_ATTR_S *pstSatAttr)
{


    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_GetSaturationAttr(VI_PIPE ViPipe, ISP_SATURATION_ATTR_S *pstSatAttr)
{


    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_SetColorToneAttr(VI_PIPE ViPipe, const ISP_COLOR_TONE_ATTR_S *pstCTAttr)
{


    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_GetColorToneAttr(VI_PIPE ViPipe, ISP_COLOR_TONE_ATTR_S *pstCTAttr)
{

    return GK_SUCCESS;
}

GK_S32 SAMPLE_MPI_ISP_QueryWBInfo(VI_PIPE ViPipe, ISP_WB_INFO_S *pstWBInfo)
{
    ALG_LIB_S stAwbLib;

    AWB_CHECK_DEV(ViPipe);
    AWB_CHECK_POINTER(pstWBInfo);

    stAwbLib.s32Id = 0;
    strncpy(stAwbLib.acLibName, "awb_lib", sizeof("awb_lib"));


    pstWBInfo->u16ColorTemp = ext_system_wb_detect_temp_read((GK_U8)stAwbLib.s32Id);

    return GK_SUCCESS;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
