/*
 * header
 */
//system header
//program header
#include "../../server/video/video_interface.h"
//server header
#include "config.h"

/*
 * static
 */
recorder_config_t recorder_config = {
    .mode = 0,
    .normal_start = "0",
    .normal_end = "0",
    .normal_repeat = 1,
    .normal_repeat_interval = 0,
    .normal_audio = 1,
    .normal_quality = VIDEO_STREAM_HIGH,
    .alarm_length = 60,
    .quality[0].bitrate = 512,
    .quality[0].audio_sample = 8000,
    .quality[1].bitrate = 1024,
    .quality[1].audio_sample = 8000,
    .quality[2].bitrate = 2048,
    .quality[2].audio_sample = 8000,
    .max_length = 600,
    .min_length = 3,
};