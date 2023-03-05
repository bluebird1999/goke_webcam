#include <stdio.h>
#include <sys/time.h>
#include <iot_export.h>
#include <iot_import.h>
#include <link_visual_api.h>
#include <link_visual_enum.h>
#include <link_visual_struct.h>
//system
#include "../../server/video/video_interface.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../global/global_interface.h"
#include "../../server/device/wifi_tools.h"
#include "../../server/device/gk_sd.h"
#include "../../server/device/gk_gpio.h"
#include "../../server/device/gk_motor.h"

//server
#include "linkkit_client.h"
#include "aliyun.h"
#include "aliyun_interface.h"
#include "config.h"


int g_master_dev_id = -1;
static int g_running = 0;
static void *g_thread = NULL;
static int g_thread_not_quit = 0;
static int g_connect = 0;
static int is_wifi_start = 0;
static int is_iot_start = 0;

int linkkit_sync_property_int(int iDevid, char *pPropName, int iValue);
int linkkit_sync_property_string(int iDevid, char *pPropName, char *pValueStr);
int Ali_GetWifiConf(stAliWifiConf *pstConf);

/* 这个字符串数组用于说明LinkVisual需要处理的物模型服务 */
static char *link_visual_service[] = {
        "TriggerPicCapture",//触发设备抓图
        "StartVoiceIntercom",//开始语音对讲
        "StopVoiceIntercom",//停止语音对讲
        "StartVod",//开始录像观看
        "StartVodByTime",//开始录像按时间观看
        "StopVod",//停止录像观看
        "QueryRecordTimeList",//查询录像列表
        "QueryRecordList",//查询录像列表
        "StartP2PStreaming",//开始P2P直播
        "StartPushStreaming",//开始直播
        "StopPushStreaming",//停止直播
        "QueryMonthRecord",//按月查询卡录像列表
//        "EjectStorageCard",//弹出存储卡
//        "FormatStorageMedium",//格式化存储卡
//        "Reboot",//休眠
//        "PTZActionControl",//PTZ控制
};

void aliyun_report_sd_info(void) {
    char acReportBuf[1024];
    sd_info_t stInfo;
    memset(acReportBuf, 0, sizeof(acReportBuf));
    int status = sd_get_status();
    if (status == SD_ALIYUN_STATUS_NORMAL) {
        memset(&stInfo, 0, sizeof(sd_info_t));
        usleep(50000);
        if (sd_get_info(&stInfo) == 0) {
            sprintf(acReportBuf, "{\"StorageStatus\":1,\"StorageTotalCapacity\":%d,\"StorageRemainCapacity\":%d}",
                    stInfo.totalBytes / 1024, stInfo.freeBytes / 1024);
        } else {
            sprintf(acReportBuf, "{\"StorageStatus\":0}");
        }
    } else {
        sprintf(acReportBuf, "{\"StorageStatus\":%d}", status);
    }
    int iDevid = g_master_dev_id;
    log_goke(DEBUG_WARNING,"Data:%s",acReportBuf);
    if(g_master_dev_id >= 0)
        IOT_Linkkit_Report(iDevid, ITM_MSG_POST_PROPERTY,(unsigned char *) acReportBuf, strlen(acReportBuf));
}

