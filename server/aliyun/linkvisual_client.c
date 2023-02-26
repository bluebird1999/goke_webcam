//system
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
//library
#include <iot_import.h>
#include <link_visual_api.h>
//server
#include "../video/video_interface.h"
#include "../audio/audio_interface.h"
#include "../recorder/recorder_interface.h"
#include "../player/player_interface.h"
#include "../device/file_manager.h"
//local
#include "linkkit_client.h"
#include "linkvisual_client.h"
#include "aliyun_interface.h"
#include "aliyun.h"
#include "config.h"

static  int                 g_init = 0;

/*
 *
 * helper
 *
 *
 */
static int start_push_streaming_cb(const lv_device_auth_s *auth, const lv_start_push_stream_param_s *param)
{
    int ret = -1;
    message_t msg;
    player_init_t player;

    log_goke(DEBUG_WARNING,"start_push_streaming_cb:%d %d", param->common.service_id, param->common.stream_cmd_type);

    switch(param->common.stream_cmd_type) {
        case LV_STREAM_CMD_LIVE:
#if 0
            //player init
            memset(&player, 0, sizeof(player_init_t));
            player.start = 1676889652;
            player.stop = 1677062452;
            player.offset = 0;
            player.speed = 1;
            player.channel_merge = 0;
            player.switch_to_live = 1;
            player.audio = 1;
            player.type = 1;
            player.channel = param->common.service_id;
            //send for player server
            msg_init(&msg);
            msg.sender = msg.receiver = SERVER_ALIYUN;
            msg.message = MSG_PLAYER_REQUEST;
            msg.arg_in.duck = msg.arg_pass.duck = STREAM_SOURCE_PLAYER;
            msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
            msg.arg = &player;
            msg.arg_size = sizeof(player_init_t);
            global_common_send_message( SERVER_PLAYER, &msg);
#endif
            log_goke(DEBUG_WARNING,"LV_STREAM_CMD_LIVE");
            //send for video server
            msg_init(&msg);
            msg.message = MSG_VIDEO_START;
            msg.sender = msg.receiver = SERVER_ALIYUN;
            if( param->live.stream_type == 0 ) {
                msg.arg_in.cat = msg.arg_pass.cat = VIDEO_STREAM_LOW;
            } else {
                msg.arg_in.cat = msg.arg_pass.cat = VIDEO_STREAM_HIGH;
            }
            msg.arg_in.duck = msg.arg_pass.duck = STREAM_SOURCE_LIVE;
            msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
            global_common_send_message( SERVER_VIDEO, &msg);
            //send for audio server
            msg.message = MSG_AUDIO_START;
            global_common_send_message( SERVER_AUDIO, &msg);
            break;
        case LV_STREAM_CMD_STORAGE_RECORD_BY_UTC_TIME:
            log_goke(DEBUG_WARNING,"storage record, start = %u, stop = %u",
                    param->by_utc.start_time, param->by_utc.stop_time);
            //player init
            memset(&player, 0, sizeof(player_init_t));
            player.start = param->by_utc.start_time + param->by_utc.seek_time;  //UTC
            player.stop = param->by_utc.stop_time;
            player.offset = 0;
            player.speed = 1;
            player.channel_merge = 0;
            player.switch_to_live = 1;
            player.audio = 1;
            player.type = param->by_utc.record_type;
            player.channel = param->common.service_id;
            //send for player server
            msg_init(&msg);
            msg.sender = msg.receiver = SERVER_ALIYUN;
            msg.message = MSG_PLAYER_REQUEST;
            msg.arg_in.duck = msg.arg_pass.duck = STREAM_SOURCE_PLAYER;
            msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
            msg.arg = &player;
            msg.arg_size = sizeof(player_init_t);
            global_common_send_message( SERVER_PLAYER, &msg);
            break;
        case LV_STREAM_CMD_STORAGE_RECORD_BY_FILE: {
            log_goke(DEBUG_WARNING, "storage record, start = %u, stop = %u",
                     param->by_utc.start_time, param->by_utc.stop_time);
            //player init
            memset(&player, 0, sizeof(player_init_t));
            player.start = 0;
            player.stop = 0;
            player.offset = param->by_file.seek_time;
            player.speed = 1;
            player.channel_merge = 0;
            player.switch_to_live = 1;
            player.audio = 1;
            player.channel = param->common.service_id;
            //send for player server
            msg_init(&msg);
            msg.sender = msg.receiver = SERVER_ALIYUN;
            msg.message = MSG_PLAYER_REQUEST;
            msg.arg_in.duck = msg.arg_pass.duck = STREAM_SOURCE_PLAYER;
            msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
            msg.arg = &player;
            msg.arg_size = sizeof(player_init_t);
            if( strlen( param->by_file.file_name) > 0 ) {
                strcpy( player.filename, param->by_file.file_name);
                global_common_send_message(SERVER_PLAYER, &msg);
            } else {
                log_goke( DEBUG_SERIOUS, " error in request player, by file mode without file name");
            }
            break;
        }
        case LV_STREAM_CMD_PRE_EVENT_RECORD:
            break;
        case LV_STREAM_CMD_VOICE:
            msg_init(&msg);
            msg.message = MSG_AUDIO_START;
            msg.sender = msg.receiver = SERVER_ALIYUN;
            msg.arg_in.cat = 2;//intercom
            msg.arg_in.duck = msg.arg_pass.duck = STREAM_SOURCE_LIVE;
            msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
            ret = global_common_send_message(SERVER_AUDIO,&msg);
            if(ret) {
                log_goke(DEBUG_WARNING,"lv_voice_intercom_start_service ret = %d", ret);
            }
            break;
        default:
            break;
    }
    return 0;
}

