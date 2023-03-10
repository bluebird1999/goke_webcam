#ifndef COMMON_HISI_COMMON_H
#define COMMON_HISI_COMMON_H

#include "hi_common.h"
#include "hi_buffer.h"
#include "hi_comm_sys.h"
#include "hi_comm_vb.h"
#include "hi_comm_isp.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "hi_comm_venc.h"
#include "hi_comm_vpss.h"
#include "hi_comm_region.h"
#include "hi_comm_adec.h"
#include "hi_comm_aenc.h"
#include "hi_comm_ai.h"
#include "hi_comm_ao.h"
#include "hi_comm_aio.h"
#include "hi_defines.h"
#include "hi_mipi.h"
#include "hi_comm_vgs.h"
#include "hi_comm_ive.h"
#include "ivs_md.h"
#include "hi_ivp.h"
#include "hi_securec.h"
#include "hi_type.h"
#include "hi_common.h"

#include "mpi_sys.h"
#include "mpi_vb.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_vpss.h"
#include "mpi_region.h"
#include "mpi_audio.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_ive.h"
#include "hi_math.h"
#include "hi_sns_ctrl.h"
#include "mpi_vgs.h"
#include "audio_acodec.h"

#include "hi_resampler_api.h"
#include "hi_vqe_register_api.h"

#define IVE_RECT_NUM   20

typedef enum sensor_type_t {
    SONY_IMX327_MIPI_2M_30FPS_12BIT,
    SONY_IMX327_MIPI_2M_30FPS_12BIT_WDR2TO1,
    SONY_IMX327_2L_MIPI_2M_30FPS_12BIT,
    SONY_IMX327_2L_MIPI_2M_30FPS_12BIT_WDR2TO1,
    SONY_IMX307_MIPI_2M_30FPS_12BIT,
    SONY_IMX307_MIPI_2M_30FPS_12BIT_WDR2TO1,
    SONY_IMX307_2L_MIPI_2M_30FPS_12BIT,
    SONY_IMX307_2L_MIPI_2M_30FPS_12BIT_WDR2TO1,
    SONY_IMX335_MIPI_5M_30FPS_12BIT,
    SONY_IMX335_MIPI_5M_30FPS_10BIT_WDR2TO1,
    SONY_IMX335_MIPI_4M_30FPS_12BIT,
    SONY_IMX335_MIPI_4M_30FPS_10BIT_WDR2TO1,
    SMART_SC4236_MIPI_3M_30FPS_10BIT,
    SMART_SC4236_MIPI_3M_20FPS_10BIT,
    SMART_SC2231_MIPI_2M_30FPS_10BIT,
    SOI_JXF37_MIPI_2M_30FPS_10BIT,
    SMART_SC2235_DC_2M_30FPS_10BIT,
    SMART_SC3235_MIPI_3M_30FPS_10BIT,
    GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT,
    GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT_FORCAR,
    OMNIVISION_OS05A_MIPI_4M_30FPS_12BIT,
    OMNIVISION_OS05A_MIPI_5M_30FPS_12BIT,
    OMNIVISION_OS05A_MIPI_4M_30FPS_10BIT_WDR2TO1,
    BT1120_2M_30FPS_8BIT,
    BT656_2M_30FPS_8BIT,
    BT601_2M_30FPS_8BIT,
    SAMPLE_SNS_TYPE_BUTT,
} sensor_type_t;

