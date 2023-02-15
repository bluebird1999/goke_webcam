#ifndef SERVER_MANAGER_CONFIG_H_
#define SERVER_MANAGER_CONFIG_H_

/*
 * header
 */
#include "../../global/global_interface.h"

/*
 * define
 */
#define RECORDER_FOLDER_NUM  3

typedef struct manager_config_t {
    unsigned char			running_mode;
    unsigned char			debug_level;
    unsigned char           goke_path[MAX_SYSTEM_STRING_SIZE*2];
    sleep_t			        sleep;
    int						timezone;
    unsigned int			server_start;
    unsigned int			server_sleep;
    unsigned char			fail_restart;
    unsigned char			fail_restart_interval;
    unsigned char			cache_clean;
    unsigned char			msg_overrun;
    char				    media_directory[MAX_SYSTEM_STRING_SIZE];
    char				    folder_prefix[RECORDER_FOLDER_NUM][MAX_SYSTEM_STRING_SIZE];
    iotx_cloud_region_types_t   iot_server;
    int                     msg_buffer_size_media;
    int                     msg_buffer_size_small;
    int                     msg_buffer_size_normal;
    int                     msg_buffer_size_large;
} manager_config_t;

manager_config_t    manager_config;

#endif
