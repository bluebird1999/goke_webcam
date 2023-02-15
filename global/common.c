/*
 * header
 */
//system header
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
//program header
#include "../common/tools_interface.h"
//server header
#include "../server/audio/audio_interface.h"
#include "../server/video/video_interface.h"
#include "../server/goke/goke_interface.h"
#include "../server/aliyun/aliyun_interface.h"
#include "../server/player/player_interface.h"
#include "../server/recorder/recorder_interface.h"
#include "../server/manager/manager_interface.h"
#include "../server/device/device_interface.h"
//
#include "global_interface.h"

/*
 * static
 */
//variable


//specific

/*
 * helper
 */
int global_common_send_message(int receiver, message_t *msg)
{
	int st = 0;
	switch(receiver) {
        case SERVER_DEVICE:
            st = server_device_message(msg);
            break;
        case SERVER_GOKE:
            st = server_goke_message(msg);
            break;
        case SERVER_ALIYUN:
            st = server_aliyun_message(msg);
            break;
        case SERVER_VIDEO:
            st = server_video_message(msg);
            break;
        case SERVER_AUDIO:
            st = server_audio_message(msg);
            break;
        case SERVER_RECORDER:
            st = server_recorder_message(msg);
            break;
        case SERVER_PLAYER:
            st = server_player_message(msg);
            break;
        case SERVER_MANAGER:
            st = manager_message(msg);
            break;
		default:
			log_goke(DEBUG_SERIOUS, "unknown message target! %d FROM msg id %d", receiver, msg->message);
			break;
	}
	return st;
}

void global_common_send_dummy(int server)
{
	message_t msg;
	msg_init(&msg);
	msg.sender = msg.receiver = server;
	msg.message = MSG_MANAGER_DUMMY;
	global_common_send_message(server,&msg);
}

void global_common_message_do_delivery(int receiver, message_t *msg)
{
	int num = 0;
	int ret = 0;
	ret = global_common_send_message(receiver, &msg);
	while( (ret!=0) && (num<MESSAGE_RESENT) ) {
		ret = global_common_send_message(receiver, &msg);
		num++;
		sleep(MESSAGE_RESENT_SLEEP);
	};
}