static int stop_push_streaming_cb(const lv_device_auth_s *auth, const lv_stop_push_stream_param_s *param)
{
    int ret;
    message_t msg;
    
    log_goke(DEBUG_WARNING,"stopPushStreamingCallback:%d", param->service_id);
    msg_init(&msg);
    msg.sender = msg.receiver = SERVER_ALIYUN;
    msg.arg_in.wolf = msg.arg_pass.wolf = param->service_id;
    msg.message = MSG_VIDEO_STOP;
    global_common_send_message(SERVER_VIDEO, &msg);
    msg.message = MSG_AUDIO_STOP;
    global_common_send_message( SERVER_AUDIO, &msg);
    msg.message = MSG_PLAYER_STOP;
    global_common_send_message(SERVER_PLAYER, &msg);
    return 0;
}

static int on_push_streaming_cmd_cb(const lv_device_auth_s *auth, const lv_on_push_stream_cmd_param_s *param)
{
    message_t msg;
    int ret;
    printf("on_push_streaming_cmd_cb service_id:%d, stream_cmd_type:%d cmd:%d %d",
           param->common.service_id, param->common.stream_cmd_type, param->common.cmd_type, param->seek.timestamp_ms);

    if (param->common.cmd_type == LV_LIVE_REQUEST_I_FRAME) {
        log_goke(DEBUG_WARNING,"I Frame request");
        msg_init(&msg);
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
        msg.message = MSG_VIDEO_KEY_FRAME_REQUIRE;
        global_common_send_message(SERVER_VIDEO, &msg);
    } else if (param->common.cmd_type == LV_STORAGE_RECORD_START) {
        log_goke(DEBUG_WARNING,"LV_STORAGE_RECORD_START, timestamp = %u, speed = %u, key = %d",
                 param->seek.timestamp_ms, param->set_param.speed, param->set_param.key_only);
        msg_init(&msg);
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.message = MSG_PLAYER_START;
        msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
        global_common_send_message(SERVER_PLAYER, &msg);
    } else if (param->common.cmd_type == LV_STORAGE_RECORD_STOP) {
        log_goke(DEBUG_WARNING,"LV_STORAGE_RECORD_STOP, timestamp = %u, speed = %u, key = %d",
                 param->seek.timestamp_ms, param->set_param.speed, param->set_param.key_only);
        msg_init(&msg);
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.message = MSG_PLAYER_STOP;
        msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
        server_recorder_message(&msg);
    }
    else if (param->common.cmd_type == LV_STORAGE_RECORD_PAUSE) {
        log_goke(DEBUG_WARNING,"LV_STORAGE_RECORD_PAUSE, timestamp = %u, speed = %u, key = %d",
                 param->seek.timestamp_ms, param->set_param.speed, param->set_param.key_only);

        msg_init(&msg);
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.message = MSG_PLAYER_PAUSE;
        msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
        server_recorder_message(&msg);
    }
    else if (param->common.cmd_type == LV_STORAGE_RECORD_UNPAUSE) {
        log_goke(DEBUG_WARNING,"LV_STORAGE_RECORD_UNPAUSE, timestamp = %u, speed = %u, key = %d",
                 param->seek.timestamp_ms, param->set_param.speed, param->set_param.key_only);

        msg_init(&msg);
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.message = MSG_PLAYER_UNPAUSE;
        msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
        server_recorder_message(&msg);
    }
    else if (param->common.cmd_type == LV_STORAGE_RECORD_SEEK) {
        log_goke(DEBUG_WARNING,"LV_STORAGE_RECORD_SEEK, timestamp = %u, speed = %u, key = %d",
                param->seek.timestamp_ms, param->set_param.speed, param->set_param.key_only);
        msg_init(&msg);
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
        msg.arg_in.cat = param->seek.timestamp_ms;
        msg.message = MSG_PLAYER_SEEK;
        global_common_send_message(SERVER_PLAYER, &msg);
    }
    else if (param->common.cmd_type == LV_STORAGE_RECORD_SET_PARAM) {
        log_goke(DEBUG_WARNING,"LV_STORAGE_RECORD_SET_PARAM, timestamp = %u, speed = %u, key = %d",
                param->seek.timestamp_ms, param->set_param.speed, param->set_param.key_only);
        msg_init(&msg);
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.arg_in.wolf = msg.arg_pass.wolf = param->common.service_id;
        msg.arg_in.cat = param->seek.timestamp_ms;
        msg.arg_in.dog = param->set_param.key_only;
        msg.arg_in.chick = param->set_param.speed;
        msg.message = MSG_PLAYER_SET_PARAM;
        global_common_send_message(SERVER_PLAYER, &msg);
    }
    return 0;
}

