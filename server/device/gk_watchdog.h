#ifndef __GK_WATCHDOG_H__
#define __GK_WATCHDOG_H__

#ifdef __cplusplus
extern "C" {
#endif

int gk_wtd_set_timeout(int fd,int timeout);
int gk_wtd_get_timeout(int fd,int *timeout);
int gk_wtd_feed(int fd);
int gk_wtd_open();
int gk_wtd_close(int fd);

#ifdef __cplusplus
}
#endif
#endif