const char *global_common_message_to_string(message_type_t type)
{
    switch (type) {
        //manager
        ENUM_TYPE(MSG_MANAGER_SIGINT)
        ENUM_TYPE(MSG_MANAGER_EXIT)
        ENUM_TYPE(MSG_MANAGER_DUMMY)
        ENUM_TYPE(MSG_MANAGER_WAKEUP)
        ENUM_TYPE(MSG_MANAGER_HEARTBEAT)
        ENUM_TYPE(MSG_MANAGER_TIMER_ADD)
        ENUM_TYPE(MSG_MANAGER_TIMER_REMOVE)
        ENUM_TYPE(MSG_MANAGER_TIMER_ON)
        ENUM_TYPE(MSG_MANAGER_PROPERTY_GET)
        ENUM_TYPE(MSG_MANAGER_PROPERTY_SET)
        ENUM_TYPE(MSG_MANAGER_SIGINT_ACK)
        ENUM_TYPE(MSG_MANAGER_EXIT_ACK)
        ENUM_TYPE(MSG_MANAGER_DUMMY_ACK)
        ENUM_TYPE(MSG_MANAGER_WAKEUP_ACK)
        ENUM_TYPE(MSG_MANAGER_HEARTBEAT_ACK)
        ENUM_TYPE(MSG_MANAGER_TIMER_ADD_ACK)
        ENUM_TYPE(MSG_MANAGER_TIMER_REMOVE_ACK)
        ENUM_TYPE(MSG_MANAGER_TIMER_ON_ACK)
        ENUM_TYPE(MSG_MANAGER_PROPERTY_GET_ACK)
        ENUM_TYPE(MSG_MANAGER_PROPERTY_SET_ACK)
        //device
        ENUM_TYPE(MSG_DEVICE_SIGINT)
        ENUM_TYPE(MSG_DEVICE_GET_PARA)
        ENUM_TYPE(MSG_DEVICE_ACTION)
        ENUM_TYPE(MSG_DEVICE_CTRL_DIRECT)
        ENUM_TYPE(MSG_DEVICE_SD_FORMAT_START)
        ENUM_TYPE(MSG_DEVICE_SD_FORMAT_STOP)
        ENUM_TYPE(MSG_DEVICE_SIGINT_ACK)
        ENUM_TYPE(MSG_DEVICE_GET_PARA_ACK)
        ENUM_TYPE(MSG_DEVICE_ACTION_ACK)
        ENUM_TYPE(MSG_DEVICE_CTRL_DIRECT_ACK)
        ENUM_TYPE(MSG_DEVICE_SD_FORMAT_START_ACK)
        ENUM_TYPE(MSG_DEVICE_SD_FORMAT_STOP_ACK)
        //goke
        ENUM_TYPE(MSG_GOKE_SIGINT)
        ENUM_TYPE(MSG_GOKE_PROPERTY_GET)
        ENUM_TYPE(MSG_GOKE_PROPERTY_NOTIFY)
        ENUM_TYPE(MSG_GOKE_SIGINT_ACK)
        ENUM_TYPE(MSG_GOKE_PROPERTY_GET_ACK)
        ENUM_TYPE(MSG_GOKE_PROPERTY_NOTIFY_ACK)
        //video
        ENUM_TYPE(MSG_VIDEO_SIGINT)
        ENUM_TYPE(MSG_VIDEO_START)
        ENUM_TYPE(MSG_VIDEO_STOP)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_SET)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_SET_EXT)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_SET_DIRECT)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_GET)
        ENUM_TYPE(MSG_VIDEO_SNAPSHOT_THUMB)
        ENUM_TYPE(MSG_VIDEO_SPD_VIDEO_DATA)
        ENUM_TYPE(MSG_VIDEO_KEY_FRAME_REQUIRE)
        ENUM_TYPE(MSG_VIDEO_SIGINT_ACK)
        ENUM_TYPE(MSG_VIDEO_START_ACK)
        ENUM_TYPE(MSG_VIDEO_STOP_ACK)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_SET_ACK)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_SET_EXT_ACK)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_SET_DIRECT_ACK)
        ENUM_TYPE(MSG_VIDEO_PROPERTY_GET_ACK)
        ENUM_TYPE(MSG_VIDEO_SNAPSHOT_THUMB_ACK)
        ENUM_TYPE(MSG_VIDEO_SPD_VIDEO_DATA_ACK)
        ENUM_TYPE(MSG_VIDEO_KEY_FRAME_REQUIRE_ACK)
        //audio
        ENUM_TYPE(MSG_AUDIO_SIGINT)
        ENUM_TYPE(MSG_AUDIO_START)
        ENUM_TYPE(MSG_AUDIO_STOP)
        ENUM_TYPE(MSG_AUDIO_PROPERTY_GET)
        ENUM_TYPE(MSG_AUDIO_PROPERTY_SET)
        ENUM_TYPE(MSG_AUDIO_SPEAKER_START)
        ENUM_TYPE(MSG_AUDIO_SPEAKER_STOP)
        ENUM_TYPE(MSG_AUDIO_SPEAKER_DATA)
        ENUM_TYPE(MSG_AUDIO_PLAY_SOUND)
        ENUM_TYPE(MSG_AUDIO_SIGINT_ACK)
        ENUM_TYPE(MSG_AUDIO_START_ACK)
        ENUM_TYPE(MSG_AUDIO_STOP_ACK)
        ENUM_TYPE(MSG_AUDIO_PROPERTY_GET_ACK)
        ENUM_TYPE(MSG_AUDIO_PROPERTY_SET_ACK)
        ENUM_TYPE(MSG_AUDIO_SPEAKER_START_ACK)
        ENUM_TYPE(MSG_AUDIO_SPEAKER_STOP_ACK)
        ENUM_TYPE(MSG_AUDIO_SPEAKER_DATA_ACK)
        ENUM_TYPE(MSG_AUDIO_PLAY_SOUND_ACK)
        //recorder
        ENUM_TYPE(MSG_RECORDER_SIGINT)
        ENUM_TYPE(MSG_RECORDER_START)
        ENUM_TYPE(MSG_RECORDER_STOP)
        ENUM_TYPE(MSG_RECORDER_PROPERTY_GET)
        ENUM_TYPE(MSG_RECORDER_PROPERTY_SET)
        ENUM_TYPE(MSG_RECORDER_PROPERTY_NOTIFY)
        ENUM_TYPE(MSG_RECORDER_ADD)
        ENUM_TYPE(MSG_RECORDER_VIDEO_DATA)
        ENUM_TYPE(MSG_RECORDER_AUDIO_DATA)
        ENUM_TYPE(MSG_RECORDER_ADD_FILE)
        ENUM_TYPE(MSG_RECORDER_CLEAN_DISK_START)
        ENUM_TYPE(MSG_RECORDER_CLEAN_DISK_STOP)
        ENUM_TYPE(MSG_RECORDER_SIGINT_ACK)
        ENUM_TYPE(MSG_RECORDER_START_ACK)
        ENUM_TYPE(MSG_RECORDER_STOP_ACK)
        ENUM_TYPE(MSG_RECORDER_PROPERTY_GET_ACK)
        ENUM_TYPE(MSG_RECORDER_PROPERTY_SET_ACK)
        ENUM_TYPE(MSG_RECORDER_PROPERTY_NOTIFY_ACK)
        ENUM_TYPE(MSG_RECORDER_ADD_ACK)
        ENUM_TYPE(MSG_RECORDER_VIDEO_DATA_ACK)
        ENUM_TYPE(MSG_RECORDER_AUDIO_DATA_ACK)
        ENUM_TYPE(MSG_RECORDER_ADD_FILE_ACK)
        ENUM_TYPE(MSG_RECORDER_CLEAN_DISK_START_ACK)
        ENUM_TYPE(MSG_RECORDER_CLEAN_DISK_STOP_ACK)
        //player
        ENUM_TYPE(MSG_PLAYER_SIGINT)
        ENUM_TYPE(MSG_PLAYER_START)
        ENUM_TYPE(MSG_PLAYER_STOP)
        ENUM_TYPE(MSG_PLAYER_PAUSE)
        ENUM_TYPE(MSG_PLAYER_UNPAUSE)
        ENUM_TYPE(MSG_PLAYER_SET_PARAM)
        ENUM_TYPE(MSG_PLAYER_GET_FILE_BY_DAY)
        ENUM_TYPE(MSG_PLAYER_GET_FILE_BY_MONTH)
        ENUM_TYPE(MSG_PLAYER_PROPERTY_SET)
        ENUM_TYPE(MSG_PLAYER_AUDIO_START)
        ENUM_TYPE(MSG_PLAYER_AUDIO_STOP)
        ENUM_TYPE(MSG_PLAYER_REQUEST)
        ENUM_TYPE(MSG_PLAYER_RELAY)
        ENUM_TYPE(MSG_PLAYER_FINISH)
        ENUM_TYPE(MSG_PLAYER_GET_PICTURE_LIST)
        ENUM_TYPE(MSG_PLAYER_GET_INFOMATION)
        ENUM_TYPE(MSG_PLAYER_SEEK)
        ENUM_TYPE(MSG_PLAYER_REQUEST_SYNC)
        ENUM_TYPE(MSG_PLAYER_SIGINT_ACK)
        ENUM_TYPE(MSG_PLAYER_START_ACK)
        ENUM_TYPE(MSG_PLAYER_STOP_ACK)
        ENUM_TYPE(MSG_PLAYER_PAUSE_ACK)
        ENUM_TYPE(MSG_PLAYER_UNPAUSE_ACK)
        ENUM_TYPE(MSG_PLAYER_SET_PARAM_ACK)
        ENUM_TYPE(MSG_PLAYER_GET_FILE_BY_DAY_ACK)
        ENUM_TYPE(MSG_PLAYER_GET_FILE_BY_MONTH_ACK)
        ENUM_TYPE(MSG_PLAYER_PROPERTY_SET_ACK)
        ENUM_TYPE(MSG_PLAYER_AUDIO_START_ACK)
        ENUM_TYPE(MSG_PLAYER_AUDIO_STOP_ACK)
        ENUM_TYPE(MSG_PLAYER_REQUEST_ACK)
        ENUM_TYPE(MSG_PLAYER_RELAY_ACK)
        ENUM_TYPE(MSG_PLAYER_FINISH_ACK)
        ENUM_TYPE(MSG_PLAYER_GET_PICTURE_LIST_ACK)
        ENUM_TYPE(MSG_PLAYER_GET_INFOMATION_ACK)
        ENUM_TYPE(MSG_PLAYER_SEEK_ACK)
        ENUM_TYPE(MSG_PLAYER_REQUEST_SYNC_ACK)
        //aliyun
        ENUM_TYPE(MSG_ALIYUN_SIGINT)
        ENUM_TYPE(MSG_ALIYUN_QRCODE_RESULT)
        ENUM_TYPE(MSG_ALIYUN_CONNECT)
        ENUM_TYPE(MSG_ALIYUN_TIME_SYNC)
        ENUM_TYPE(MSG_ALIYUN_VIDEO_DATA)
        ENUM_TYPE(MSG_ALIYUN_AUDIO_DATA)
        ENUM_TYPE(MSG_ALIYUN_YIELD)
        ENUM_TYPE(MSG_ALIYUN_TIME_SYNCHRONIZED)
        ENUM_TYPE(MSG_ALIYUN_EVENT)
        ENUM_TYPE(MSG_ALIYUN_SETUP)
        ENUM_TYPE(MSG_ALIYUN_RESTART)
        ENUM_TYPE(MSG_ALIYUN_GET_TIME_SYNC)
        ENUM_TYPE(MSG_ALIYUN_SIGINT_ACK)
        ENUM_TYPE(MSG_ALIYUN_QRCODE_RESULT_ACK)
        ENUM_TYPE(MSG_ALIYUN_CONNECT_ACK)
        ENUM_TYPE(MSG_ALIYUN_TIME_SYNC_ACK)
        ENUM_TYPE(MSG_ALIYUN_VIDEO_DATA_ACK)
        ENUM_TYPE(MSG_ALIYUN_AUDIO_DATA_ACK)
        ENUM_TYPE(MSG_ALIYUN_YIELD_ACK)
        ENUM_TYPE(MSG_ALIYUN_TIME_SYNCHRONIZED_ACK)
        ENUM_TYPE(MSG_ALIYUN_EVENT_ACK)
        ENUM_TYPE(MSG_ALIYUN_SETUP_ACK)
        ENUM_TYPE(MSG_ALIYUN_RESTART_ACK)
        ENUM_TYPE(MSG_ALIYUN_GET_TIME_SYNC_ACK)
    }
    return "Unkown message";
}