typedef enum IOC_NR_ISP_E {
    IOC_NR_ISP_SET_FD = 0,
    IOC_NR_ISP_GET_FRAME_EDGE,
    IOC_NR_ISP_GET_VD_TIMEOUT,
    IOC_NR_ISP_GET_VD_END_TIMEOUT,
    IOC_NR_ISP_GET_VD_BEEND_TIMEOUT,
    IOC_NR_ISP_SET_INT_ENABLE,
    IOC_NR_ISP_GET_VERSION,

    IOC_NR_ISP_STAT_BUF_INIT,
    IOC_NR_ISP_STAT_BUF_EXIT,
    IOC_NR_ISP_STAT_BUF_GET,
    IOC_NR_ISP_STAT_BUF_PUT,
    IOC_NR_ISP_STAT_ACT_GET,
    IOC_NR_ISP_REG_CFG_INIT,
    IOC_NR_ISP_REG_CFG_SET,
    IOC_NR_ISP_BE_CFG_BUF_INIT,
    IOC_NR_ISP_GET_BE_BUF_FIRST,
    IOC_NR_ISP_BE_FREE_BUF_GET,
    IOC_NR_ISP_BE_LAST_BUF_GET,
    IOC_NR_ISP_BE_CFG_BUF_EXIT,
    IOC_NR_ISP_BE_CFG_BUF_CTL,
    IOC_NR_ISP_BE_CFG_BUF_RUNNING,
    IOC_NR_ISP_BE_All_BUF_INIT,
    IOC_NR_ISP_PROC_INIT,
    IOC_NR_ISP_PROC_WRITE_ING,
    IOC_NR_ISP_PROC_WRITE_OK,
    IOC_NR_ISP_PROC_EXIT,
    IOC_NR_ISP_PROC_PARAM_GET,
    IOC_NR_ISP_TRANS_BUF_INIT,
    IOC_NR_ISP_TRANS_BUF_EXIT,
    IOC_NR_ISP_PRO_BUF_INIT,
    IOC_NR_ISP_PRO_BUF_EXIT,
    IOC_NR_ISP_SYNC_CFG_SET,
    IOC_NR_ISP_SYNC_STITCH_PARAM_INIT,
    IOC_NR_WDR_SYNC_CFG_SET,
    IOC_NR_ISP_MEM_INFO_SET,
    IOC_NR_ISP_MEM_INFO_GET,
    IOC_NR_ISP_P2EN_INFO_GET,
    IOC_NR_ISP_INIT_INFO_SET,
    IOC_NR_ISP_SYNC_INIT_SET,
    IOC_NR_ISP_RUN_STATE_SET,
    IOC_NR_ISP_RESET_CTX,
    IOC_NR_ISP_CONFIG_INFO_SET,
    IOC_NR_ISP_SNAP_INFO_GET,
    IOC_NR_ISP_PRO_EN_GET,
    IOC_NR_ISP_UPDATE_POS_GET,
    IOC_NR_ISP_FRAME_CNT_GET,
    IOC_NR_ISP_SNAP_ATTR_GET,
    IOC_NR_ISP_PWM_NUM_GET,
    IOC_NR_ISP_RES_SWITCH_SET,
    IOC_NR_ISP_CHN_SELECT_CFG,
    IOC_NR_ISP_BE_SWITCH_FINISH_STATE_SET,
    IOC_NR_ISP_MODE_SWITCH_SET,

    IOC_NR_ISP_FRAME_INFO_SET,
    IOC_NR_ISP_FRAME_INFO_GET,
    IOC_NR_ISP_DNG_INFO_SET,
    IOC_NR_ISP_DNG_INFO_GET,
    IOC_NR_ISP_SET_MOD_PARAM,
    IOC_NR_ISP_GET_MOD_PARAM,
    IOC_NR_ISP_SET_CTRL_PARAM,
    IOC_NR_ISP_GET_CTRL_PARAM,

    IOC_NR_ISP_MODE_INFO,
    IOC_NR_ISP_WORK_MODE_INIT,
    IOC_NR_ISP_WORK_MODE_GET,
    IOC_NR_ISP_WORK_MODE_EXIT,
    IOC_NR_ISP_PRE_BLK_NUM_UPDATE,
    IOC_NR_ISP_GET_HDR_ATTR,
    IOC_NR_ISP_GET_WDR_ATTR,
    IOC_NR_ISP_GET_STITCH_ATTR,
    IOC_NR_ISP_GET_FUSION_ATTR,
    IOC_NR_ISP_GET_PIPE_SIZE,
    IOC_NR_ISP_PUB_ATTR_INFO,
    IOC_NR_ISP_CLUT_BUF_INIT,
    IOC_NR_ISP_CLUT_BUF_EXIT,
    IOC_NR_ISP_CLUT_BUF_GET,
    IOC_NR_ISP_STT_BUF_INIT,
    IOC_NR_ISP_STT_BUF_EXIT,
    IOC_NR_ISP_STT_ADDR_INIT,
    IOC_NR_ISP_BE_LUT_STT_BUF_GET,
    IOC_NR_ISP_KERNEL_RUNONCE,
    IOC_NR_ISP_YUV_RUNONCE_INFO,
    IOC_NR_ISP_KERNEL_YUV_RUNONCE,
    IOC_NR_ISP_OPT_RUNONCE_INFO,
    IOC_NR_ISP_SET_PROCALCDONE,
    IOC_NR_ISP_SPECAWB_BUF_INIT,
    IOC_NR_ISP_SPECAWB_BUF_EXIT,
    IOC_NR_ISP_SPECAWB_BUF_GET,
    IOC_NR_ISP_SET_RGBIR_FORMAT,
    IOC_NR_ISP_UPDATE_INFO_SET,
    IOC_NR_ISP_BE_SYNC_PARAM_INIT,
    IOC_NR_ISP_LDCI_BUF_INIT,
    IOC_NR_ISP_LDCI_BUF_EXIT,
    IOC_NR_ISP_LDCI_READ_STT_BUF_GET,
    IOC_NR_ISP_BUTT,
} IOC_NR_ISP_E;

