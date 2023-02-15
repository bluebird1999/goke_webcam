#ifndef SERVER_ALIYUN_INTERFACE_H
#define SERVER_ALIYUN_INTERFACE_H

/*
 * header
 */
#include <stdbool.h>
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"

/*
 * define
 */

/*
 * structure
 */
#define     ALIYUN_VISUAL_MAX_CHANNEL               4

typedef enum media_type_e {
    MEDIA_TYPE_VIDEO = 0,
    MEDIA_TYPE_AUDIO,
};

typedef enum find_channel_status_t {
    CHANNEL_FIND_OLD = 0,
    CHANNEL_FIND_NEW,
    CHANNEL_FIND_FULL,
} find_channel_status_t;

typedef enum stream_source_type_t {
    STREAM_SOURCE_NONE =  0,
    STREAM_SOURCE_LIVE,
    STREAM_SOURCE_PLAYER,
} stream_source_type_t;

typedef enum aliyun_event_enum_t {
    ALIYUN_EVENT_OBJECT_DETECTED = 0,
};

int server_aliyun_start(void);
int server_aliyun_message(message_t *msg);
int linkkit_sync_property_int(int iDevid, char *pPropName, int iValue);
int linkkit_sync_property_string(int iDevid, char *pPropName, char *pValueStr);

#endif
