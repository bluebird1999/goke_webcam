#ifndef SERVER_AUDIO_CONFIG_H_
#define SERVER_AUDIO_CONFIG_H_

/*
 * header
 */
#include "../../global/global_interface.h"
#include "../../server/goke/hisi/hisi_common.h"

/*
 * define
 */

/*
 * structure
 */
typedef struct audio_config_t {
    int							enable;
    int                         ai_dev;
    int                         ao_dev;
    int                         ai_channel;
    int                         ao_channel;
    int                         ae_channel;
    int                         ad_channel;
    char                        codec_file[MAX_SYSTEM_STRING_SIZE];
    AIO_ATTR_S                  aio_attr;
    AI_TALKVQE_CONFIG_S         vqe_attr;
    int                         capture_volume;
    int                         speaker_volume;
    PAYLOAD_TYPE_E              payload_type;
} audio_config_t;

audio_config_t   audio_config;

#endif
