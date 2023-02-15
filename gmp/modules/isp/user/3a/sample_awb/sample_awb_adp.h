/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#ifndef __SAMPLE_AWB_ADP_H__
#define __SAMPLE_AWB_ADP_H__

#include <string.h>
#include "type.h"
#include "comm_3a.h"
#include "awb_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct SAMPLE_AWB_CTX_S {
    /* usr var */
    GK_U16                  u16DetectTemp;
    GK_U8                   u8WbType;

    /* communicate with isp */
    ISP_AWB_PARAM_S         stAwbParam;
    GK_U32                  u32FrameCnt;
    ISP_AWB_INFO_S          stAwbInfo;
    ISP_AWB_RESULT_S        stAwbResult;
    VI_PIPE                 IspBindDev;

    /* communicate with sensor, defined by user. */
    GK_BOOL                 bSnsRegister;
    ISP_SNS_ATTR_INFO_S     stSnsAttrInfo;
    AWB_SENSOR_DEFAULT_S    stSnsDft;
    AWB_SENSOR_REGISTER_S   stSnsRegister;

    /* global variables of awb algorithm */
} SAMPLE_AWB_CTX_S;

#define MAX_AWB_REGISTER_SNS_NUM 1

extern SAMPLE_AWB_CTX_S g_astAwbCtx[MAX_AWB_LIB_NUM];

/* we assumed that the different lib instance have different id,
 * use the id 0 & 1.
 */

#define AWB_GET_EXTREG_ID(s32Handle)   (((s32Handle) == 0) ? 0x4 : 0x5)

#define AWB_GET_CTX(s32Handle)           (&g_astAwbCtx[s32Handle])

#define AWB_CHECK_HANDLE_ID(s32Handle)\
    do {\
        if (((s32Handle) < 0) || ((s32Handle) >= MAX_AWB_LIB_NUM))\
        {\
            printf("Illegal handle id %d in %s!\n", (s32Handle), __FUNCTION__);\
            return GK_FAILURE;\
        }\
    }while(0)

#define AWB_CHECK_LIB_NAME(acName)\
    do {\
        if (strncmp((acName), ISP_AWB_LIB_NAME, ALG_LIB_NAME_SIZE_MAX) != 0)\
        {\
            printf("Illegal lib name %s in %s!\n", (acName), __FUNCTION__);\
            return GK_FAILURE;\
        }\
    }while(0)

#define AWB_CHECK_POINTER(ptr)\
    do {\
        if (ptr == GK_NULL)\
        {\
            printf("Null Pointer in %s!\n", __FUNCTION__);\
            return GK_FAILURE;\
        }\
    }while(0)

#define AWB_CHECK_DEV(dev)\
    do {\
        if (((dev) < 0) || ((dev) >= ISP_MAX_PIPE_NUM))\
        {\
            ISP_TRACE(MODULE_DBG_ERR, "Err AWB dev %d in %s!\n", dev, __FUNCTION__);\
            return ERR_CODE_ISP_ILLEGAL_PARAM;\
        }\
    }while(0)

#define AWB_TRACE(level, fmt, ...)\
    do{ \
        MODULE_TRACE(level,MOD_ID_ISP,"[Func]:%s [Line]:%d [Info]:"fmt,__FUNCTION__, __LINE__,##__VA_ARGS__);\
    }while(0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
