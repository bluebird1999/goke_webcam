#ifndef SERVER_DEVICE_SD_CONTRIL_H_
#define SERVER_DEVICE_SD_CONTRIL_H_

/*
 * header
 */
#include "device_interface.h"
/*
 * define
 */
#define SIZE 					128
#define SIZE1024 				1024
#define SD_MOUNTED_FLAG			"/mnt/.mounted"
#define SD_MOUNT_PATH 			"/mnt/sdcard"
#define SD_MOUNT_NODE 			"/dev/mmcblk0"
#define SD_MOUNT_NODEP1			"/dev/mmcblk0p1"
#define VFAT_FORMAT_TOOL_PATH   "/sbin/mkfs.vfat"
#define MOUNT_PROC_PATH 		"/proc/mounts"
#define SD_PLUG_PATH 			"/sys/devices/platform/soc/10010000.sdhci/mmc_host/mmc0/mmc0:"
#define SD_PLUG_ADD_CMD         "add@/devices/platform/soc/10010000.sdhci/mmc_host/mmc0/mmc0:"
#define SD_PLUG_REMOVE_CMD      "remove@/devices/platform/soc/10010000.sdhci/mmc_host/mmc0/mmc0:"
#define KMSG_PATH               "/proc/kmsg"

#define MMC0_PLUG_KMSG          "mmc0: new high speed SDHC card at address "

#define DEVICE_SD_CLEAN_SIZE    (128*1024)  //minimum 128M

#define FREE_T(x) {\
	free(x);\
	x=NULL;\
}


/*
 * structure
 */


/*
 * function
 */
typedef enum E_SDCARD_STATUS
{
	E_SDCARD_STATUS_NULL = 0,//无sd插入
	E_SDCARD_STATUS_PLUGIN,//插入
    E_SDCARD_STATUS_FORMAT,
	E_SDCARD_STATUS_FORMATING,//格式化中...
	E_SDCARD_STATUS_FORMAT_SUCCESS,//格式化成功
	E_SDCARD_STATUS_MOUNT,//插入并mount，可正常使用
	E_SDCARD_STATUS_UNMOUNT,//推出TF卡，插入但没有mount
    E_SDCARD_STATUS_UNMOUNTING,
    E_SDCARD_STATUS_UNMOUNT_SUCCESS,//推出TF卡，插入但没有mount
	E_SDCARD_STATUS_MOUNT_FAIL,//mount失败
    E_SDCARD_STATUS_BUTT,
} E_SDCARD_STATUS;

typedef enum SD_ALIYUN_STATUS {
    SD_ALIYUN_STATUS_UNPLUG = 0,
    SD_ALIYUN_STATUS_NORMAL,
    SD_ALIYUN_STATUS_UNFORMAT,
    SD_ALIYUN_STATUS_FORMATING,
    SD_ALIYUN_STTAUS_BUTT,
}SD_ALIYUN_STATUS;

//sd		------------------------------------------------------------
typedef struct sd_info_t {
    E_SDCARD_STATUS	    status;
    unsigned long	totalBytes;
    unsigned long	usedBytes;
    unsigned long	freeBytes;
} sd_info_t;

typedef enum sd_action_t {
    E_SD_ACTION_FORMAT = 0,
    E_SD_ACTION_UNMOUNT,
    E_SD_ACTION_BUTT,
} sd_action_t;

int sd_proc(pthread_rwlock_t *lock);
int sd_get_status();
int sd_format();
int sd_unmount();
int sd_get_info(sd_info_t *pInfo);
int sd_get_file_status();
int sd_set_response( sd_action_t, server_type_t );
int update_hotplug_event(int type, char*);
#endif
