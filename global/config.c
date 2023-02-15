/*
 * header
 */
//system header
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/fs.h>
#include <mtd/mtd-user.h>

//program header
#include "../common/tools_interface.h"
//server header
#include "config.h"
#define DEVINFO_MTD_PART	"/dev/mtd7"
#include "global_interface.h"

/*
 * static
 */
//variable
static pthread_rwlock_t ilock = PTHREAD_RWLOCK_INITIALIZER;
static config_t config;
static config_map_t config_profile_map[] = {
        {"video_quality",               &(config.video_quality),                cfg_u8, "2",  0, 2,},
        {"subvideo_quality",            &(config.subvideo_quality),             cfg_u8, "0",  0, 2,},
        {"alarm_switch",                &(config.alarm_switch),                 cfg_u8, "1",  0, 1,},
        {"motion_detect_sensitivity",   &(config.motion_detect_sensitivity),    cfg_u8, "1",  0, 5,},
        {"alarm_freqency",              &(config.alarm_freqency),               cfg_u8, "1",  0, 2,},
        {"video_mirror",                &(config.video_mirror),                   cfg_u8, "0",  0, 1,},
        {"ipc_version",                 &(config.ipc_version),                  cfg_string, APPLICATION_VERSION_STRING,  0, 32,},
        {"day_night_mode",              &(config.day_night_mode),               cfg_u8, "0",  0, 2,},
        {"motion_scheduler_enable",     &(config.motion_scheduler_enable),               cfg_u8, "0",  0, 1,},
        {"motion_scheduler_0_begin",    &(config.motion_scheduler[0].begin), cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_0_end",      &(config.motion_scheduler[0].end),   cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_1_begin",    &(config.motion_scheduler[1].begin), cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_1_end",      &(config.motion_scheduler[1].end),   cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_2_begin",    &(config.motion_scheduler[2].begin), cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_2_end",      &(config.motion_scheduler[2].end),   cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_3_begin",    &(config.motion_scheduler[3].begin), cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_3_end",      &(config.motion_scheduler[3].end),   cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_4_begin",    &(config.motion_scheduler[4].begin), cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_4_end",      &(config.motion_scheduler[4].end),   cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_5_begin",    &(config.motion_scheduler[5].begin), cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_5_end",      &(config.motion_scheduler[5].end),   cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_6_begin",    &(config.motion_scheduler[6].begin), cfg_u32, "0",  0, 86400,},
        {"motion_scheduler_6_end",      &(config.motion_scheduler[6].end),   cfg_u32, "0",  0, 86400,},
        {"osd_enable",                  &(config.osd_enable),                cfg_u8, "1",  0, 1,},
        {"wdr_enable",                  &(config.wdr_enable),                cfg_u8, "0",  0, 1,},
        {"speaker_enable",              &(config.speaker_enable),                cfg_u8, "1",  0, 1,},
        {"recorder_enable",             &(config.recorder_enable),           cfg_u8, "1",  0, 1,},
        {"led_enable",                  &(config.led_enable),           cfg_u8, "1",  0, 1,},
        {"image_correct",               &(config.image_correct),                cfg_u8, "1",  0, 1,},
        {"device_name",                 &(config.device_name),                cfg_string, "",  0, 32,},
        {"debug_level",                 &(config.debug_level),                  cfg_u8,"3",0,8},
        {NULL,},
};

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

//function
static int config_save_proc(void);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * interface
 */
static int config_save_proc(void) {
    int ret = 0;
    message_t msg;

    return ret;
}

/*
 * it's safe here to not using any lock as read and write never conflict: read only
 * happened at start.
 */
int config_get(config_t *rconfig) {
    int ret = 0;
    char fname[MAX_SYSTEM_STRING_SIZE * 2];
    memset(fname, 0, sizeof(fname));
    sprintf(fname, "%s", CONFIG_PATH);
    ret = read_config_file(&config_profile_map, fname);
    memcpy(rconfig, &config, sizeof(config_t));
    return ret;
}

/*
 * write lock is necessary to avoid multiple write from threads
 */
int config_set(config_t *wconfig) {
    int ret = 0;
    char fname[MAX_SYSTEM_STRING_SIZE];
    pthread_rwlock_wrlock(&ilock);
    memcpy((config_t *) (&config), wconfig, sizeof(config_t));
    memset(fname, 0, sizeof(fname));
    sprintf(fname, "%s", CONFIG_PATH);
    ret = write_config_file(&config_profile_map, fname);
    pthread_rwlock_unlock(&ilock);
    return ret;
}

int config_devinfo_get(devinfo_t *pDevinfo)
{
	#define MTD_PAGE_SIZE		64*1024
	int iRet = -1,iMtdFd = -1;
	char acBuf[MTD_PAGE_SIZE];
	mtd_info_t stMtdInfo;
	memset(acBuf,0,sizeof(acBuf));
	iMtdFd = open(DEVINFO_MTD_PART, O_RDWR);
	if(iMtdFd > 0)
	{
		ioctl(iMtdFd, MEMGETINFO, &stMtdInfo);   // get the device info
		if(stMtdInfo.type != 3)
		{
			printf("unExist %s \r\n",DEVINFO_MTD_PART);
			close(iMtdFd);
			return -1;
		}
		lseek(iMtdFd,0,SEEK_SET);
		iRet = read(iMtdFd,acBuf,MTD_PAGE_SIZE);
		close(iMtdFd);
		if(iRet <= 0)
		{
			return -1;
		}
		iRet = 0;
		memcpy(pDevinfo,acBuf,sizeof(devinfo_t));
	}
    
	return iRet;
}

int config_devinfo_set(devinfo_t *pDevinfo)
{
	#define MTD_PAGE_SIZE		64*1024
	int iRet = -1,iMtdFd = -1,i = 0,erase_count = 0;
	char acBuf[MTD_PAGE_SIZE];
	mtd_info_t stMtdInfo;
	struct erase_info_user e;
	memset(acBuf,0,sizeof(acBuf));
	iMtdFd = open(DEVINFO_MTD_PART, O_RDWR);
	if(iMtdFd > 0)
	{
		ioctl(iMtdFd, MEMGETINFO, &stMtdInfo);   // get the device info
		if(stMtdInfo.type != 3)
		{
			printf("unExist %s \r\n",DEVINFO_MTD_PART);
			close(iMtdFd);
			return -1;
		}
		
		e.length = stMtdInfo.erasesize;
		e.start = 0;
		erase_count = (stMtdInfo.size + stMtdInfo.erasesize - 1) / stMtdInfo.erasesize;
		for (i = 1; i <= erase_count; i++)
		{
			iRet = ioctl(iMtdFd, MEMERASE, &e);
			if(iRet != 0)
			{
				close(iMtdFd);
				return -1;
			}
			e.start += stMtdInfo.erasesize;
		}
		lseek(iMtdFd,0,SEEK_SET);
		memcpy(acBuf,pDevinfo,sizeof(devinfo_t));
		iRet = write(iMtdFd,acBuf,MTD_PAGE_SIZE);
		close(iMtdFd);
		if(iRet <= 0)
		{
			return -1;
		}
		iRet = 0;
		
	}
	return iRet;
}