static int on_push_streaming_data_cb(const lv_device_auth_s *auth, const lv_on_push_streaming_data_param_s *param) {
    log_goke(DEBUG_MAX, "Receive voice data, param = %d %d %d %d, size = %u, timestamp = %u",
           param->audio_param->format, param->audio_param->channel, param->audio_param->sample_bits, param->audio_param->sample_rate, param->len, param->timestamp);

    message_t msg;
    msg_init(&msg);
    msg.message = MSG_AUDIO_SPEAKER_DATA;
    msg.arg_in.cat = SPEAKER_CTL_INTERCOM_DATA;
    msg.arg = (void *)param->p;
    msg.arg_size = param->len;
    msg.arg_in.wolf = param->service_id;
    global_common_send_message(SERVER_AUDIO,&msg);
    return 0;
}

static int feature_check_cb(void) {
    /* demo未实现预录事件录像，未启用预建联功能 */
    return LV_FEATURE_CHECK_CLOSE;
}

static int voice_intercom_receive_metadata_cb(int service_id, 
        const lv_audio_param_s *audio_param) 
{
    log_goke(DEBUG_WARNING,"voice intercom receive metadata, service_id:%d", service_id);
    return 0;
}

static void voice_intercom_receive_data_cb(const char *buffer, 
        unsigned int buffer_size) 
{
}

static void query_storage_record_cb(const lv_device_auth_s *auth, const lv_query_record_param_s *param) {
    int i, ret;
    int max;
    message_t message;
    if (!param) {
        return;
    }
    log_goke( DEBUG_INFO,"query_storage_record_cb");
    if (param->common.type == LV_QUERY_RECORD_BY_DAY) {
        lv_query_record_response_param_s response;
        memset(&response, 0, sizeof(lv_query_record_response_param_s));
        if( param->by_day.num == 0 ) {
            max = 32;
        } else {
            max = param->by_day.num;
        }
        ret = file_manager_get_file_by_day( param->by_day.type, max, param->by_day.start_time, param->by_day.stop_time, &response);
        if( !ret ) {
            lv_post_query_record(param->common.service_id, &response);
            for(int i=0; i< response.by_day.num; i++ ) {
                if( response.by_day.days[i].file_name) {
                    free(response.by_day.days[i].file_name);
                }
            }
            if( response.by_day.days) {
                free(response.by_day.days);
            }
        }
    } else if (param->common.type == LV_QUERY_RECORD_BY_MONTH) {

    }
    return;
}

static int trigger_pic_capture_cb(const char *trigger_id)
{
    char fname[MAX_SYSTEM_STRING_SIZE * 2];
    log_goke(DEBUG_WARNING,"trigger_id:%s", trigger_id);
    message_t msg;
    memset(fname, 0, sizeof(fname));
    sprintf(fname, "%s%s/ss-%s.jpg", manager_config.media_directory,
            manager_config.folder_prefix[LV_STORAGE_RECORD_INITIATIVE], trigger_id);
    /**********************************************/
    /**********************************************/
    msg_init(&msg);
    msg.sender = msg.receiver = SERVER_ALIYUN;
    msg.arg_in.cat = SNAP_TYPE_CLOUD;
    msg.arg = fname;
    msg.arg_size = strlen(fname) + 1;
    server_video_snap_message(&msg);
    /**********************************************/
    return 0;
}

static int trigger_record_cb(int type, int duration, int pre_duration) 
{
    log_goke(DEBUG_WARNING,"");
    return 0;
}

