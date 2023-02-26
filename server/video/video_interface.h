#ifndef SERVER_VIDEO_VIDEO_INTERFACE_H_
#define SERVER_VIDEO_VIDEO_INTERFACE_H_

/*
 * header
 */
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"

/*
 * define
 */
#define		SERVER_VIDEO_VERSION_STRING		"alpha-8.1"


//video control command from
//standard camera
#define 	VIDEO_PROPERTY_NIGHT_SHOT               	(0x0002)
#define 	VIDEO_PROPERTY_TIME_WATERMARK           	(0x0003)
#define     VIDEO_PROPERTY_IMAGE_FLIP                   (0x0004)
//standard motion detection
#define 	VIDEO_PROPERTY_MOTION_SWITCH          		(0x0010)
#define 	VIDEO_PROPERTY_MOTION_ALARM_INTERVAL    	(0x0011)
#define 	VIDEO_PROPERTY_MOTION_SENSITIVITY 			(0x0012)
#define 	VIDEO_PROPERTY_MOTION_PLAN		        	(0x0013)
//qcy custom
#define 	VIDEO_PROPERTY_CUSTOM_WARNING_PUSH         	(0x0100)
#define 	VIDEO_PROPERTY_CUSTOM_DISTORTION          	(0x0101)
//video control command others
#define		VIDEO_PROPERTY_QUALITY						(0x0020)
#define		VIDEO_PROPERTY_SUBQUALITY					(0x0021)
#define		VIDEO_PROPERTY_WDR					        (0x0022)
#define     VIDEO_PROPERTY_IMAGE_CORRECTION             (0x0023)

typedef enum video_stream_type_t {
    VIDEO_STREAM_HIGH = 0,
    VIDEO_STREAM_LOW,
} video_stream_type_t;

typedef enum snap_type_t {
    SNAP_TYPE_NORMAL = 0,
    SNAP_TYPE_BARCODE,
    SNAP_TYPE_CLOUD,
    SNAP_TYPE_BUT,
} snap_type_t;
/*
 * structure
 */


/*
 * function
 */
int server_video_start(void);
int server_video_message( message_t *msg);
int server_video_snap_message(message_t *msg);
int audio_video_synchronize(int id);
#endif
