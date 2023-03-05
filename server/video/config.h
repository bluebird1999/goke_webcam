#ifndef SERVER_VIDEO_CONFIG_H_
#define SERVER_VIDEO_CONFIG_H_

/*
 * header
 */
#include <stdbool.h>
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"
#include "../../server/goke/hisi/hisi_common.h"

/*
 * hisi define
*/

#define WDR_MAX_PIPE_NUM        2
#define IVE_MD_IMG_NUM          2
#define MAX_STREAM_NUM          5
#define MAX_OSD_LABLE_SIZE      19
/*
 * define
 */

#define     VIDEO_MAX_THREAD                    8
#define     MAX_OSD_OVERLAY                     3
#define ISP_AUTO_STENGTH_NUM 16

typedef enum HISI_ISP_MODE_E
{
    HISI_ISP_MODE_DAY = 1,
    HISI_ISP_MODE_NIGHT = 0,
}HISI_ISP_MODE_E;

typedef enum
{
    HISI_IMAGE_SATURATION = 0,
    HISI_IMAGE_CONTRAST,
    HISI_IMAGE_BRIGHTNESS,
    HISI_IMAGE_SHARPNESS
}HISI_IMAGE_E;

typedef struct HISI_EXPOSURE_INFO_S
{
    unsigned int ExpTime;
    unsigned int AGain;
    unsigned int DGain;
    unsigned int ISPGain;
    unsigned char AveLuma;
    unsigned int ColorTemp;
}HISI_EXPOSURE_INFO_S;

typedef struct
{
    unsigned char saturation;
    unsigned char contrast;
    unsigned char brightness;
    unsigned char sharpness;
}HISI_IMAGE_ATTR_S;

typedef struct
{
    int slow_shutter_enable;    //0 disable / 1 enable
    HISI_IMAGE_ATTR_S image_attr;
}HISI_ISP_S;

typedef enum GAMMA_LUMA_STATE
{
    GAMMA_LUMA_STATE_LOW = 0,
    GAMMA_LUMA_STATE_MIDDLE,
    GAMMA_LUMA_STATE_HIGH,
    GAMMA_LUMA_STATE_BUTT
}GAMMA_LUMA_STATE;

typedef enum ISO_LEVEL
{
    ISO_LEVEL_0 = 0,
    ISO_LEVEL_1,
    ISO_LEVEL_2,
    ISO_LEVEL_3,
    ISO_LEVEL_4,
    ISO_LEVEL_5,
    ISO_LEVEL_6,
    ISO_LEVEL_7,
    ISO_LEVEL_8,
    ISO_LEVEL_9,
    ISO_LEVEL_10,
    ISO_LEVEL_11,
    ISO_LEVEL_12,
    ISO_LEVEL_13,
    ISO_LEVEL_14,
    ISO_LEVEL_15,
    ISO_LEVEL_BUTT
}ISO_LEVEL;

typedef struct isp_config_t
{
    ISP_EXPOSURE_ATTR_S stExpAttr;
    ISP_AE_ROUTE_S stAERoute;
    ISP_BLACK_LEVEL_S stBlackLevel[ISP_AUTO_STENGTH_NUM];
    ISP_WB_ATTR_S stWBAttr;
    ISP_AWB_ATTR_EX_S stAWBAttrEx;
    //  ISP_COLORMATRIX_ATTR_S stCCMAttr;
    ISP_SATURATION_ATTR_S stSaturation;
    ISP_SHARPEN_ATTR_S stSharpenAttr;
    VPSS_GRP_NRX_PARAM_S astNR[ISP_AUTO_STENGTH_NUM];
    ISP_NR_ATTR_S stNRAttr;
    ISP_DEMOSAIC_ATTR_S stDemosaicAttr;
    ISP_ANTIFALSECOLOR_ATTR_S stAntiFC;
    ISP_GAMMA_ATTR_S stGammaAttr[GAMMA_LUMA_STATE_BUTT];
    HI_U16 GammaSysgainThreshold[GAMMA_LUMA_STATE_BUTT];
    ISP_DEMOSAIC_ATTR_S stDefogAttr[ISP_AUTO_STENGTH_NUM];
    ISP_DRC_ATTR_S astDrcAttr[ISP_AUTO_STENGTH_NUM];
    ISP_LDCI_ATTR_S stDci;
    ISP_DP_DYNAMIC_ATTR_S stDPDynamic;
//    ISP_UVNR_ATTR_S stUvnrAttr;
    ISP_CR_ATTR_S stCRAttr;

    ISO_LEVEL last_iso_level;
    GAMMA_LUMA_STATE last_gamma_state;
    int last_iso;

    //real time info
    ISP_EXP_INFO_S stExpInfo;

    //last isp day night mode
    HISI_ISP_MODE_E last_mode;
    HISI_IMAGE_ATTR_S image_attr;

    ISP_EXPOSURE_ATTR_S stSLExpAttr; //starlight config

    //drop frame cfg
    int bSlowShutter;
    int frame_level[5];
    int last_fps;
    int max_fps[2]; // current max fps
    int max_fps_cfg[2];  // config max fps
    struct timeval last_level_time;
    int delay_time; // 稳定时间(毫秒)
} isp_config_t;

typedef enum video_thread_id_enum {
    VIDEO_THREAD_ENCODER,
    VIDEO_THREAD_ISP = VPSS_MAX_PHY_CHN_NUM,
    VIDEO_THREAD_ISP_EXTRA,
    VIDEO_THREAD_OSD,
    VIDEO_THREAD_MD,
    VIDEO_THREAD_BUTT,
} video_thread_id_enum;

