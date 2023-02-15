#ifndef SERVER_ALIYUN_CONFIG_H_
#define SERVER_ALIYUN_CONFIG_H_

/*
 * header
 */
#include <iot_export.h>
#include <link_visual_enum.h>

/*
 * define
 */

/*
 * structure
 */
typedef struct aliyun_config_t {
    int     linkkit_debug_leval;
    int     linkvisual_debug_leval;
} aliyun_config_t;

aliyun_config_t   aliyun_config;

#endif