int Ali_SetStreamVideoQuality(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_in.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_QUALITY;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof( item->valueint );
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetStreamVideoQuality(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.video_quality);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetSubStreamVideoQuality(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_in.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_SUBQUALITY;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof( item->valueint );
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetSubStreamVideoQuality(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.subvideo_quality);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetAlarmSwitch(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_MOTION_SWITCH;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetAlarmSwitch(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.alarm_switch);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetMotionDetectSensitivity(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_MOTION_SENSITIVITY;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetMotionDetectSensitivity(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.motion_detect_sensitivity);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetAlarmFrequencyLevel(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_MOTION_ALARM_INTERVAL;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetAlarmFrequencyLevel(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.alarm_freqency);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetImageFlipState(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_IMAGE_FLIP;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetImageFlipState(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.video_mirror);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetIpcVersion(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsString(item)) {
            strcpy( _config_.ipc_version, item->valuestring);
            linkkit_sync_property_string(devid, pCmd, _config_.ipc_version );
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetIpcVersion(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_string(devid, pCmd, _config_.ipc_version);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetDebugCmd(int devid, char *pData, char *pCmd) {
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    return 0;
}

int Ali_SetDebugCmd(int devid, char *pData, char *pCmd) {
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    return 0;
}

int Ali_SetDayNightMode(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_NIGHT_SHOT;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetDayNightMode(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.day_night_mode);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetAlarmNotifyPlan(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    cJSON *client_list = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsArray(item)) {
            int day, begin, end;
            client_list = item->child;
            while (client_list != NULL) {
                day = cJSON_GetObjectItem(client_list, "DayOfWeek")->valueint;
                begin = cJSON_GetObjectItem(client_list, "BeginTime")->valueint;
                end = cJSON_GetObjectItem(client_list, "EndTime")->valueint;
                client_list = client_list->next;
                log_goke(DEBUG_WARNING, "week:%d,time:%d-%d", day, begin, end);
                msg_init(&msg);
                msg.arg_pass.wolf = devid;
                msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
                msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_MOTION_PLAN;
                msg.arg_in.dog = day;
                msg.arg_in.chick = begin;
                msg.arg_in.duck = end;
                global_common_send_message(SERVER_VIDEO, &msg);
            }
        } else {
            log_goke(DEBUG_WARNING, " ");
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_MOTION_PLAN;
            msg.arg_in.dog = -1;
            msg.arg_in.chick = -1;
            msg.arg_in.duck = -1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetAlarmNotifyPlan(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetTimeDisplay(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_TIME_WATERMARK;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetTimeDisplay(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.osd_enable);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetWdrSupport(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_WDR;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetWdrSupport(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.wdr_enable);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetWiFiRSSI(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int iWifiQuality = 0;
    stAliWifiConf stWifiConf;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            memset(&stWifiConf, 0, sizeof(stAliWifiConf));
            if (Ali_GetWifiConf(&stWifiConf) == 0) {
#ifdef RELEASE_VERSION
		        GetWifiQuality(stWifiConf.s,&iWifiQuality);
#endif
            }
            linkkit_sync_property_int(devid, pCmd, iWifiQuality);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetSpeakerSwitch(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int value;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_AUDIO_PROPERTY_SET;
            msg.arg_in.cat = msg.arg_pass.cat = AUDIO_PROPERTY_SPEAKER;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_AUDIO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetSpeakerSwitch(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_int(devid, pCmd, _config_.speaker_enable);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetStorageStatus(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int value;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            value = sd_get_status();
            linkkit_sync_property_int(devid, pCmd, value);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetStorageTotalCapacity(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int value = 0;
    sd_info_t stSdCardInfo;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            if (sd_get_info(&stSdCardInfo) == 0) {
                value = stSdCardInfo.totalBytes / 1024;
            }
            linkkit_sync_property_int(devid, pCmd, value);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetStorageRemainCapacity(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int value = 0;
    sd_info_t stSdCardInfo;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            if (sd_get_info(&stSdCardInfo) == 0) {
                value = stSdCardInfo.freeBytes / 1024;
            }
            linkkit_sync_property_int(devid, pCmd, value);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetStatusLightSwitch(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int value;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            value = item->valueint;
            if( value != _config_.led_enable ) {
                if( value ) {
                    if( g_connect ) {
                        gk_led1_off();
                        gk_led2_on();
                    } else {
                        gk_led1_on();
                        gk_led2_off();
                    }
                } else {
                    gk_led1_off();
                    gk_led2_off();
                }
                _config_.led_enable = value;
                config_set(&_config_);  //save
                log_goke(DEBUG_INFO, "changed the led status = %d", _config_.led_enable);
            }
            linkkit_sync_property_int(devid, pCmd, value);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetStatusLightSwitch(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int value = 0;
    sd_info_t stSdCardInfo;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            linkkit_sync_property_int(devid, pCmd, _config_.led_enable);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetStorageRecordMode(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_RECORDER_PROPERTY_SET;
            msg.arg_in.cat = msg.arg_pass.cat = RECORDER_PROPERTY_SWITCH;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_RECORDER, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetStorageRecordMode(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    sd_info_t stSdCardInfo;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            linkkit_sync_property_int(devid, pCmd, _config_.recorder_enable);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetImageCorrect(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int value;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            msg_init(&msg);
            msg.arg_pass.wolf = devid;
            msg.message = MSG_VIDEO_PROPERTY_SET_DIRECT;
            msg.arg_in.cat = msg.arg_pass.cat = VIDEO_PROPERTY_IMAGE_CORRECTION;
            msg.arg = &item->valueint;
            msg.arg_size = sizeof(item->valueint);
            msg.extra = pCmd;
            msg.extra_size = strlen(pCmd) + 1;
            global_common_send_message(SERVER_VIDEO, &msg);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetImageCorrect(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    sd_info_t stSdCardInfo;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            linkkit_sync_property_int(devid, pCmd, _config_.image_correct);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetDeviceName(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    message_t msg;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsString(item)) {
            strcpy( _config_.device_name, item->valuestring);
            linkkit_sync_property_string(devid, pCmd, _config_.device_name );
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetDeviceName(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item) {
            linkkit_sync_property_string(devid, pCmd, _config_.device_name);
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_SetDeviceTimeZone(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    int ret;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            if( _config_.time_zone != item->valueint) {
                ret = device_set_timezone(item->valueint);
            }
            if(!ret) {
                linkkit_sync_property_int(devid, pCmd, item->valueint);
            } else {
                linkkit_sync_property_int(devid, pCmd, _config_.time_zone);
            }
        }
        cJSON_Delete(root);
    }
    return 0;
}

int Ali_GetDeviceTimeZone(int devid, char *pData, char *pCmd) {
    cJSON *root = NULL;
    cJSON *item = NULL;
    sd_info_t stSdCardInfo;
    log_goke(DEBUG_WARNING, "Data:%s", pData);
    root = cJSON_Parse(pData);
    if (root) {
        item = cJSON_GetObjectItem(root, pCmd);
        if (item && cJSON_IsNumber(item)) {
            linkkit_sync_property_int(devid, pCmd, _config_.time_zone);
        }
        cJSON_Delete(root);
    }
    return 0;
}

stAliCmd gAliCmdMap[] =
        {
                {E_ALI_PROPERTY_STREAM_VIDEO_QUALITY,       "StreamVideoQuality",       Ali_SetStreamVideoQuality,      Ali_GetStreamVideoQuality},
                {E_ALI_PROPERTY_SUB_STREAM_VIDIO_QUALITY,   "SubStreamVideoQuality",    Ali_SetSubStreamVideoQuality,   Ali_GetSubStreamVideoQuality},
                {E_ALI_PROPERTY_ALARM_SWITCH,               "AlarmSwitch",              Ali_SetAlarmSwitch,             Ali_GetAlarmSwitch},
                {E_ALI_PROPERTY_MOTION_DETECT_SENSITIVITY,  "MotionDetectSensitivity",  Ali_SetMotionDetectSensitivity, Ali_GetMotionDetectSensitivity},
                {E_ALI_PROPERTY_ALARM_FREQUENCY_LEVEL,      "AlarmFrequencyLevel",      Ali_SetAlarmFrequencyLevel,     Ali_GetAlarmFrequencyLevel},
                {E_ALI_PROPERTY_IMAGE_FLIP_STATE,           "ImageFlipState",           Ali_SetImageFlipState,          Ali_GetImageFlipState},
                {E_ALI_PROPERTY_IPC_VERSION,                "IpcVersion",               Ali_SetIpcVersion,              Ali_GetIpcVersion},
                {E_ALI_PROPERTY_DEBUG_CMD,                  "DebugCmd",                 Ali_SetDebugCmd,                Ali_GetDebugCmd},
                {E_ALI_PROPERTY_DAY_NIGHT_MODE,             "DayNightMode",             Ali_SetDayNightMode,            Ali_GetDayNightMode},
                {E_ALI_PROPERTY_ALARM_NOTIFY_PLAN,          "AlarmNotifyPlan",          Ali_SetAlarmNotifyPlan,         Ali_GetAlarmNotifyPlan},
                {E_ALI_PROPERTY_TIMEDISPLAY,                "TimeDisplay",              Ali_SetTimeDisplay,             Ali_GetTimeDisplay},
                {E_ALI_PROPERTY_WDRSUPPORT,                 "WdrSupport",               Ali_SetWdrSupport,              Ali_GetWdrSupport},
                {E_ALI_PROPERTY_WIFIRSSI,                   "WiFiRSSI",                 0,                              Ali_GetWiFiRSSI},
                {E_ALI_PROPERTY_SPEEKERSWITCH,              "SpeakerSwitch",            Ali_SetSpeakerSwitch,           Ali_GetSpeakerSwitch},
                {E_ALI_PROPERTY_STORAGESTATUS,              "StorageStatus",            0,                              Ali_GetStorageStatus},
                {E_ALI_PROPERTY_STORAGETOTALCAPACITY,       "StorageTotalCapacity",     0,                              Ali_GetStorageTotalCapacity},
                {E_ALI_PROPERTY_STORAGEREMAINCAPACITY,      "StorageRemainCapacity",    0,                              Ali_GetStorageRemainCapacity},
                {E_ALI_PROPERTY_STATUSLIGHTSWITCH,          "StatusLightSwitch",        Ali_SetStatusLightSwitch,       Ali_GetStatusLightSwitch},
                {E_ALI_PROPERTY_STORAGERECORDMODE,          "StorageRecordMode",        Ali_SetStorageRecordMode,       Ali_GetStorageRecordMode},
                {E_ALI_PROPERTY_IMAGECORRECT,               "ImageCorrect",             Ali_SetImageCorrect,            Ali_GetImageCorrect},
                {E_ALI_PROPERTY_DEVICENAME,                 "DeviceName",               Ali_SetDeviceName,              Ali_GetDeviceName},
                {E_ALI_PROPERTY_DEVICETIMEZONE,                 "DeviceTimeZone",               Ali_SetDeviceTimeZone,              Ali_GetDeviceTimeZone},
        };

int Ali_GetWifiConf(stAliWifiConf *pstConf) {
    FILE *fp = NULL;
    cJSON *pRoot = NULL, *pItem = NULL;
    int iSize = 0;
    char acBuf[1024];
    if (access(WIFI_INFO_CONF, F_OK) == 0) {
        fp = fopen(WIFI_INFO_CONF, "rb");
        if (fp) {
            iSize = fread(acBuf, 1, sizeof(acBuf), fp);
            if (iSize > 0) {
                pRoot = cJSON_Parse(acBuf);
                if (pRoot) {
                    pItem = cJSON_GetObjectItem(pRoot, "v");
                    if (pItem && cJSON_IsString(pItem)) {
                        snprintf(pstConf->v, sizeof(pstConf->v) - 1, "%s", pItem->valuestring);
                    }

                    pItem = cJSON_GetObjectItem(pRoot, "s");
                    if (pItem && cJSON_IsString(pItem)) {
                        snprintf(pstConf->s, sizeof(pstConf->s) - 1, "%s", pItem->valuestring);
                        log_goke(DEBUG_WARNING, "ssid:%s", pstConf->s);
                    }

                    pItem = cJSON_GetObjectItem(pRoot, "p");
                    if (pItem && cJSON_IsString(pItem)) {
                        snprintf(pstConf->p, sizeof(pstConf->p) - 1, "%s", pItem->valuestring);
                    }

                    pItem = cJSON_GetObjectItem(pRoot, "b");
                    if (pItem && cJSON_IsString(pItem)) {
                        snprintf(pstConf->b, sizeof(pstConf->b) - 1, "%s", pItem->valuestring);
                    }

                    pItem = cJSON_GetObjectItem(pRoot, "t");
                    if (pItem && cJSON_IsString(pItem)) {
                        snprintf(pstConf->t, sizeof(pstConf->t) - 1, "%s", pItem->valuestring);
                    }
                    cJSON_Delete(pRoot);
                    return 0;
                }
            }
            fclose(fp);
        }
    }
    return -1;
}

int Ali_ReportProperty(int iDevid) {
    int iWifiQuality = 0;
    char *pReportInfo = NULL;
    cJSON *pRoot = NULL;
    stAliWifiConf stWifiConf;
    sd_info_t stSdcardInfo;
    memset(&stSdcardInfo, 0, sizeof(sd_info_t));
    sd_get_info(&stSdcardInfo);
    memset(&stWifiConf, 0, sizeof(stAliWifiConf));
    if (Ali_GetWifiConf(&stWifiConf) == 0) {
#ifdef RELEASE_VERSION
		GetWifiQuality(stWifiConf.s,&iWifiQuality);
#endif
    }

    pRoot = cJSON_CreateObject();
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_STREAM_VIDEO_QUALITY].strCmd, _config_.video_quality);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_SUB_STREAM_VIDIO_QUALITY].strCmd, _config_.subvideo_quality);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_ALARM_SWITCH].strCmd, _config_.alarm_switch);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_MOTION_DETECT_SENSITIVITY].strCmd, _config_.motion_detect_sensitivity);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_ALARM_FREQUENCY_LEVEL].strCmd, _config_.alarm_freqency);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_IMAGE_FLIP_STATE].strCmd, _config_.video_mirror);
    cJSON_AddStringToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_IPC_VERSION].strCmd, _config_.ipc_version);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_DAY_NIGHT_MODE].strCmd, _config_.day_night_mode);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_TIMEDISPLAY].strCmd, _config_.osd_enable);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_WDRSUPPORT].strCmd, _config_.wdr_enable);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_WIFIRSSI].strCmd, iWifiQuality);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_SPEEKERSWITCH].strCmd, _config_.speaker_enable);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_STORAGESTATUS].strCmd, sd_get_status());
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_STORAGETOTALCAPACITY].strCmd,
                            stSdcardInfo.totalBytes / 1024);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_STORAGEREMAINCAPACITY].strCmd,
                            stSdcardInfo.freeBytes / 1024);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_STATUSLIGHTSWITCH].strCmd, _config_.led_enable);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_STORAGERECORDMODE].strCmd, _config_.recorder_enable);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_IMAGECORRECT].strCmd, _config_.image_correct);
    cJSON_AddStringToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_DEVICENAME].strCmd, _config_.device_name);
    cJSON_AddNumberToObject(pRoot, gAliCmdMap[E_ALI_PROPERTY_DEVICETIMEZONE].strCmd, _config_.time_zone);
    pReportInfo = cJSON_Print(pRoot);
    IOT_Linkkit_Report(iDevid, ITM_MSG_POST_PROPERTY,
                       (unsigned char *) pReportInfo, strlen(pReportInfo));

    cJSON_Delete(pRoot);
    return 0;
}

