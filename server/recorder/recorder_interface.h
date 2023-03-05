#ifndef SERVER_RECORDER_RECORDER_INTERFACE_H_
#define SERVER_RECORDER_RECORDER_INTERFACE_H_

/*
 * header
 */
#include <mp4v2/mp4v2.h>
#include <pthread.h>
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"
#include "config.h"

/*
 * define
 */
#define		SERVER_RECORDER_VERSION_STRING		"alpha-8.0"

#define		RECORDER_MODE_BY_TIME					0x00
#define		RECORDER_MODE_BY_SIZE					0x01

#define		MAX_RECORDER_JOB						3

#define		RECORDER_PROPERTY_SERVER_STATUS				(0x000)
#define		RECORDER_PROPERTY_CONFIG_STATUS				(0x001)
#define		RECORDER_PROPERTY_SAVE_MODE					(0x002)
#define		RECORDER_PROPERTY_RECORDING_MODE			(0x003)
#define		RECORDER_PROPERTY_NORMAL_DIRECTORY			(0x004)
#define     RECORDER_PROPERTY_SWITCH                    (0x005)

/*
 * structure
 */
typedef enum {
	RECORDER_THREAD_NONE = 0,
	RECORDER_THREAD_INITED,
	RECORDER_THREAD_STARTED,
	RECORDER_THREAD_RUN,
	RECORDER_THREAD_PAUSE,
	RECORDER_THREAD_ERROR,
};

typedef struct recorder_init_t {
	int		type;
    int     sub_type;
	int		mode;
	int		repeat;
	int		repeat_interval;
    int		audio;
    int		quality;
    int		video_channel;
    char   	start[MAX_SYSTEM_STRING_SIZE];
    char   	stop[MAX_SYSTEM_STRING_SIZE];
    HANDLER	func;
} recorder_init_t;

typedef enum aliyun_recorder_mode_t {
    ALIYUN_RECORDER_MODE_NONE,
    ALIYUN_RECORDER_MODE_ALARM,
    ALIYUN_RECORDER_MODE_PLAN,
    ALIYUN_RECORDER_MODE_ALL,
    ALIYUN_RECORDER_MODE_BUTT,
} aliyun_recorder_mode_t;

typedef struct recorder_run_t {
	char   				file_path[MAX_SYSTEM_STRING_SIZE*2];
	MP4FileHandle 		mp4_file;
	MP4TrackId 			video_track;
	MP4TrackId 			audio_track;
    FILE    			*file;
	unsigned long long 	start;
	unsigned long long 	stop;
	unsigned long long 	real_start;
	unsigned long long 	real_stop;
	unsigned long long	last_write;
	unsigned long long	last_vframe_stamp;
	unsigned long long	last_aframe_stamp;
	unsigned long long 	first_frame_stamp;
	char				first_audio;
	char				sps_read;
	char				pps_read;
	char				first_frame;
    int					fps;
    int					width;
    int					height;
    char				exit;
} recorder_run_t;

typedef struct recorder_job_t {
	char				status;
	char				t_id;
	recorder_init_t		init;
	recorder_run_t		run;
} recorder_job_t;
/*
 * function
 */
int server_recorder_start(void);
int server_recorder_message(message_t *msg);
int server_recorder_video_message(message_t *msg);
int server_recorder_audio_message(message_t *msg);
void server_recorder_interrupt_routine(int param);

#endif
