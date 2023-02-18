/*
* @Author: Marte
* @Date:   2020-09-24 10:56:09
* @Last Modified by:   Marte
* @Last Modified time: 2020-09-24 18:18:43
*/
#include <errno.h>
#include <stdio.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include "sd_control.h"
#include "device_interface.h"
#include "../../server/aliyun/linkkit_client.h"
#include "file_manager.h"

static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
static int sd_card_status = E_SDCARD_STATUS_NULL;
static int sd_card_last_status = -1;
static int sleep_time = 100000;
static int file_init = -1;
static int size_check_tag = 0;
static int format_status = 0;
static int init_wait = 0;
#ifdef RELEASE_VERSION
static char sd_address[32] = "";
#else
static char sd_address[32] = "e624";
#endif

static int exec_t(char *cmd) {
    int ret;
    char buff[SIZE] = {0};
    FILE *fstream = NULL;

    if (cmd == NULL) {
        return -1;
    }

    if (NULL == (fstream = popen(cmd, "r"))) {
        log_goke(DEBUG_WARNING, "execute command failed, cmd = %s\n", cmd);
        return -1;
    }

    //clear fb cache
    while (NULL != fgets(buff, sizeof(buff), fstream));

    ret = pclose(fstream);
    return 0;
}


//*获取sd挂载状态*//
static int sd_is_mount(char *pMountPath) {
    FILE *fp = NULL;
    char *pLine = NULL;
    char acLine[512];

    if (NULL == pMountPath) {
        return 0;
    }

    fp = fopen(MOUNT_PROC_PATH, "r");
    if (NULL == fp) {
        return 0;
    }

    while (1) {
        pLine = fgets(acLine, sizeof(acLine), fp);
        if (NULL != pLine) {
            if (strstr(pLine, pMountPath)) {
                //printf("pLine:%s\n",pLine);
                fclose(fp);
                return 1;
            }
        } else {
            break;
        }
    }
    fclose(fp);
    return 0;
}

static int sd_is_plugin(void) {
    int ret;
    char sd_plug_path[MAX_SYSTEM_STRING_SIZE];
    FILE *fp = NULL;
    char *pLine = NULL;
    char acLine[512];
    char *pos = NULL;
    //
    if( strlen(sd_address) == 0 ) { //to search in the linux start message
//        fp = fopen(KMSG_PATH, "r");
        fp = popen("dmesg","r");
        if (NULL == fp) {
            return -1;
        }
        while(1) {
            pLine = fgets(acLine, sizeof(acLine), fp);
            if (NULL != pLine) {
                pos = strstr(pLine, MMC0_PLUG_KMSG);
                if (pos) {
                    log_goke(DEBUG_INFO, "pLine:%s", pLine);
                    memset(sd_address, 0, sizeof(sd_address));
                    memcpy(sd_address, pos + strlen(MMC0_PLUG_KMSG), strlen(pLine) - strlen(MMC0_PLUG_KMSG) - 1);
                    log_goke(DEBUG_INFO, "found sd mount addres = %s", sd_address);
                }
            } else {
                break;
            }
        }
        fclose(fp);
    }
    memset(sd_plug_path,0,sizeof(sd_plug_path));
    sprintf(sd_plug_path, "%s%s/cid",SD_PLUG_PATH, sd_address);
    ret = access(sd_plug_path, F_OK);
    return ret;
}

static int sd_mount() {
    int ret = -1;
    char cmd[128];
    if (access(SD_MOUNT_NODEP1, F_OK) == 0) {
        if (0 == mount(SD_MOUNT_NODEP1, SD_MOUNT_PATH, "vfat", MS_SYNCHRONOUS, "")) {
            ret = 0;
        }
    } else if (access(SD_MOUNT_NODE, F_OK) == 0) {
        if (0 == mount(SD_MOUNT_NODE, SD_MOUNT_PATH, "vfat", MS_SYNCHRONOUS, "")) {
            ret = 0;
        }
    }
    if( ret == -1) {
        memset(cmd, 0, sizeof(cmd));
        sprintf(cmd, "mount -t vfat %s %s", SD_MOUNT_NODEP1, SD_MOUNT_PATH);
        ret = system(cmd);
    }
    return ret;
}