typedef enum rc_mode_t {
    RC_MODE_CBR = 0,
    RC_MODE_VBR,
    RC_MODE_CVBR,
    RC_MODE_AVBR,
    RC_MODE_QPMAP,
    RC_MODE_FIXQP,
    RC_MODE_QVBR,
    RC_MODE_BUTT,
} rc_mode_t;

typedef enum picture_size_t {
    PIC_CIF,
    PIC_360P,      /* 640 * 360 */
    PIC_VGA,
    PIC_640x360,
    PIC_D1_PAL,    /* 720 * 576 */
    PIC_D1_NTSC,   /* 720 * 480 */
    PIC_720P,      /* 1280 * 720  */
    PIC_1080P,     /* 1920 * 1080 */
    PIC_2304x1296,
    PIC_2592x1520,
    PIC_2592x1536,
    PIC_2592x1944,
    PIC_2688x1520,
    PIC_2688x1536,
    PIC_2688x1944,
    PIC_2716x1524,
    PIC_3840x2160,
    PIC_4096x2160,
    PIC_3000x3000,
    PIC_4000x3000,
    PIC_7680x4320,
    PIC_3840x8640,
    PIC_BUTT
} picture_size_t;

typedef struct rect_t {
    POINT_S     points[4];
} rect_t;

typedef struct rect_array_t {
    HI_U16 u16Num;
    rect_t astRect[IVE_RECT_NUM];
} rect_array_t;

#define IVE_CONVERT_64BIT_ADDR(Type,Addr) (Type*)(HI_UL)(Addr)

//free mmz
#define IVE_MMZ_FREE(phy,vir)\
    do{\
        if ((0 != (phy)) && (0 != (vir)))\
        {\
            HI_MPI_SYS_MmzFree((phy),(HI_VOID *)(HI_UL)(vir));\
            (phy) = 0;\
            (vir) = 0;\
        }\
    }while(0)

/*
 * external
 */
extern ISP_PUB_ATTR_S ISP_PUB_ATTR_GC2063;
extern combo_dev_attr_t SENSOR_GC2063_CHN1_ATTR;
extern VI_DEV_ATTR_S DEV_ATTR_GC2063_BASE;
extern VI_PIPE_ATTR_S PIPE_ATTR_1920x1080_RAW10_420_3DNR_RFR;
extern VI_CHN_ATTR_S CHN_ATTR_1920x1080_420_SDR8_LINEAR;
#endif
