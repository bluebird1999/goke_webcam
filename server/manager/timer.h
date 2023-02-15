#ifndef SERVER_MANAGER_TIMER_INTERFACE_H_
#define SERVER_MANAGER_TIMER_INTERFACE_H_

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
int timer_init(void);
int timer_proc(int server);
int timer_release(void);
int timer_remove(HANDLER const task_handler);
int timer_add(HANDLER const task_handler, int interval, int delay, int oneshot, int sender, message_arg_t arg);

#endif