typedef enum vi_init_start_enum {
    VI_INIT_START_MIPI = 0,
    VI_INIT_START_DEVICE,
    VI_INIT_START_ISP,
    VI_INIT_START_PIPE,
    VI_INIT_START_CHANNEL,
    VI_INIT_START_BUTT,
} vi_init_start_enum;

typedef enum video_stream_id_e {
    ID_HIGH = 0,
    ID_LOW,
    ID_PIC,
    ID_MD,
    ID_BUT,
};

typedef enum video_region_id_e {
    ID_TIME = 0,
    ID_WARNING,
    ID_NOTICE,
};
/*
 * structure
 */
typedef struct stream_profile_info_t {
    PAYLOAD_TYPE_E      type;
    picture_size_t      size;
    int                 width;
    bool                enable;
    int                 height;
    int                 frame_rate;
    int                 profile;
    rc_mode_t           rc_mode;
    VENC_GOP_MODE_E     gop_mode;
    int                 gop;
    VPSS_CHN            vpss_chn;
    VENC_CHN            venc_chn;
} stream_profile_info_t;

typedef struct video_profile_info_t {
    stream_profile_info_t    stream[MAX_STREAM_NUM];
    bool                     enc_shared_buff;
} video_profile_info_t;

typedef struct vi_sensor_info_t {
    sensor_type_t       sensor_type;
    HI_S32              sensor_id;
    HI_S32              bus_id;
    combo_dev_t         mipi_device;
    char                device[MAX_SYSTEM_STRING_SIZE];
    HI_U32              width;
    HI_U32              height;
    HI_U32              framerate;
    HI_U32              full_line_height;       //line
    lane_divide_mode_t  lane_divide_mode;
} vi_sensor_info_t;

typedef struct vi_device_info_t {
    VI_DEV              device;
    WDR_MODE_E          wdr_mode;
} vi_device_info_t;

typedef struct vi_pipe_info_t {
    VI_PIPE         pipe[WDR_MAX_PIPE_NUM];
    VI_VPSS_MODE_E  mast_pipe_mode;
    HI_BOOL         multi_pipe;
    HI_BOOL         cnumber_configured;
    HI_BOOL         isp_bypass;
    PIXEL_FORMAT_E  pixel_format;
    HI_U32          vc_number[WDR_MAX_PIPE_NUM];
} vi_pipe_info_t;

typedef struct vi_channel_info_t {
    VI_CHN              channel;
    PIXEL_FORMAT_E      pixel_format;
    DYNAMIC_RANGE_E     dynamic_range;
    VIDEO_FORMAT_E      video_format;
    COMPRESS_MODE_E     compress_mode;
} vi_channel_info_t;

typedef struct vi_snap_info_t {
    HI_BOOL             snap;
    HI_BOOL             double_pipe;
    VI_PIPE             video_pipe;
    VI_PIPE             snap_pipe;
    VI_VPSS_MODE_E      video_pipe_mode;
    VI_VPSS_MODE_E      snap_pipe_mode;
} vi_snap_info_t;

typedef struct vi_info_t {
    vi_sensor_info_t        sensor_info;
    vi_device_info_t        device_info;
    vi_pipe_info_t          pipe_info;
    vi_channel_info_t       channel_info;
    vi_snap_info_t          snap_info;
} vi_info_t;

typedef struct vpss_info_t {
    int                     group;
    bool                    buff_wrap;
    bool                    enabled[VPSS_MAX_PHY_CHN_NUM];
    bool                    ldc;
    VI_VPSS_MODE_E          vi_vpss_mode;
    int                     wrap_buffer_line;
    VPSS_GRP_ATTR_S         group_info;
    VPSS_CHN_ATTR_S         channel_info[VPSS_MAX_PHY_CHN_NUM];
} vpss_info_t;

typedef struct venc_info_t {

} venc_info_t;

typedef struct region_layer_info_t {
    int         id;
    int         layer;
    PIXEL_FORMAT_E pixel_format;
    int         size;
    int         color;
    int         offset_left;
    int         offset_top;
    bool        rotate;
    int         alpha;
    char        font_face[MAX_SYSTEM_STRING_SIZE];
    int         venc_chn;
} region_layer_info_t;

typedef struct region_info_t {
    int                 time_big_id;
    int                 time_small_id;
    int                 notice_id;
    region_layer_info_t overlay[MAX_OSD_OVERLAY];
} region_info_t;

typedef struct md_config_t {
    IVE_SRC_IMAGE_S     ast_img[IVE_MD_IMG_NUM];
    IVE_DST_MEM_INFO_S  blob;
    MD_ATTR_S           md_attr;
    rect_array_t        region;
    int                 sad_thresh[6];
    int                 alarm_interval[3];
    int                 md_channel;
} md_config_t;

typedef struct od_config_t {
    double              thresh_value;
} od_config_t;

typedef struct video_config_t {
    video_profile_info_t        profile;
    vi_info_t                   vi;
    VB_CONFIG_S                 vb;
    vpss_info_t                 vpss;
    venc_info_t                 venc;
    region_info_t               region;
    md_config_t                 md;
    isp_config_t                isp;
    od_config_t                 od;
} video_config_t;

/*
 * function
 */
video_config_t video_config;

#endif