/*
 * 
 */
int sd_get_info(sd_info_t *pInfo) {
    struct statfs statFS;
    unsigned long long llTotalBytes = 0;
    unsigned long long llFreeBytes = 0;
    if (sd_card_status != E_SDCARD_STATUS_MOUNT) return -1;
    if (pInfo == NULL) {
        return -1;
    }

    if (statfs(SD_MOUNT_PATH, &statFS) == -1) {
        log_goke(DEBUG_WARNING, "statfs failed for path->[%s]\n", SD_MOUNT_PATH);
        return -1;
    }

    llTotalBytes = (unsigned long long) statFS.f_blocks * (unsigned long long) statFS.f_bsize / 1024;
    llFreeBytes = (unsigned long long) statFS.f_bfree * (unsigned long long) statFS.f_bsize / 1024;

    pInfo->totalBytes = (unsigned int) llTotalBytes;
    pInfo->freeBytes = (unsigned int) llFreeBytes;
    pInfo->usedBytes = pInfo->totalBytes - pInfo->freeBytes;

    return 0;
}

int sd_get_file_status() {
    return file_init;
}

int sd_get_status() {
    SD_ALIYUN_STATUS st;
    switch (sd_card_status) {
        case E_SDCARD_STATUS_NULL:
        case E_SDCARD_STATUS_PLUGIN:
        case E_SDCARD_STATUS_UNMOUNT:
        case E_SDCARD_STATUS_UNMOUNTING:
        case E_SDCARD_STATUS_UNMOUNT_SUCCESS:
            st = SD_ALIYUN_STATUS_UNPLUG;
            break;
        case E_SDCARD_STATUS_FORMAT:
        case E_SDCARD_STATUS_FORMATING:
        case E_SDCARD_STATUS_FORMAT_SUCCESS:
            st = SD_ALIYUN_STATUS_FORMATING;
            break;
        case E_SDCARD_STATUS_MOUNT:
            st = SD_ALIYUN_STATUS_NORMAL;
            break;
        case E_SDCARD_STATUS_MOUNT_FAIL:
            st = SD_ALIYUN_STATUS_UNPLUG;
            break;
    }
    return st;
}

int sd_format() {
    if (sd_card_status == E_SDCARD_STATUS_NULL)  {
        return -1;
    }
    sd_card_status = E_SDCARD_STATUS_FORMAT;
    message_t msg;
    msg.message = MSG_DEVICE_SD_FORMAT_START;
    msg.sender = msg.receiver = SERVER_ALIYUN;
    global_common_send_message( SERVER_RECORDER,&msg);
    global_common_send_message( SERVER_PLAYER,&msg);
    return 0;
}

int sd_unmount() {
    if (sd_card_status != E_SDCARD_STATUS_MOUNT) {
        return -1;
    }
    sd_card_status = E_SDCARD_STATUS_UNMOUNT;
    message_t msg;
    msg.message = MSG_DEVICE_SD_FORMAT_START;
    msg.sender = msg.receiver = SERVER_ALIYUN;
    global_common_send_message( SERVER_RECORDER,&msg);
    global_common_send_message( SERVER_PLAYER,&msg);
    return 0;
}

int sd_set_response( sd_action_t action, server_type_t server) {
    pthread_rwlock_wrlock(&lock);
    if( (sd_card_status == E_SDCARD_STATUS_FORMAT) || (sd_card_status == E_SDCARD_STATUS_UNMOUNT) ) {
        if (action == E_SD_ACTION_FORMAT) {
            misc_set_bit(&format_status, server);
            log_goke(DEBUG_INFO, "sd format waiting status was updated by server %d, status=%d", server, format_status);
        }
    }
    pthread_rwlock_unlock(&lock);
    return 0;
}