static iotx_linkkit_dev_meta_info_t *g_master_dev_info = NULL;

extern int iotx_dm_get_triple_by_devid(int devid, char **product_key, char **device_name, char **device_secret);


int linkkit_message_publish_cb(const lv_message_publish_param_s *param) {
    iotx_mqtt_topic_info_t topic_msg;

    /* Initialize topic information */
    memset(&topic_msg, 0x0, sizeof(iotx_mqtt_topic_info_t));
    topic_msg.qos = param->qos;
    topic_msg.retain = 0;
    topic_msg.dup = 0;
    topic_msg.payload = param->message;
    topic_msg.payload_len = strlen(param->message);
    int rc = IOT_MQTT_Publish(NULL, param->topic, &topic_msg);
    if (rc < 0) {
        log_goke(DEBUG_WARNING, "Publish msg error:%d", rc);
        return -1;
    }

    return 0;
}

static int user_connected_event_handler(void) {
    message_t msg;

    lv_device_auth_s auth;
    linkit_get_auth(0, &auth);//这个回调只有主设备才会进入

    msg_init(&msg);
    //   msg.message = MSG_SPEAKER_WIFI_CONNECT;
    server_audio_message(&msg);

    log_goke(DEBUG_WARNING, "==================================Cloud Connected");
    g_connect = 1;
    //led
    if(_config_.led_enable) {
        gk_led1_off();   //red
        gk_led2_on();  //green
    }
//    //若为二维码扫码配网，记得作扫码的收尾工作
//    if(wifi_st == WIFI_QRCODE) {
//        set_video_status(VIDEO_QRCODE_STOP);
//    }

    /**
     * Linkkit连接后，上报设备的属性值。
     * 当APP查询设备属性时，会直接从云端获取到设备上报的属性值，而不会下发查询指令。
     * 对于设备自身会变化的属性值（存储使用量等），设备可以主动隔一段时间进行上报。
     */
    Ali_ReportProperty(g_master_dev_id);
    /* Linkkit连接后，查询下ntp服务器的时间戳，用于同步服务器时间。查询结果在user_timestamp_reply_handler中 */
    IOT_Linkkit_Query(g_master_dev_id, ITM_MSG_QUERY_TIMESTAMP, NULL, 0);

    /** 查询是否有新的固件： **/
    IOT_Linkkit_Query(g_master_dev_id, ITM_MSG_REQUEST_FOTA_IMAGE,
                      (unsigned char *) ("app-1.0.0-20180101.1001"), 30);

    // linkkit上线消息同步给LinkVisual
    lv_message_adapter_param_s in = {0};
    in.type = LV_MESSAGE_ADAPTER_TYPE_CONNECTED;
    lv_message_adapter(&auth, &in);
    return 0;
}

static int user_disconnected_event_handler(void) {
    log_goke(DEBUG_WARNING, "Cloud Disconnected");
    g_connect = 0;
    //led
    if(_config_.led_enable) {
        gk_led1_on();   //red
        gk_led2_off();  //green
    }
    return 0;
}

static int user_service_request_handler(const int devid, const char *id, const int id_len,
                                        const char *serviceid, const int serviceid_len,
                                        const char *request, const int request_len,
                                        char **response, int *response_len) {

    log_goke(DEBUG_WARNING,
             "Service Request Received, Devid: %d,id_len:%d, ID %s,serviceid_len:%d, Service ID: %s, request: %s",
             devid, id_len, id, serviceid_len, serviceid, request);

    /* 部分物模型服务消息由LinkVisual处理，部分需要自行处理。 */
    int link_visual_process = 0;
    for (unsigned int i = 0; i < sizeof(link_visual_service) / sizeof(link_visual_service[0]); i++) {
        /* 这里需要根据字符串的长度来判断 */
        if (!strncmp(serviceid, link_visual_service[i], strlen(link_visual_service[i]))) {
            log_goke(DEBUG_WARNING, "serviceid: %.*s", serviceid_len, serviceid);
            link_visual_process = 1;
            break;
        }
    }

    if (link_visual_process) {
        /* ISV将某些服务类消息交由LinkVisual来处理，不需要处理response */
        lv_device_auth_s auth;
        linkit_get_auth(devid, &auth);
        lv_message_adapter_param_s in = {0};
        in.type = LV_MESSAGE_ADAPTER_TYPE_TSL_SERVICE;
        in.msg_id = (char *) id;
        in.msg_id_len = id_len;
        in.service_name = (char *) serviceid;
        in.service_name_len = serviceid_len;
        in.request = (char *) request;
        in.request_len = request_len;
        int ret = lv_message_adapter(&auth, &in);
        if (ret < 0) {
            log_goke(DEBUG_WARNING, "LinkVisual process service request failed, ret = %d", ret);
            return -1;
        }
        log_goke(DEBUG_WARNING, "lv_ipc_linkkit_adapter");

    } else {
        /* 非LinkVisual处理的消息示例 */
        if (!strncmp(serviceid, "PTZActionControl", (serviceid_len > 0) ? serviceid_len : 0)) {
            cJSON *root = cJSON_Parse(request);
            if (root == NULL) {
                log_goke(DEBUG_WARNING, "JSON Parse Error");
                return -1;
            }
            cJSON *child = cJSON_GetObjectItem(root, "ActionType");
            if (!child) {
                log_goke(DEBUG_WARNING, "JSON Parse Error");
                cJSON_Delete(root);
                return -1;
            }
            int action_type = child->valueint;
            child = cJSON_GetObjectItem(root, "Step");
            if (!child) {
                log_goke(DEBUG_WARNING, "JSON Parse Error");
                cJSON_Delete(root);
                return -1;
            }
            int step = child->valueint;

            cJSON_Delete(root);
            log_goke(DEBUG_WARNING, "PTZActionControl %d %d", action_type, step);
            motor_control(action_type, step);
        } else if (!strncmp(serviceid, FORMATSTORAGEMEDIUM, (serviceid_len > 0) ? serviceid_len : 0)) {
            log_goke(DEBUG_WARNING, "%s", FORMATSTORAGEMEDIUM);
            sd_format();
        } else if (!strncmp(serviceid, EJECTSTORAGECARD, (serviceid_len > 0) ? serviceid_len : 0)) {
            log_goke(DEBUG_WARNING, "%s", EJECTSTORAGECARD);
            sd_unmount();
        } else if (!strncmp(serviceid, REBOOT, (serviceid_len > 0) ? serviceid_len : 0)) {
            log_goke(DEBUG_WARNING, "%s", REBOOT);
            device_reboot();
        } else {
            log_goke(DEBUG_WARNING, "unknown linkkit service id = %s", serviceid);
        }
    }

    return 0;
}

//{"identifier":"awss.BindNotify","value":{"Operation":"Unbind"}}
//{"identifier":"awss.BindNotify","value":{"Operation":"Bind"}}
static int parse_Event_Notify(const cJSON *const root) {
    char *json_item_str = "", *json_item_str1 = "";
    cJSON *item, *item1, *item2;
    char cmd[256];
    item = cJSON_GetObjectItem(root, "identifier");
    if (item) {
        if (cJSON_IsString(item)) {
            json_item_str = cJSON_Print(item);
            if (strncmp(json_item_str, "\"awss.BindNotify\"", strlen("\"awss.BindNotify\"")) != 0) {
                log_goke(DEBUG_WARNING, "json_item_str = %s", json_item_str);
                cJSON_free(json_item_str);
                return -1;
            }
        }
    } else {
        //log_goke(DEBUG_WARNING,"cJSON_GetObjectItem item err");
        return -1;
    }

    item1 = cJSON_GetObjectItem(root, "value");
    if (item1) {
        item2 = cJSON_GetObjectItem(item1, "Operation");
        if (cJSON_IsString(item2)) {
            json_item_str1 = cJSON_Print(item2);
            if (strncmp(json_item_str1, "\"Unbind\"", strlen("\"Unbind\"")) != 0) {
                log_goke(DEBUG_WARNING, "json_item_str1 = %s", json_item_str1);
                cJSON_free(json_item_str);
                cJSON_free(json_item_str1);
                return -1;
            }
        }
    } else {
        log_goke(DEBUG_WARNING, "cJSON_GetObjectItem item1 err");
        cJSON_free(json_item_str);
        return -1;
    }
    log_goke(DEBUG_WARNING, "operation unbind");

    message_t send_msg;
    msg_init(&send_msg);
    send_msg.sender = send_msg.receiver = SERVER_ALIYUN;
    send_msg.message = MSG_ALIYUN_RESTART;
    global_common_send_message(SERVER_ALIYUN, &send_msg);
    return 0;
}

int linkkit_sync_property_int(int iDevid, char *pPropName, int iValue) {
    cJSON *report = NULL;
    char *report_str;
    report = cJSON_CreateObject();
    if (report) {
        cJSON_AddNumberToObject(report, pPropName, iValue);

        report_str = cJSON_Print(report);
        IOT_Linkkit_Report(iDevid, ITM_MSG_POST_PROPERTY,
                           (unsigned char *) report_str, strlen(report_str));
        free(report_str);
        cJSON_Delete(report);
        return 0;
    } else {
        return -1;
    }
}

int linkkit_sync_property_string(int iDevid, char *pPropName, char *pValueStr) {
    cJSON *report = NULL;
    char *report_str;
    report = cJSON_CreateObject();
    if (report) {
        cJSON_AddStringToObject(report, pPropName, pValueStr);

        report_str = cJSON_Print(report);
        IOT_Linkkit_Report(iDevid, ITM_MSG_POST_PROPERTY,
                           (unsigned char *) report_str, strlen(report_str));
        free(report_str);
        cJSON_Delete(report);
        return 0;
    } else {
        return -1;
    }
}

static int user_property_set_handler(const int devid, const char *request, const int request_len) {
    log_goke(DEBUG_WARNING, "Property Set Received, Devid: %d, Request: %s", devid, request);
    int ii = 0;
    for (ii = 0; ii < E_ALI_PROPERTY_MAX; ii++) {
        if (strstr(request, gAliCmdMap[ii].strCmd)) {
            gAliCmdMap[ii].pSetCb(devid, request, gAliCmdMap[ii].strCmd);
            return 0;
        }
    }
    return 0;
}

static int user_property_get_handler(const int devid, const char *request, const int request_len) {
    log_goke(DEBUG_WARNING, "Property Get Received, Devid: %d, Request: %s", devid, request);
    int ii = 0;
    for (ii = 0; ii < E_ALI_PROPERTY_MAX; ii++) {
        if (strstr(request, gAliCmdMap[ii].strCmd)) {
            gAliCmdMap[ii].pGetCb(devid, request, gAliCmdMap[ii].strCmd);
            return 0;
        }
    }
    return 0;
}

static int user_timestamp_reply_handler(const char *timestamp) {
    struct timeval tv;
    message_t msg;
    long long llTimestamp = 0;
    log_goke(DEBUG_WARNING, "Current Timestamp: %s ", timestamp);//时间戳为字符串格式，单位：毫秒

    llTimestamp = atoll(timestamp) / 1000;
    tv.tv_sec = (unsigned int) llTimestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);

    msg_init(&msg);
    msg.message = MSG_ALIYUN_TIME_SYNCHRONIZED;
    global_common_send_message(SERVER_RECORDER, &msg);
    global_common_send_message(SERVER_PLAYER, &msg);
    global_common_send_message( SERVER_ALIYUN, &msg);
    return 0;
}

static int user_fota_handler(int type, const char *version) {
    char buffer[1024] = {0};
    int buffer_length = 1024;
    FILE *fp;

    log_goke(DEBUG_WARNING, "into");

    if (type == 0) {
        log_goke(DEBUG_WARNING, "New Firmware Version: %s", version);

        fp = fopen(FIRMWARE_INFO_TEMP, "w+");
        if (fp) {
            fwrite(version, strlen(version), 1, fp);
            fclose(fp);
        }

        IOT_Linkkit_Query(g_master_dev_id, ITM_MSG_QUERY_FOTA_DATA, (unsigned char *) buffer, buffer_length);
    }

    return 0;
}

static int user_event_notify_event_handler(const int devid, const char *request, const int request_len) {
    log_goke(DEBUG_WARNING, "Event Notify Received, Devid: %d, Request: %s ", devid, request);
    return 0;
}


int user_event_notify_dev_bind(const int state_code, const char *state_message) {
    log_goke(DEBUG_WARNING, "dev bind result = %d message=%s", state_code, state_message);
    message_t send_msg;
    if (state_code == -2052) {//绑定成功的标识
        if (access(WIFI_INFO_CONF, F_OK) != 0) {
            rename(WIFI_INFO_CONF_TEMP, WIFI_INFO_CONF);
            /****************************/
            msg_init(&send_msg);
            send_msg.message = MSG_AUDIO_PLAY_SOUND;
            send_msg.arg_in.cat = AUDIO_RESOURCE_BINDING_SUCCESS;
            global_common_send_message(SERVER_AUDIO, &send_msg);
            /****************************/
        }
    } else {

    }
    return 0;
}

static int user_link_visual_handler(const int devid, const char *service_id,
                                    const int service_id_len, const char *payload,
                                    const int payload_len) {
    /* Linkvisual自定义的消息，直接全交由LinkVisual来处理 */
    if (payload == NULL || payload_len == 0) {
        return 0;
    }
    /* 此处请注意：需要使用devid准确查询出实际的三元组 */
    lv_device_auth_s auth;
    linkit_get_auth(devid, &auth);

    lv_message_adapter_param_s in = {0};
    in.type = LV_MESSAGE_ADAPTER_TYPE_LINK_VISUAL;
    in.service_name = (char *) service_id;
    in.service_name_len = service_id_len;
    in.request = (char *) payload;
    in.request_len = payload_len;
    int ret = lv_message_adapter(&auth, &in);
    if (ret < 0) {
        log_goke(DEBUG_WARNING, "LinkVisual process service request failed, ret = %d", ret);
        return -1;
    }

    return 0;
}

/* 属性设置回调 */
static void linkkit_client_set_property_handler(const char *key, const char *value) {
    /**
     * 当收到属性设置时，开发者需要修改设备配置、改写已存储的属性值，并上报最新属性值。demo只上报了最新属性值。
     */
    char result[1024] = {0};
    snprintf(result, 1024, "{\"%s\":\"%s\"}", key, value);
    log_goke(DEBUG_WARNING, "result:%s", result);
    IOT_Linkkit_Report(0, ITM_MSG_POST_PROPERTY, (unsigned char *) result, strlen(result));
}

int linkkit_client_start(int need_bind) {
    FILE *fp = NULL;
    message_t send_msg;
    char cmd[256];
    int ret = 0;
    log_goke(DEBUG_WARNING, "linkkit_client_start ...");

    //check wifi connection
    if (is_wifi_start == 0) {
#ifdef RELEASE_VERSION
        ret = wifi_link_ok();
#else
        log_goke( DEBUG_WARNING, "---wifi link check not available in DEBUG version!---");
        ret = 1;
#endif
        if (ret) {
            is_wifi_start = 1;
        } else {
#ifdef RELEASE_VERSION
            wifi_connect();
            ret = wifi_link_ok();   //try again once
            if(ret) {
                is_wifi_start = 1;
            }
#else
            log_goke( DEBUG_WARNING, "---wifi connect not available in DEBUG version!---");
#endif
        }
    }

    if (is_wifi_start) {
#ifdef RELEASE_VERSION
        if( need_bind ) {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "/usr/sbin/wpa_cli -i wlan0 save_config");
//ning
            ret = system(cmd);
            usleep(100000);
        }
#else
        log_goke( DEBUG_WARNING, "---wpa conf save not available in DEBUG version!---");
#endif
        /* 连接到服务器 */
        if (is_iot_start == 0) {
            ret = IOT_Linkkit_Connect(g_master_dev_id);
            if (ret < 0) {
                log_goke(DEBUG_WARNING, "IOT_Linkkit_Connect Failed");
                return -1;
            } else {
                is_iot_start = 1;
            }
        }
        return ret;
    }
    return -1;
}

int linkkit_init_client(const char *product_key,
                        const char *product_secret,
                        const char *device_name,
                        const char *device_secret) {
    if (g_running) {
        log_goke(DEBUG_WARNING, "Already running");
        return 0;
    }
    g_running = 1;

    /* 设置调试的日志级别 */
    if( _config_.debug_level == DEBUG_MAX) {
        aliyun_config.linkkit_debug_level = IOT_LOG_DEBUG;
    } else if( _config_.debug_level == DEBUG_VERBOSE) {
        aliyun_config.linkkit_debug_level = IOT_LOG_DEBUG;
    } else if( _config_.debug_level == DEBUG_INFO) {
        aliyun_config.linkkit_debug_level = IOT_LOG_INFO;
    } else if( _config_.debug_level == DEBUG_WARNING) {
        aliyun_config.linkkit_debug_level = IOT_LOG_WARNING;
    } else if( _config_.debug_level == DEBUG_SERIOUS) {
        aliyun_config.linkkit_debug_level = IOT_LOG_CRIT;
    } else if( _config_.debug_level == DEBUG_NONE) {
        aliyun_config.linkkit_debug_level = IOT_LOG_NONE;
    }
    IOT_SetLogLevel(aliyun_config.linkkit_debug_level);
    /* Smart Living */
    aiot_kv_init();
    /* 注册链接状态的回调 */
    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);

    /* 注册消息通知 */
    IOT_RegisterCallback(ITE_LINK_VISUAL, user_link_visual_handler);//linkvisual自定义消息
    IOT_RegisterCallback(ITE_SERVICE_REQUST, user_service_request_handler);//物模型服务类消息
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_handler);//物模型属性设置
    IOT_RegisterCallback(ITE_PROPERTY_GET, user_property_get_handler);//物模型属性设置
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_handler);//NTP时间
    IOT_RegisterCallback(ITE_FOTA, user_fota_handler);//固件OTA升级事件
    IOT_RegisterCallback(ITE_EVENT_NOTIFY, user_event_notify_event_handler);
    IOT_RegisterCallback(ITE_STATE_DEV_BIND, user_event_notify_dev_bind);

    if (!g_master_dev_info) {
        g_master_dev_info = (iotx_linkkit_dev_meta_info_t *) malloc(sizeof(iotx_linkkit_dev_meta_info_t));
        if (!g_master_dev_info) {
            log_goke(DEBUG_WARNING, "Malloc failed ");
            return -1;
        }
        memset(g_master_dev_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    }

    strncpy(g_master_dev_info->product_key, product_key, PRODUCT_KEY_MAXLEN - 1);
    strncpy(g_master_dev_info->product_secret, product_secret, PRODUCT_SECRET_MAXLEN - 1);
    strncpy(g_master_dev_info->device_name, device_name, DEVICE_NAME_MAXLEN - 1);
    strncpy(g_master_dev_info->device_secret, device_secret, DEVICE_SECRET_MAXLEN - 1);

    /* 选择服务器地址，当前使用上海服务器 */
    int domain_type = manager_config.iot_server;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *) &domain_type);

    /* 动态注册 */
    int dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *) &dynamic_register);

    /* 创建linkkit资源 */
    g_master_dev_id = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, g_master_dev_info);
    if (g_master_dev_id < 0) {
        log_goke(DEBUG_WARNING, "IOT_Linkkit_Open Failed");
        return -1;
    }
    return 0;
}


void linkkit_client_destroy(void) {
    log_goke(DEBUG_WARNING, "Before destroy linkkit");
    if (!g_running) {
        return;
    }
    g_running = 0;
    is_wifi_start = 0;
#if 0
    /* 等待线程退出，并释放线程资源，也可用分离式线程，但需要保证线程不使用linkkit资源后，再去释放linkkit */
    while (g_thread_not_quit) {
        HAL_SleepMs(20);
    }
    if (g_thread) {
        HAL_ThreadDelete(g_thread);
        g_thread = NULL;
    }
#endif
    IOT_Linkkit_Close(g_master_dev_id);
    g_master_dev_id = -1;
    is_iot_start = 0;
    //aiot_kv_deinit();
    log_goke(DEBUG_WARNING, "After destroy linkkit");
}

void aliyun_time_sync(void) {
    IOT_Linkkit_Query(g_master_dev_id, ITM_MSG_QUERY_TIMESTAMP, NULL, 0);
}


void linkit_get_auth(int dev_id, lv_device_auth_s *auth) {
    if (dev_id > 0) {
        /* 此处请注意：需要使用devid准确查询出实际的三元组 */
        char *product_key = NULL;
        char *device_name = NULL;
        char *device_secret = NULL;
        iotx_dm_get_triple_by_devid(dev_id, &product_key, &device_name, &device_secret);
        auth->dev_id = dev_id;
        auth->product_key = product_key;
        auth->device_name = device_name;
        auth->device_secret = device_secret;
    } else if (dev_id == 0) {
        /* Notice:这里本应该也使用iotx_dm_get_triple_by_devid进行查询，方便代码统一。
         * 但此函数查询devid=0时，会丢失掉device_secret数据(有bug)
         * 此处改为直接使用主设备的三元组信息，不进行查询 */
        auth->dev_id = dev_id;
        auth->product_key = g_master_dev_info->product_key;
        auth->device_name = g_master_dev_info->device_name;
#ifdef LINKKIT_DYNAMIC_REGISTER
        HAL_GetDeviceSecret(g_master_dev_info->device_secret);
#endif
        auth->device_secret = g_master_dev_info->device_secret;
    }
}