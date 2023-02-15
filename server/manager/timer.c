/*
 * header
 */
//system header
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
//program header
#include "../../common/tools_interface.h"
//server header
#include "time.h"
#include "manager_interface.h"
/*
 * static
 */
//variable
static int timer_run = 0;
static timer_struct_t timers[TIMER_NUMBER];
static pthread_rwlock_t timer_lock = PTHREAD_RWLOCK_INITIALIZER;
//function;
static unsigned int timer_get_ms(void);

//specific

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static unsigned int timer_get_ms(void)
{
    struct timeval tv;
    unsigned int time;
    gettimeofday(&tv, NULL);
    time = tv.tv_sec*1000  + tv.tv_usec / 1000;
    return time;
}

/*
 * interface
 */
int timer_add(HANDLER const task_handler, int interval, int delay, int oneshot, int sender, message_arg_t arg)
{
    int i;
    int ret = 0;
    if ((timer_run == 0) || (task_handler == NULL)) {
        return -1;
    }
    pthread_rwlock_wrlock(&timer_lock);
    int usable_id = 1110;
    for (i = 0; i < TIMER_NUMBER; i++) {
        if (timers[i].tid == 0 && (usable_id == 1110)) {
            usable_id = i;
        }
        if (timers[i].tid != 0 && task_handler == timers[i].fpcallback) {
            usable_id = 1110;
            break;
        }
    }
    if (i == TIMER_NUMBER) {
        if(usable_id == 1110) {
            log_goke(DEBUG_SERIOUS, "timer_add_task: timer list full");
            ret = -1;
            goto out;
        } else {
            i = usable_id;
        }
    }
    memset(&timers[i],0,sizeof(timer_struct_t));
    timers[i].fpcallback = task_handler;

    timers[i].tid = i+1;
    timers[i].interval = interval;
    timers[i].oneshot = oneshot;
    timers[i].delay = delay;
    timers[i].sender = sender;
    memcpy(&timers[i].arg, &arg, sizeof(message_arg_t));

    if( timers[i].delay == -1 )
    	timers[i].tick = 0;
    else
    	timers[i].tick = timer_get_ms() + delay;
out:    pthread_rwlock_unlock(&timer_lock);
    return ret;
}

int timer_remove(HANDLER const task_handler)
{
    int i;
    if(timer_run == 0 || task_handler==NULL) {
        return -1;
    }
    for(i = 0; i < TIMER_NUMBER; i++) {
        if(timers[i].tid != 0 &&  task_handler == timers[i].fpcallback) {
            break;
        }
    }
    if(i != TIMER_NUMBER)
    {
        pthread_rwlock_wrlock(&timer_lock);
        memset(&timers[i],0,sizeof(timer_struct_t));
        pthread_rwlock_unlock(&timer_lock);
    }
    return 0;
}

/*
 * interface internal
 */
int timer_init(void)
{
	int i;
    if( !timer_run ) {
        for (i = 0; i < TIMER_NUMBER; i++) {
            memset(&timers[i], 0, sizeof(timer_struct_t));
        }
        timer_run = 1;
    }
	return 0;
}

int timer_proc(int server)
{
    int i, ret=0;
    timer_struct_t *timer;
    timer_struct_t extimer;
    unsigned int now ;

	for(i = 0; i < TIMER_NUMBER; i++) {
		if( timers[i].tid != 0 ) {
			pthread_rwlock_wrlock(&timer_lock);
			timer = &timers[i];
			now = timer_get_ms();
			if( (misc_get_bit( server, timer->sender ) == 0)
					&& (timer->sender != 32) ) {	//if server is already gone
				memset(timer,0,sizeof(timer_struct_t));
				pthread_rwlock_unlock(&timer_lock);
				continue;
			}
			if( ( now < timer->tick) || (now > (timer->tick + timer->interval)) ) {
				memcpy(&extimer,timer,sizeof(timer_struct_t));
				if(timer->oneshot == 1) {
					memset(timer,0,sizeof(timer_struct_t));
				}
				else {
					timer->tick = timer_get_ms();
				}
				pthread_rwlock_unlock(&timer_lock);
//				extimer.fpcallback();
				message_t msg;
			    /********message body********/
				msg_init(&msg);
				msg.message = MSG_MANAGER_TIMER_ON;
				msg.sender = SERVER_MANAGER;
				msg.receiver = extimer.sender;
				msg.arg_in.handler = extimer.fpcallback;
				memcpy(&msg.arg_pass, &extimer.arg, sizeof(message_arg_t));
				/****************************/
				global_common_send_message(extimer.sender, &msg);
			}
			else {
				pthread_rwlock_unlock(&timer_lock);
			}
		}

	}
    return ret;
}

int timer_release(void)
{
	int i,ret=0;
    if( timer_run ) {
        for (i = 0; i < TIMER_NUMBER; i++) {
            memset(&timers[i], 0, sizeof(timer_struct_t));
        }
        timer_run = 0;
    }
	return ret;
}
