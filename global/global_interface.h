#ifndef GLOBAL_GLOBAL_INTERFACE_H_
#define GLOBAL_GLOBAL_INTERFACE_H_

/*
 * header
 */
#include "../common/tools_interface.h"
#include "config.h"

/*
 * define
 */
#define				APPLICATION_VERSION_STRING	"alpha-4.0"

#define             RELEASE_VERSION              1

#define				MESSAGE_RESENT				100
#define				MESSAGE_RESENT_SLEEP		1000*200

#define             ENUM_TYPE(x)   case x: return(#x);
/*
 * structure
 */
typedef enum server_type_t {
    SERVER_MANAGER,
    SERVER_DEVICE,
    SERVER_GOKE,
    SERVER_VIDEO,
    SERVER_AUDIO,
    SERVER_RECORDER,
    SERVER_PLAYER,
    SERVER_ALIYUN,
    SERVER_BUTT,
} server_type_t;

//message
typedef enum message_type_t {
    //manager
    MSG_MANAGER_BASE = (SERVER_MANAGER << 16),
    //system
    MSG_MANAGER_SIGINT,
    MSG_MANAGER_EXIT,
    MSG_MANAGER_DUMMY,
    MSG_MANAGER_WAKEUP,
    MSG_MANAGER_HEARTBEAT,
    MSG_MANAGER_TIMER_ADD,
    MSG_MANAGER_TIMER_REMOVE,
    MSG_MANAGER_TIMER_ON,
    //none-system
    MSG_MANAGER_NONE_SYSTEM = ((SERVER_MANAGER << 16 ) | 0x0010),
    MSG_MANAGER_PROPERTY_GET,
    MSG_MANAGER_PROPERTY_SET,
    MSG_MANAGER_ACK_BASE = ((SERVER_MANAGER << 16) | 0x1000),
    MSG_MANAGER_SIGINT_ACK,
    MSG_MANAGER_EXIT_ACK,
    MSG_MANAGER_DUMMY_ACK,
    MSG_MANAGER_WAKEUP_ACK,
    MSG_MANAGER_HEARTBEAT_ACK,
    MSG_MANAGER_TIMER_ADD_ACK,
    MSG_MANAGER_TIMER_REMOVE_ACK,
    MSG_MANAGER_TIMER_ON_ACK,
    //none-system
    MSG_MANAGER_NONE_SYSTEM_ACK = ((SERVER_MANAGER << 16 ) | 0x1010),
    MSG_MANAGER_PROPERTY_GET_ACK,
    MSG_MANAGER_PROPERTY_SET_ACK,
    //device
    MSG_DEVICE_BASE = (SERVER_DEVICE<<16),
    MSG_DEVICE_SIGINT,
    MSG_DEVICE_GET_PARA,
    MSG_DEVICE_ACTION,
    MSG_DEVICE_CTRL_DIRECT,
    MSG_DEVICE_SD_FORMAT_START,
    MSG_DEVICE_SD_FORMAT_STOP,
    MSG_DEVICE_ACK_BASE = ((SERVER_DEVICE << 16) | 0x1000),
    MSG_DEVICE_SIGINT_ACK,
    MSG_DEVICE_GET_PARA_ACK,
    MSG_DEVICE_ACTION_ACK,
    MSG_DEVICE_CTRL_DIRECT_ACK,
    MSG_DEVICE_SD_FORMAT_START_ACK,
    MSG_DEVICE_SD_FORMAT_STOP_ACK,
    //goke
    MSG_GOKE_BASE = (SERVER_GOKE<<16),
    MSG_GOKE_SIGINT,
    MSG_GOKE_PROPERTY_GET,
    MSG_GOKE_PROPERTY_NOTIFY,
    MSG_GOKE_ACK_BASE = ((SERVER_GOKE << 16) | 0x1000),
    MSG_GOKE_SIGINT_ACK,
    MSG_GOKE_PROPERTY_GET_ACK,
    MSG_GOKE_PROPERTY_NOTIFY_ACK,
    //video
    MSG_VIDEO_BASE = (SERVER_VIDEO<<16),
    MSG_VIDEO_SIGINT,
    MSG_VIDEO_START,
    MSG_VIDEO_STOP,
    MSG_VIDEO_PROPERTY_SET,
    MSG_VIDEO_PROPERTY_SET_EXT,
    MSG_VIDEO_PROPERTY_SET_DIRECT,
    MSG_VIDEO_PROPERTY_GET,
    MSG_VIDEO_SNAPSHOT_THUMB,
    MSG_VIDEO_SPD_VIDEO_DATA,
    MSG_VIDEO_KEY_FRAME_REQUIRE,
    MSG_VIDEO_ACK_BASE = ((SERVER_VIDEO << 16) | 0x1000),
    MSG_VIDEO_SIGINT_ACK,
    MSG_VIDEO_START_ACK,
    MSG_VIDEO_STOP_ACK,
    MSG_VIDEO_PROPERTY_SET_ACK,
    MSG_VIDEO_PROPERTY_SET_EXT_ACK,
    MSG_VIDEO_PROPERTY_SET_DIRECT_ACK,
    MSG_VIDEO_PROPERTY_GET_ACK,
    MSG_VIDEO_SNAPSHOT_THUMB_ACK,
    MSG_VIDEO_SPD_VIDEO_DATA_ACK,
    MSG_VIDEO_KEY_FRAME_REQUIRE_ACK,
    //audio
    MSG_AUDIO_BASE = (SERVER_AUDIO<<16),
    MSG_AUDIO_SIGINT,
    MSG_AUDIO_START,
    MSG_AUDIO_STOP,
    MSG_AUDIO_PROPERTY_GET,
    MSG_AUDIO_PROPERTY_SET,
    MSG_AUDIO_SPEAKER_START,
    MSG_AUDIO_SPEAKER_STOP,
    MSG_AUDIO_SPEAKER_DATA,
    MSG_AUDIO_PLAY_SOUND,
    MSG_AUDIO_ACK_BASE = ((SERVER_AUDIO << 16) | 0x1000),
    MSG_AUDIO_SIGINT_ACK,
    MSG_AUDIO_START_ACK,
    MSG_AUDIO_STOP_ACK,
    MSG_AUDIO_PROPERTY_GET_ACK,
    MSG_AUDIO_PROPERTY_SET_ACK,
    MSG_AUDIO_SPEAKER_START_ACK,
    MSG_AUDIO_SPEAKER_STOP_ACK,
    MSG_AUDIO_SPEAKER_DATA_ACK,
    MSG_AUDIO_PLAY_SOUND_ACK,
    //recorder
    MSG_RECORDER_BASE = (SERVER_RECORDER<<16),
    MSG_RECORDER_SIGINT,
    MSG_RECORDER_START,
    MSG_RECORDER_STOP,
    MSG_RECORDER_PROPERTY_GET,
    MSG_RECORDER_PROPERTY_SET,
    MSG_RECORDER_PROPERTY_NOTIFY,
    MSG_RECORDER_ADD,
    MSG_RECORDER_VIDEO_DATA,
    MSG_RECORDER_AUDIO_DATA,
    MSG_RECORDER_ADD_FILE,
    MSG_RECORDER_CLEAN_DISK_START,
    MSG_RECORDER_CLEAN_DISK_STOP,
    MSG_RECORDER_ACK_BASE = ((SERVER_RECORDER << 16) | 0x1000),
    MSG_RECORDER_SIGINT_ACK,
    MSG_RECORDER_START_ACK,
    MSG_RECORDER_STOP_ACK,
    MSG_RECORDER_PROPERTY_GET_ACK,
    MSG_RECORDER_PROPERTY_SET_ACK,
    MSG_RECORDER_PROPERTY_NOTIFY_ACK,
    MSG_RECORDER_ADD_ACK,
    MSG_RECORDER_VIDEO_DATA_ACK,
    MSG_RECORDER_AUDIO_DATA_ACK,
    MSG_RECORDER_ADD_FILE_ACK,
    MSG_RECORDER_CLEAN_DISK_START_ACK,
    MSG_RECORDER_CLEAN_DISK_STOP_ACK,
    //player
    MSG_PLAYER_BASE = (SERVER_PLAYER<<16),
    MSG_PLAYER_SIGINT,
    MSG_PLAYER_START,
    MSG_PLAYER_STOP,
    MSG_PLAYER_PAUSE,
    MSG_PLAYER_UNPAUSE,
    MSG_PLAYER_SET_PARAM,
    MSG_PLAYER_GET_FILE_BY_DAY,
    MSG_PLAYER_GET_FILE_BY_MONTH,
    MSG_PLAYER_PROPERTY_SET,
    MSG_PLAYER_AUDIO_START,
    MSG_PLAYER_AUDIO_STOP,
    MSG_PLAYER_REQUEST,
    MSG_PLAYER_RELAY,
    MSG_PLAYER_FINISH,
    MSG_PLAYER_GET_PICTURE_LIST,
    MSG_PLAYER_GET_INFOMATION,
    MSG_PLAYER_SEEK,
    MSG_PLAYER_REQUEST_SYNC,
    MSG_PLAYER_ACK_BASE = ((SERVER_PLAYER << 16) | 0x1000),
    MSG_PLAYER_SIGINT_ACK,
    MSG_PLAYER_START_ACK,
    MSG_PLAYER_STOP_ACK,
    MSG_PLAYER_PAUSE_ACK,
    MSG_PLAYER_UNPAUSE_ACK,
    MSG_PLAYER_SET_PARAM_ACK,
    MSG_PLAYER_GET_FILE_BY_DAY_ACK,
    MSG_PLAYER_GET_FILE_BY_MONTH_ACK,
    MSG_PLAYER_PROPERTY_SET_ACK,
    MSG_PLAYER_AUDIO_START_ACK,
    MSG_PLAYER_AUDIO_STOP_ACK,
    MSG_PLAYER_REQUEST_ACK,
    MSG_PLAYER_RELAY_ACK,
    MSG_PLAYER_FINISH_ACK,
    MSG_PLAYER_GET_PICTURE_LIST_ACK,
    MSG_PLAYER_GET_INFOMATION_ACK,
    MSG_PLAYER_SEEK_ACK,
    MSG_PLAYER_REQUEST_SYNC_ACK,
    //aliyun
    MSG_ALIYUN_BASE = (SERVER_ALIYUN << 16),
    MSG_ALIYUN_SIGINT,
    MSG_ALIYUN_QRCODE_RESULT,
    MSG_ALIYUN_CONNECT,
    MSG_ALIYUN_TIME_SYNC,
    MSG_ALIYUN_VIDEO_DATA,
    MSG_ALIYUN_AUDIO_DATA,
    MSG_ALIYUN_YIELD,
    MSG_ALIYUN_TIME_SYNCHRONIZED,
    MSG_ALIYUN_EVENT,
    MSG_ALIYUN_SETUP,
    MSG_ALIYUN_RESTART,
    MSG_ALIYUN_GET_TIME_SYNC,
    MSG_ALIYUN_ACK_BASE = ((SERVER_ALIYUN << 16) | 0x1000),
    MSG_ALIYUN_SIGINT_ACK,
    MSG_ALIYUN_QRCODE_RESULT_ACK,
    MSG_ALIYUN_CONNECT_ACK,
    MSG_ALIYUN_TIME_SYNC_ACK,
    MSG_ALIYUN_VIDEO_DATA_ACK,
    MSG_ALIYUN_AUDIO_DATA_ACK,
    MSG_ALIYUN_YIELD_ACK,
    MSG_ALIYUN_TIME_SYNCHRONIZED_ACK,
    MSG_ALIYUN_EVENT_ACK,
    MSG_ALIYUN_SETUP_ACK,
    MSG_ALIYUN_RESTART_ACK,
    MSG_ALIYUN_GET_TIME_SYNC_ACK,
} message_type_t;

/*
 * function
 */
/*
 * global variables
 */
extern  config_t	_config_;
extern  long long   _universal_tag_;
extern  int         _reset_;

int global_common_send_message(int receiver, message_t *msg);
void global_common_send_dummy(int server);
void global_common_message_do_delivery(int receiver, message_t *msg);
const char *global_common_message_to_string(message_type_t type);
const char *global_common_get_server_name(server_type_t type);
const char *global_common_get_server_names(int servers);

#endif
