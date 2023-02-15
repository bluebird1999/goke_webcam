#ifndef SERVER_PLAYER_PLAYER_H_
#define SERVER_PLAYER_PLAYER_H_

/*
 * header
 */

/*
 * define
 */
typedef enum player_init_condition_t {
    PLAYER_INIT_CONDITION_DEVICE_SD = 0,
    PLAYER_INIT_CONDITION_TIME_SYCHRONIZED,
    PLAYER_INIT_CONDITION_NUM,
} player_init_condition_t;

#define		DEFAULT_SYNC_DURATION					67			//67ms

#define		PLAYER_EXIT_CONDITION					( (1 << SERVER_ALIYUN) )


typedef enum {
	PLAYER_THREAD_NONE = 0,
	PLAYER_THREAD_INITED,
    PLAYER_THREAD_SYNC,
	PLAYER_THREAD_IDLE,
	PLAYER_THREAD_RUN,
	PLAYER_THREAD_STOP,
	PLAYER_THREAD_PAUSE,
	PLAYER_THREAD_FINISH,
	PLAYER_THREAD_ERROR,
};

typedef enum {
    PLAYER_SYNC_VIDEO = 0,
    PLAYER_SYNC_AUDIO,
    PLAYER_SYNC_BUTT,
};

/*
 * structure
 */
typedef struct player_run_t {
	file_list_node_t	*current;
	char   				file_path[MAX_SYSTEM_STRING_SIZE*2];
	MP4FileHandle 		mp4_file;
	MP4TrackId 			video_track;
	MP4TrackId 			audio_track;
	int					video_frame_num;
	int					audio_frame_num;
	int					video_timescale;
	int					audio_timescale;
	int					video_index;
	int					audio_index;
	int					video_codec;
	int					audio_codec;
	int					duration;
    int 				slen;
    unsigned char 		sps[MAX_SYSTEM_STRING_SIZE*4];
    int 				plen;
    unsigned char 		pps[MAX_SYSTEM_STRING_SIZE*4];
    char				vstream_wait;
    char				astream_wait;
    unsigned long long	video_sync;
    unsigned long long	audio_sync;
	unsigned long long 	start;
	unsigned long long 	stop;
	char				i_frame_read;
    int					fps;
    int					width;
    int					height;
} player_run_t;

typedef struct player_job_t {
	//shared
	char				status;
	char				run;
	char				speed;
	char				restart;
	char				exit;
    char                pause;
	char				audio;
	//non-shared
	char				channel;
	char				sid;
	player_init_t		init;
    int                 type;
    int                 live_sync;
} player_job_t;

/*
 * function
 */

#endif
