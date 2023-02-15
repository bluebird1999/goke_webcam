#ifndef SERVER_ALIYUN_LINKVISUAL_CLIENT_H
#define SERVER_ALIYUN_LINKVISUAL_CLIENT_H

#if defined(__cplusplus)
extern "C" {
#endif

#include <link_visual_api.h>
#include "aliyun.h"

int linkvisual_init_client(void);
void linkvisual_client_stop(void);

#if defined(__cplusplus)
}
#endif
#endif
