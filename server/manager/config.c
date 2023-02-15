/*
 * header
 */
//system header
//program header
//server header
#include "config.h"
#include "link_visual_enum.h"

manager_config_t manager_config = {
    .running_mode = RUNNING_MODE_NORMAL,
    .debug_level = DEBUG_VERBOSE,
    .goke_path = "/app/config/",
    .timezone = 80,
    .sleep.enable = 0,
    .sleep.repeat = 0,
    .sleep.weekday = 0,
    .sleep.start = "00:00",
    .sleep.stop = "00:00",
    .server_start = 254,
    .server_sleep = 254,
    .msg_overrun = 1,
    .fail_restart = 1,
    .fail_restart_interval = 3,
    .cache_clean = 1,
    .media_directory = "/mnt/sdcard/media/",
    .folder_prefix[LV_STORAGE_RECORD_PLAN] = "plan",
    .folder_prefix[LV_STORAGE_RECORD_ALARM] = "alarm",
    .folder_prefix[LV_STORAGE_RECORD_INITIATIVE] = "normal",
    .iot_server = IOTX_CLOUD_REGION_SHANGHAI,
    .msg_buffer_size_small = 8,
    .msg_buffer_size_normal = 16,
    .msg_buffer_size_large = 16,
    .msg_buffer_size_media = 16,
};
