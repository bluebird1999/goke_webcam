#ifndef SERVER_AUDIO_AUDIO_H_
#define SERVER_AUDIO_AUDIO_H_

/*
 * header
 */
#include <stdbool.h>

/*
 * define
 */
#define		AUDIO_RUN_MODE_SAVE				    0
#define 	AUDIO_RUN_MODE_ALIYUN    	        4

typedef enum audio_init_condition_e {
    AUDIO_INIT_CONDITION_GOKE = 0,
    AUDIO_INIT_CONDITION_NUM,
};
#define		AUDIO_EXIT_CONDITION			( (1 << SERVER_ALIYUN) | (1 << SERVER_RECORDER) )

#define AUDIO_SCAN_START 			"/misc/audio/scan_start.alaw"
#define AUDIO_SCAN_SUCCESS			"/misc/audio/scan_success.alaw"
#define AUDIO_BINDING_SUCCESS       "/misc/audio/binding_success.alaw"
#define AUDIO_START                 "/misc/audio/start.alaw"
#define AUDIO_REBOOT                "/misc/audio/scan_success.alaw"
#define AUDIO_RESET                 "/misc/audio/scan_success.alaw"

/*
 * structure
 */
#define SPEAKER_BUFFER_SIZE         512

typedef enum audio_thread_id_enum {
    AUDIO_THREAD_ENCODER = 0,
    AUDIO_THREAD_BUTT,
} thread_id_enum;

typedef struct audio_running_info_t {
    pthread_t                   thread_id;
    bool                        ai_init;
    bool                        ai_start;
    bool                        codec_init;
    bool                        ae_init;
    bool                        ae_start;
    bool                        ao_init;
    bool                        ao_start;
    bool                        ad_init;
    bool                        ad_start;
    bool                        ae_ai_bind;
    bool                        ao_ad_bind;
} audio_running_info_t;

typedef struct audio_stream_info_t {
    int                     channel;
    int 	                frame;
    int                     width;
    int                     height;
    int                     fps;
    unsigned long long int  goke_stamp;
    unsigned long long int  unix_stamp;
} audio_stream_info_t;

typedef struct audio_channel_t {
    channel_status_t        status;
    server_type_t           channel_type;
    int                     service_id;
    bool                    require_key;
    int                     qos_success;
} audio_channel_t;
/*
 * function
 */

#endif
