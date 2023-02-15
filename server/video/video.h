#ifndef SERVER_VIDEO_VIDEO_H_
#define SERVER_VIDEO_VIDEO_H_

/*
 * header
 */
#include "video_interface.h"
#include "config.h"

/*
 * define
 */
#define		VIDEO_RUN_MODE_SAVE				0
#define		VIDEO_RUN_MODE_SAVE_EXTRA		4
#define 	VIDEO_RUN_MODE_ALIYUN    	    8
#define     VIDEO_RUN_MODE_ALIYUN_EXTRA     12
#define		VIDEO_RUN_MODE_MOTION			16
#define		VIDEO_RUN_MODE_PERSISTENT		20

typedef enum video_init_condition_t {
    VIDEO_INIT_CONDITION_GOKE = 0,
    VIDEO_INIT_CONDITION_NUM,
} video_init_condition_t;

#define		VIDEO_EXIT_CONDITION				( (1 << SERVER_RECORDER) | (1 << SERVER_ALIYUN) )


/*
 * structure
 */
typedef struct video_running_info_t {
    pthread_t                   thread_id[VIDEO_MAX_THREAD];
    vi_init_start_enum          vi_init;
    vi_init_start_enum          vi_start;
    bool                        vpss_init;
    bool                        vpss_start;
    bool                        venc_init[VPSS_MAX_PHY_CHN_NUM];
    bool                        venc_start[VPSS_MAX_PHY_CHN_NUM];
    bool                        region_init;
    bool                        region_start;
    bool                        md_init;
    bool                        md_start;
    bool                        md_run;
} video_running_info_t;

typedef struct video_stream_info_t {
    int                     channel;
    int 	                frame;
    int                     width;
    int                     height;
    int                     fps;
    int                     key_frame;
    VENC_RC_MODE_E          rc_mode;
    unsigned long long int  goke_stamp;
    unsigned long long int  unix_stamp;
} video_stream_info_t;

typedef struct video_channel_t {
    channel_status_t        status;
    server_type_t           channel_type;
    video_stream_type_t     stream_type;
    int                     service_id;
    bool                    require_key;
} video_channel_t;

/*
 * function
 */

#endif
