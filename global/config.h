#ifndef GLOBAL_CONFIG_H_
#define GLOBAL_CONFIG_H_

#include <iot_export.h>
#include "../../server/goke/hisi/hisi_common.h"

#define CONFIG_PATH         "/app/bin/webcam.config"

#define 			        MAX_SYSTEM_STRING_SIZE 		64
#define						MAX_DEVINFO_SIZE			128
#define						DEVINFO_FLAG_MAGIC			0xfdb97531
typedef struct sleep_t {
    unsigned char enable;
    unsigned char repeat;
    unsigned char weekday;                //0~7;
    unsigned char start[MAX_SYSTEM_STRING_SIZE];
    unsigned char stop[MAX_SYSTEM_STRING_SIZE];
} sleep_t;

typedef enum global_running_mode_t {
    RUNNING_MODE_NONE,            //	0
    RUNNING_MODE_SLEEP,            //		1
    RUNNING_MODE_NORMAL,        //			2
    RUNNING_MODE_TESTING,        //		3
    RUNNING_MODE_DEEP_SLEEP,
    RUNNING_MODE_EXIT,
    RUNNING_MODE_BUTT,
} global_running_mode_t;

typedef struct scheduler_plan_t {
    int begin;
    int end;
} scheduler_plan_t;

/*
 * header
 */

typedef struct config_t {
    unsigned char video_quality;
    unsigned char subvideo_quality;
    unsigned char alarm_switch;
    unsigned char motion_detect_sensitivity;
    unsigned char alarm_freqency;
    unsigned char video_mirror;
    char ipc_version[32];
    unsigned char day_night_mode;//日夜模式 0白天 1夜晚 2自动
    unsigned char motion_scheduler_enable;
    scheduler_plan_t motion_scheduler[7];
    unsigned char osd_enable;
    unsigned char wdr_enable;
    unsigned char speaker_enable;
    unsigned char recorder_enable;
    unsigned char led_enable;
    unsigned char image_correct;
    char device_name[32];
    unsigned char smd_enable;
    unsigned char debug_level;
    unsigned char time_zone;
    unsigned char debug_switch;
    int debug[20];
} config_t;


typedef struct devinfo_t
{
	unsigned int iFlag;
	char acProductKey[MAX_DEVINFO_SIZE];
	char acProductSecret[MAX_DEVINFO_SIZE];
	char acDeviceName[MAX_DEVINFO_SIZE];
	char acDeviceSecret[MAX_DEVINFO_SIZE];
}devinfo_t;
/*
 * function
 */
int config_get(config_t *rconfig);

int config_set(config_t *wconfig);
int config_devinfo_get(devinfo_t *pDevinfo);
int config_devinfo_set(devinfo_t *pDevinfo);
#endif
