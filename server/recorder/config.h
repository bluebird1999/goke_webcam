#ifndef SERVER_RECORDER_CONFIG_H_
#define SERVER_RECORDER_CONFIG_H_

/*
 * header
 */
#include "../../global/global_interface.h"

/*
 * define
 */

/*
 * structure
 */
typedef struct recorder_quality_t {
	int	bitrate;
	int audio_sample;
} recorder_quality_t;


typedef struct recorder_config_t {
    unsigned int		mode;
    char				normal_start[MAX_SYSTEM_STRING_SIZE];
    char				normal_end[MAX_SYSTEM_STRING_SIZE];
    int					normal_repeat;
    int					normal_repeat_interval;
    int					normal_audio;
    int					normal_quality;
    int                 alarm_length;
    recorder_quality_t	quality[3];
    unsigned int		max_length;		//in seconds
    unsigned int		min_length;     //
} recorder_config_t;

recorder_config_t recorder_config;
/*
 * function
 */

#endif