static int cloud_event_cb(lv_cloud_event_type_e type, const lv_cloud_event_param_s *param) 
{
    log_goke(DEBUG_WARNING,"cloud_event_cb: %d ", type);
    if (type == LV_CLOUD_EVENT_DOWNLOAD_FILE) {
        log_goke(DEBUG_WARNING,"cloud_event_cb %d %s %s",
                param->file_download.file_type, 
                param->file_download.file_name, 
                param->file_download.url);
    } else if (type == LV_CLOUD_EVENT_UPLOAD_FILE) {
        log_goke(DEBUG_WARNING,"cloud_event_cb %d %s %s",
                param->file_upload.file_type, 
                param->file_upload.file_name, 
                param->file_upload.url);
    }
    return 0;
}

/**
 * linkvisual_demo_init负责LinkVisual相关回调的注册。
 * 1. 开发者需要注册相关认证信息和回调函数，并进行初始化
 * 2. 根据回调函数中的信息，完成音视频流的上传
 * 3. 当前文件主要用于打印回调函数的命令，并模拟IPC的行为进行了音视频的传输
 */
int linkvisual_init_client(void)
{
    lv_init_config_s config;
    lv_init_callback_s callback;
    lv_init_system_s system;

    log_goke(DEBUG_WARNING,"before init linkvisual");
    if (g_init) {
        log_goke(DEBUG_WARNING,"linkvisual_demo_init already init");
        return 0;
    }
    memset(&config, 0, sizeof(lv_init_config_s));
    memset(&callback, 0, sizeof(lv_init_callback_s));
    memset(&system, 0, sizeof(lv_init_system_s));

    //config
    config.device_type = 0;
    config.sub_num = 0;
    /* SDK的日志配置 */
    if( _config_.debug_level == DEBUG_MAX) {
        aliyun_config.linkkit_debug_level = LV_LOG_MAX;
    } else if( _config_.debug_level == DEBUG_VERBOSE) {
        aliyun_config.linkkit_debug_level = LV_LOG_VERBOSE;
    } else if( _config_.debug_level == DEBUG_INFO) {
        aliyun_config.linkkit_debug_level = LV_LOG_INFO;
    } else if( _config_.debug_level == DEBUG_WARNING) {
        aliyun_config.linkkit_debug_level = LV_LOG_WARN;
    } else if( _config_.debug_level == DEBUG_SERIOUS) {
        aliyun_config.linkkit_debug_level = LV_LOG_ERROR;
    } else if( _config_.debug_level == DEBUG_NONE) {
        aliyun_config.linkkit_debug_level = IOT_LOG_NONE;
    }
    config.log_level = aliyun_config.linkkit_debug_level;
    config.log_dest = LV_LOG_DESTINATION_STDOUT;
    /* 码流路数限制 */
    config.storage_record_source_solo_num = 1;
    config.storage_record_source_num = 4;//该参数仅对NVR有效
    /* 图片性能控制 */
    config.image_size_max = 1024 * 1024;//内存申请不超过1M
    config.image_parallel = 2;//2路并发
    /* 码流检查功能 */
    config.stream_auto_check = 1;
#if 1 /* 码流保存为文件功能，按需使用 */
    config.stream_auto_save = 0;
#else
    config.stream_auto_save = 1;
    char *path = "/tmp/";
    memcpy(config.stream_auto_save_path, path, strlen(path));
#endif
    /* das默认开启 */
    config.das_close = 1;
    config.dns_mode = LV_DNS_SYSTEM;

    //callback注册
    callback.message_publish_cb = linkkit_message_publish_cb;
    //音视频推流服务
    callback.start_push_streaming_cb = start_push_streaming_cb;
    callback.stop_push_streaming_cb = stop_push_streaming_cb;
    callback.on_push_streaming_cmd_cb = on_push_streaming_cmd_cb;
    callback.on_push_streaming_data_cb = on_push_streaming_data_cb;
    //获取存储录像录像列表
    callback.query_storage_record_cb = query_storage_record_cb;
    callback.trigger_picture_cb = trigger_pic_capture_cb;
    /* 云端事件通知 */
    callback.cloud_event_cb = cloud_event_cb;
    callback.feature_check_cb = feature_check_cb;

    //先准备好LinkVisual相关资源
    int ret = lv_init(&config, &callback, &system);
    if (ret < 0) {
        printf("lv_init failed, result = %d", ret);
        return -1;
    }
    g_init = 1;
    log_goke(DEBUG_WARNING,"after init linkvisual");
    return 0;
}

void linkvisual_client_stop(void) {
    log_goke(DEBUG_WARNING, "before destroy linkvisual");
    if (!g_init) {
        log_goke(DEBUG_WARNING, "linkvisual_demo_destroy is not init");
        return;
    }
	g_init = 0;
    lv_destroy();
    log_goke(DEBUG_WARNING, "after destroy linkvisual");
}