#ifndef SERVER_GOKE_GOKE_INTERFACE_H_
#define SERVER_GOKE_GOKE_INTERFACE_H_

/*
 * header
 */
#include "../../global/global_interface.h"

/*
 * define
 */
#define		SERVER_GOKE_VERSION_STRING		"alpha-8.1"

#define     GOKE_LIVE_BASE_MS               100000
#define     MAX_AV_CHANNEL                  4

#define		MAX_AUDIO_FRAME_SIZE		2*1024
#define		MAX_VIDEO_FRAME_SIZE		128*1024

#define     QOS_MAX_LATENCY             10*1000 //10s
/*
 * structure
 */
typedef struct av_data_info_t {
    unsigned int	flag;
    unsigned int	index;
    unsigned int	frame_index;
    unsigned int	type;
    unsigned int	volume_l;
    unsigned int	volume_r;
    unsigned int	timestamp;
    unsigned int	fps;
    unsigned int	width;
    unsigned int	height;
    unsigned int	size;
    unsigned int	source;
    unsigned int	channel;
    unsigned int    key_frame;
} av_data_info_t;

typedef enum channel_status_t {
    CHANNEL_EMPTY = 0,
    CHANNEL_VALID,
} channel_status_t;

typedef enum goke_status_t {
    GOKE_PROPERTY_STATUS = 0,
    GOKE_PROPERTY_BUTT,
} goke_status_t;

/*
 * structure
 */


/*
 * function
 */
int server_goke_start(void);
int server_goke_message( message_t *msg);

#endif
