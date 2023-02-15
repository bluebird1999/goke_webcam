#ifndef __NTPC_H__
#define __NTPC_H__

#ifdef __cplusplus
extern "C"{
#endif

#define NTP_SERVER    "ntp.aliyun.com"

int ntpcdate(char *ntp_server);

#ifdef __cplusplus
}
#endif
#endif