int update_hotplug_event(int type, char *addr) {
    if( type ) {
        init_wait = 0;
        memset(sd_address, 0, sizeof(sd_address));
        strcpy(sd_address, addr);
    } else {
        memset(sd_address, 0, sizeof(sd_address));
        init_wait = 1;
    }
    return 0;
}

int sd_proc(pthread_rwlock_t *lock) {
    int ret = 0;
    char cmd[256];
    char *pSdCardNode = NULL;
    sd_info_t stInfo;
    char *block_path;
    char *mountpath;

    switch (sd_card_status) {
        case E_SDCARD_STATUS_NULL: { //无需操作
            if( !init_wait ) {  // only check once the program start to save resource untill hotplug event happened!
                ret = sd_is_plugin();
                if (ret == 0) {
                    sd_card_status = E_SDCARD_STATUS_PLUGIN;
                } else {
                    init_wait = 1;
                }
            }
        }
        break;
        case E_SDCARD_STATUS_PLUGIN: { //插入
            if (!sd_is_mount(SD_MOUNT_PATH)) { //sdcard hasn't mount here.
                ret = sd_mount();
                if(!ret) {
                    sd_card_status = E_SDCARD_STATUS_MOUNT;
                    sleep_time = 200000;
                    log_goke(DEBUG_WARNING, "mount sdcard success");
                } else {
                    log_goke(DEBUG_WARNING, "mount sdcard failed!");
                }
            } else {
                sd_card_status = E_SDCARD_STATUS_MOUNT;
                sleep_time = 200000;
                log_goke(DEBUG_WARNING, "sdcard is already mounted!");
            }
        }
        break;
        case E_SDCARD_STATUS_FORMAT: {
            if( misc_get_bit( format_status, SERVER_RECORDER) &&
                    misc_get_bit( format_status, SERVER_PLAYER) ) {
                sd_card_status = E_SDCARD_STATUS_FORMATING;
            }
        }
            break;
        case E_SDCARD_STATUS_FORMATING: //格式化中...
        {
            file_manager_set_reinit();
            format_status = 0;
            if (sd_is_mount(SD_MOUNT_PATH)) {//sdcard has mount,please umount
                memset(cmd, 0, sizeof(cmd));
                snprintf( cmd, sizeof(cmd), "umount %s", SD_MOUNT_PATH);
                ret = system(cmd);
                if (/*umount(SD_MOUNT_PATH) */ ret== 0) {
                    log_goke(DEBUG_INFO, "unmount sdcard success");
                } else {
                    log_goke(DEBUG_WARNING, "unmount sdcard failed!");
                    break;
                }
            }
            usleep(20*1000);
            memset(cmd, 0, sizeof(cmd));
            if (access(SD_MOUNT_NODEP1, F_OK) == 0) {
                pSdCardNode = SD_MOUNT_NODEP1;
            } else if (access(SD_MOUNT_NODE, F_OK) == 0) {
                pSdCardNode = SD_MOUNT_NODE;
            } else {
                pSdCardNode = NULL;
            }
            if (pSdCardNode) {
                snprintf(cmd, sizeof(cmd) - 1, "%s %s", VFAT_FORMAT_TOOL_PATH, pSdCardNode);
                log_goke( DEBUG_INFO, "format cmd = %s", cmd);
                if (exec_t(cmd) == 0) {
                    sd_card_status = E_SDCARD_STATUS_FORMAT_SUCCESS;
                    log_goke(DEBUG_INFO, "format sdcard success");
                } else {
                    log_goke(DEBUG_WARNING, "format sdcard failed!");
                }
            } else {
                sd_card_status = E_SDCARD_STATUS_NULL;
            }
            usleep(20*1000);
        }
            break;
        case E_SDCARD_STATUS_FORMAT_SUCCESS://格式化成功也要重新mount
        {//
            if ( !sd_is_mount(SD_MOUNT_PATH) ) {//
                ret = sd_mount();
                if( !ret ) {
                    sd_card_status = E_SDCARD_STATUS_MOUNT;
                    sleep_time = 200000;
                    log_goke(DEBUG_INFO, "after format, sdcard mount success");
                }
            } else {
                sd_card_status = E_SDCARD_STATUS_MOUNT;
                log_goke(DEBUG_INFO, "after format, sdcard mount success");
            }
        }
            break;
        case E_SDCARD_STATUS_MOUNT://插入并mount，可正常使用
        {
        }
            break;
        case E_SDCARD_STATUS_UNMOUNT: {
            if( misc_get_bit( format_status, SERVER_RECORDER) &&
                misc_get_bit( format_status, SERVER_PLAYER) ) {
                sd_card_status = E_SDCARD_STATUS_UNMOUNTING;
            }
        }
        break;
        case E_SDCARD_STATUS_UNMOUNTING:{ //推出TF卡，插入但没有mount
            file_manager_set_reinit();
            format_status = 0;
            if (sd_is_mount(SD_MOUNT_PATH)) {
                memset(cmd, 0, sizeof(cmd));
                snprintf( cmd, sizeof(cmd), "umount %s", SD_MOUNT_PATH);
                ret = system(cmd);
                if (/*umount(SD_MOUNT_PATH) */ ret== 0) {
                    log_goke(DEBUG_WARNING, "unmount sdcard success");
                    sd_card_status = E_SDCARD_STATUS_UNMOUNT_SUCCESS;
                } else {
                    log_goke(DEBUG_WARNING, "unmount sdcard failed!");
                }
            } else {
                log_goke(DEBUG_WARNING, "sd card is already unmounted!");
                sd_card_status = E_SDCARD_STATUS_UNMOUNT_SUCCESS;
            }
        }
            break;
        case E_SDCARD_STATUS_UNMOUNT_SUCCESS:{
            log_goke( DEBUG_INFO, "unmount sd success!, will change to unplug status!");
            sd_card_status = E_SDCARD_STATUS_NULL;
            init_wait = 1;  //automatically set wait untill a hotplug event happened!
        }
            break;
        case E_SDCARD_STATUS_MOUNT_FAIL://mount失败
        {
        }
            break;
        default:
            break;
    }
    if (sd_card_last_status != sd_card_status) {
        sd_card_last_status = sd_card_status;
        if (sd_card_status == E_SDCARD_STATUS_MOUNT) {
            ret = file_manager_read_file_list(manager_config.timezone);
            if (ret) {
                log_goke(DEBUG_SERIOUS, "file reading failed");
                file_init = 0;
            } else {
                file_init = 1;
                log_goke(DEBUG_SERIOUS, "file_manager_read_file_list ok");
            }
        }
        aliyun_report_sd_info();
    }

    size_check_tag += sleep_time / 1000;
    if( sd_card_status == E_SDCARD_STATUS_MOUNT) {
        if (size_check_tag > 20000) {//every 20s to check size
            if (sd_get_info(&stInfo) == 0) {
                log_goke(DEBUG_WARNING, "totalBytes:%d,freeBytes:%d", stInfo.totalBytes / 1024,
                         stInfo.freeBytes / 1024);
                if (stInfo.freeBytes < DEVICE_SD_CLEAN_SIZE) {
                    //clean sd
                    file_manager_clean_disk();
                    file_manager_read_file_list(manager_config.timezone);
                }
            } else {
                log_goke(DEBUG_WARNING, "{\"StorageStatus\":0}");
            }
            size_check_tag = 0;
            aliyun_report_sd_info();
        }
    }
    usleep(sleep_time);
    return 0;
}