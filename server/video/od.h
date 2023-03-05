#ifndef SERVER_VIDEO_IVP_H
#define SERVER_VIDEO_IVP_H

#include "../../global/global_interface.h"

#define IVP_RESOURCE_ALLDAY     "/misc/model/ivp_re_allday_f1y4f2m3_640x360_v1006.oms"

#define MAX_CLASS_NUM 2
#define MAX_RECT_NUM 10

typedef struct ivp_param_t {
    hi_s32 ivp_handle;
    hi_ivp_mem_info ivp_mem_info;
} ivp_param_t;

typedef struct ivp_effect_param_t {
    hi_bool low_bitrate_en;
    hi_bool advance_isp_en;
} ivp_effect_param_t;


HI_S32 od_init(void);
HI_S32 od_uninit(void);
HI_VOID od_get_param(ivp_param_t *ivp_param);

#endif