const char *global_common_get_server_name(server_type_t type) {
    switch (type) {
        ENUM_TYPE(SERVER_MANAGER)
        ENUM_TYPE(SERVER_DEVICE)
        ENUM_TYPE(SERVER_GOKE)
        ENUM_TYPE(SERVER_VIDEO)
        ENUM_TYPE(SERVER_AUDIO)
        ENUM_TYPE(SERVER_RECORDER)
        ENUM_TYPE(SERVER_PLAYER)
        ENUM_TYPE(SERVER_ALIYUN)
    }
    return "Unkown server!";
}

const char *global_common_get_server_names(int servers) {
    char *server_str = 0;
    server_str = calloc(128,1);
    if( server_str == 0) {
        printf("memory allocation error in get server names!\n");
        return 0;
    }
    for( int i=0; i<SERVER_BUTT; i++ ) {
        if(misc_get_bit(servers, i)) {
            strcat( server_str, global_common_get_server_name(i) );
            strcat( server_str, ",");
        }
    }
    if(strlen(server_str)==0) {
        strcat( server_str, "-");
    }
    else {
        server_str[strlen(server_str)-1] = 0;
    }
    return server_str;
}

int server_set_status(server_info_t *info, pthread_rwlock_t *lock,
                             server_info_tag_e tag, int value) {
    int ret = -1;
    ret = pthread_rwlock_wrlock(lock);
    if(ret) {
        printf("add lock fail, ret = %d", ret);
        return ret;
    }
    switch(tag) {
        case INFO_TAG_STATUS:
            info->status = value;
            break;
        case INFO_TAG_INIT:
            info->init = value;
            break;
        case INFO_TAG_ERROR:
            info->error = value;
            break;
        case INFO_TAG_MSG_LOCK:
            info->msg_lock = value;
            break;
        case INFO_TAG_STATUS2:
            info->status2 = value;
            break;
        case INFO_TAG_EXIT:
            info->exit = value;
            break;
        case INFO_TAG_INIT_STATUS:
            info->init_status = value;
            break;
        case INFO_TAG_STATUS_OLD:
            info->old_status = value;
            break;
        case INFO_TAG_THREAD_EXIT:
            info->thread_exit = value;
            break;
        case INFO_TAG_THREAD_START:
            info->thread_start = value;
            break;
        default:
            printf("unkonw info tag = %d", tag);
            break;
    }
    ret = pthread_rwlock_unlock(lock);
    if (ret) {
        printf("add unlock fail, ret = %d", ret);
    }
    return ret;
}

int server_get_status(server_info_t *info, pthread_rwlock_t *lock,
                      server_info_tag_e tag) {
    int value;
    int ret;
    ret = pthread_rwlock_rdlock(lock);
    if(ret) {
        printf("add lock fail, ret = %d", ret);
        return ret;
    }
    switch(tag) {
        case INFO_TAG_STATUS:
            value = info->status;
        case INFO_TAG_INIT:
            value = info->init;
        case INFO_TAG_ERROR:
            value = info->error;
        case INFO_TAG_MSG_LOCK:
            value = info->msg_lock;
        case INFO_TAG_STATUS2:
            value = info->status2;
        case INFO_TAG_EXIT:
            value = info->exit;
        case INFO_TAG_INIT_STATUS:
            value = info->init_status;
        case INFO_TAG_STATUS_OLD:
            value = info->old_status;
        case INFO_TAG_THREAD_EXIT:
            value = info->thread_exit;
        case INFO_TAG_THREAD_START:
            value = info->thread_start;
        default:
            printf("unkonw info tag = %d", tag);
            value = -1;
            break;
    }
    ret = pthread_rwlock_unlock(lock);
    if( ret ) {
        printf("add unlock fail ret = %d",ret);
        return ret;
    }
    return value;
}