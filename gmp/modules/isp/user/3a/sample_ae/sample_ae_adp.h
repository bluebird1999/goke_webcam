/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */
#ifndef __SAMPLE_AE_ADP_H__
#define __SAMPLE_AE_ADP_H__

#include <string.h>
#include "type.h"
#include "comm_3a.h"
#include "ae_comm.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

typedef struct SAMPLE_AE_CTX_S {
    /* usr var */
    GK_U8                   u8AeMode;

    /* communicate with isp */
    ISP_AE_PARAM_S          stAeParam;
    GK_U32                  u32FrameCnt;
    ISP_AE_INFO_S           stAeInfo;
    ISP_AE_RESULT_S         stAeResult;
    VI_PIPE                 IspBindDev;

    /* communicate with sensor, defined by user. */
    GK_BOOL                 bSnsRegister;
    ISP_SNS_ATTR_INFO_S     stSnsAttrInfo;
    AE_SENSOR_DEFAULT_S     stSnsDft;
    AE_SENSOR_REGISTER_S    stSnsRegister;

    /* global variables of ae algorithm */
} SAMPLE_AE_CTX_S;

#define MAX_AE_REGISTER_SNS_NUM 1
#define GAIN_USER_LINEAR_SHIFT  10

extern SAMPLE_AE_CTX_S g_astAeCtx_sample[MAX_AE_LIB_NUM];
extern GK_U8 g_u8AeExtRegId;

/* we assumed that the different lib instance have different id,
 * use the id 0 & 1.
 */
#define AE_GET_EXTREG_ID(s32Handle)   (((s32Handle) == 0) ? 0x4 : 0x5)

#define AE_GET_CTX(s32Handle)           (&g_astAeCtx_sample[s32Handle])

#define AE_CHECK_HANDLE_ID(s32Handle)\
    do {\
        if (((s32Handle) < 0) || ((s32Handle) >= MAX_AE_LIB_NUM))\
        {\
            printf("Illegal handle id %d in %s!\n", (s32Handle), __FUNCTION__);\
            return GK_FAILURE;\
        }\
    }while(0)

#define AE_CHECK_LIB_NAME(acName)\
    do {\
        if (strcmp((acName), ISP_AE_LIB_NAME) != 0)\
        {\
            printf("Illegal lib name %s in %s!\n", (acName), __FUNCTION__);\
            return GK_FAILURE;\
        }\
    }while(0)

#define AE_CHECK_POINTER(ptr)\
    do {\
        if (ptr == GK_NULL)\
        {\
            printf("Null Pointer in %s!\n", __FUNCTION__);\
            return GK_FAILURE;\
        }\
    }while(0)


#define AE_CHECK_DEV(dev)\
    do {\
        if (((dev) < 0) || ((dev) >= ISP_MAX_PIPE_NUM))\
        {\
            ISP_TRACE(MODULE_DBG_ERR, "Err AE dev %d in %s!\n", dev, __FUNCTION__);\
            return ERR_CODE_ISP_ILLEGAL_PARAM;\
        }\
    }while(0)

#define AE_TRACE(level, fmt, ...)\
    do{ \
        MODULE_TRACE(level,MOD_ID_ISP,"[Func]:%s [Line]:%d [Info]:"fmt,__FUNCTION__, __LINE__,##__VA_ARGS__);\
    }while(0)

#define AE_FREE(ptr)\
    do{\
        if (ptr != GK_NULL)\
        {\
            free(ptr);\
            ptr = GK_NULL;\
        }\
    } while (0)

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif
