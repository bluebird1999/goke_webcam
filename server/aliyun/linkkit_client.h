#ifndef SERVER_ALIYUN_LINKKIT_CLIENT_H_
#define SERVER_ALIYUN_LINKKIT_CLIENT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <link_visual_api.h>

#define ALARM_EVENT                 "AlarmEvent"
#define INTELLIGENT_ALARM           "IntelligentAlarm"
#define REBOOT                      "Reboot"
#define START_PUSH_STREAMING        "StartPushStreaming"
#define STOP_PUSH_STREAMING         "StopPushStreaming"
#define TRIGGER_PIC_CAPTURE         "TriggerPicCapture"
#define FORMATSTORAGEMEDIUM			"FormatStorageMedium"
#define QUERYRECORDTIMELIST			"QueryRecordTimeList"
#define	STARTVOD					"StartVod"
#define STARTVODBYTIME				"StartVodByTime"
#define QUERYRECORDLIST				"QueryRecordList"
#define PTZACTIONCONTROL			"PTZActionControl"
#define STARTPTZACTION				"StartPTZAction"
#define STOPPTZACTION				"StopPTZAction"
#define EJECTSTORAGECARD			"EjectStorageCard"//弹出存储卡

//属性类
#define ALARM_EVENT                 "AlarmEvent"
#define INTELLIGENT_ALARM           "IntelligentAlarm"
#define STREAM_VIDEO_QUALITY        "StreamVideoQuality"
#define SUB_STREAM_VIDIO_QUALITY    "SubStreamVideoQuality"
#define ALARM_SWITCH                "AlarmSwitch"
#define MOTION_DETECT_SENSITIVITY   "MotionDetectSensitivity"
#define ALARM_FREQUENCY_LEVEL       "AlarmFrequencyLevel"
#define IMAGE_FLIP_STATE            "ImageFlipState"
#define IPC_VERSION                 "IpcVersion"
#define DEBUG_CMD                   "DebugCmd"
#define DAY_NIGHT_MODE              "DayNightMode"
#define ALARM_NOTIFY_PLAN           "AlarmNotifyPlan"

typedef enum E_ALI_PROPERTY_TYPE
{
	E_ALI_PROPERTY_STREAM_VIDEO_QUALITY = 0,
	E_ALI_PROPERTY_SUB_STREAM_VIDIO_QUALITY,
	E_ALI_PROPERTY_ALARM_SWITCH,
	E_ALI_PROPERTY_MOTION_DETECT_SENSITIVITY,
	E_ALI_PROPERTY_ALARM_FREQUENCY_LEVEL,
	E_ALI_PROPERTY_IMAGE_FLIP_STATE,
	E_ALI_PROPERTY_IPC_VERSION,
	E_ALI_PROPERTY_DEBUG_CMD,
	E_ALI_PROPERTY_DAY_NIGHT_MODE,
	E_ALI_PROPERTY_ALARM_NOTIFY_PLAN,
	E_ALI_PROPERTY_TIMEDISPLAY,
	E_ALI_PROPERTY_WDRSUPPORT,
	E_ALI_PROPERTY_WIFIRSSI,
	E_ALI_PROPERTY_SPEEKERSWITCH,
	E_ALI_PROPERTY_STORAGESTATUS,
	E_ALI_PROPERTY_STORAGETOTALCAPACITY,
	E_ALI_PROPERTY_STORAGEREMAINCAPACITY,
	E_ALI_PROPERTY_STATUSLIGHTSWITCH,
	E_ALI_PROPERTY_STORAGERECORDMODE,
	E_ALI_PROPERTY_IMAGECORRECT,
	E_ALI_PROPERTY_DEVICENAME,
	E_ALI_PROPERTY_MAX,
}E_ALI_PROPERTY_TYPE;

typedef int (*AliSetCb)(int devid,char *pData,char *pCmd);
typedef int (*AliGetCb)(int devid,char *pData,char *pCmd);

typedef struct stAliCmd
{
    E_ALI_PROPERTY_TYPE     id;
    char                    strCmd[64];
    AliSetCb                pSetCb;
    AliGetCb                pGetCb;
}stAliCmd;



typedef enum {
    NONE_ENUM,
    ALARM_EVENT_ENUM,
    INTELLIGENT_ALARM_ENUM,
    REBOOT_ENUM,
    START_PUSH_STREAMING_ENUM,
    STOP_PUSH_STREAMING_ENUM,
    TRIGGER_PIC_CAPTURE_ENUM,
    STREAM_VIDEO_QUALITY_ENUM,
    SUB_STREAM_VIDIO_QUALITY_ENUM,
    ALARM_SWITCH_ENUM,
    MOTION_DETECT_SENSITIVITY_ENUM,
    ALARM_FREQUENCY_LEVEL_ENUM,
    IMAGE_FLIP_STATE_ENUM,
    IPC_VERSION_ENUM
}CLOUD_FUNCTION_EVENT;


typedef struct stAliWifiConf
{
	char v[128];
	char s[128];//wifi ssid
	char p[128];//wifi passwd
	char b[128];
	char t[128];
} stAliWifiConf;

/* linkkit与服务器断开连接，并释放资源 */
void linkkit_client_destroy();

void linkkit_client_run(void);

int linkkit_client_restart(void);
int linkkit_client_start(int need_bind);

int linkkit_init_client(const char *product_key,
                         const char *product_secret,
                         const char *device_name,
                         const char *device_secret);

void aliyun_report_sd_info(void);
void aliyun_time_sync(void);
int linkkit_message_publish_cb(const lv_message_publish_param_s *param);
int Ali_ReportSdcardInfo(char *pData);

#if defined(__cplusplus)
}
#endif
#endif
