#ifndef SERVER_MANAGER_SLEEP_INTERFACE_H_
#define SERVER_MANAGER_SLEEP_INTERFACE_H_

/*
 * header
 */
#include "manager_interface.h"

/*
 * define
 */

/*
 * structure
 */

/*
 * function
 */
int sleep_init(int n, char *start, char*stop);
int sleep_release(void);
int sleep_scheduler_init(int n, char *start, char*stop);

#endif
