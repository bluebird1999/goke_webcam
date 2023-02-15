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
#include "manager_interface.h"
#include "time.h"
#include "sleep.h"
/*
 * static
 */
//variable
static scheduler_time_t  	scheduler;
static int					mode;
static int					enable;

//specific
static int sleep_get_scheduler_time(char *input, scheduler_time_t *st, int *mode);
static int sleep_check_scheduler_time(scheduler_time_t *st, int *mode);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static int sleep_get_scheduler_time(char *input, scheduler_time_t *st, int *mode)
{
    char timestr[16];
    char start_hour_str[4]={0};
    char start_min_str[4]={0};
    char end_hour_str[4]={0};
    char end_min_str[4]={0};
    int start_hour = 0;
    int start_min = 0;
    int end_hour = 0;
    int end_min = 0;
    if(strlen(input) > 0)
    {
        memcpy(timestr,input,strlen(input));
		memcpy(start_hour_str,timestr,2);
        start_hour_str[2] = '\0';
		memcpy(start_min_str,timestr+3,2);
        start_min_str[2] = '\0';
        memcpy(end_hour_str,timestr+3+3,2);
        end_hour_str[2] = '\0';
		memcpy(end_min_str,timestr+3+3+3,2);
        end_min_str[2] = '\0';
        log_goke(DEBUG_INFO, "time:%s:%s-%s:%s",start_hour_str,start_min_str,end_hour_str,end_min_str);
        start_hour =  atoi(start_hour_str);
        start_min =  atoi(start_min_str);
        end_hour =  atoi(end_hour_str);
        end_min =  atoi(end_min_str);

        if(!start_hour&&!start_min&&!end_hour&&!end_min){
            *mode = 0;
        }
        else {
        	*mode = 1;
			st->start_hour = start_hour;
			st->start_min = start_min;
			st->start_sec= 0;
            if(end_hour > start_hour) {
    			st->stop_hour = end_hour;
    			st->stop_min = end_min;
    			st->stop_sec= 0;
            }
            else if(end_hour == start_hour) {
                if(end_min > start_min) {
        			st->stop_hour = end_hour;
        			st->stop_min = end_min;
        			st->stop_sec= 0;
                }
                else if(end_min == start_min) {
                    *mode = 0;
        			st->stop_hour = end_hour;
        			st->stop_min = end_min;
        			st->stop_sec= 0;
                }
                else {
        			st->stop_hour = 23;
        			st->stop_min = 59;
        			st->stop_sec= 0;
                }
            }
            else {
    			st->stop_hour = 23;
    			st->stop_min = 59;
    			st->stop_sec= 0;
            }
        }
    }
    else
    	*mode = 0;
    return 0;
}

static int sleep_check_scheduler_time(scheduler_time_t *st, int *mode)
{
	int ret = 0;
    time_t timep;
    struct tm  tv={0};
    int	start, end, now;

	if( *mode==0 ) return 1;
    timep = time(NULL);
    localtime_r(&timep, &tv);
    start = st->start_hour * 3600 + st->start_min * 60 + st->start_sec;
    end = st->stop_hour * 3600 + st->stop_min * 60 + st->stop_sec;
    now = tv.tm_hour * 3600 + tv.tm_min * 60 + tv.tm_sec;
    if( now >= start && now <= end ) return 1;
    else if( now > end) return 2;
    return ret;
}

static int sleep_check_scheduler(void)
{
	int ret = 0;
	if( enable == 2 ) {
		ret = sleep_check_scheduler_time(&scheduler, &mode);
		if( ret==1 ) {
			manager_deep_sleep();
		}
		else {
			manager_wakeup();
		}
	}
	return ret;
}

/*
 * interface
 */
int sleep_scheduler_init(int n, char *start, char*stop)
{
	int ret = 0;
	char final[MAX_SYSTEM_STRING_SIZE];
	memset(final, 0, MAX_SYSTEM_STRING_SIZE);
	sprintf(final, "%s-%s", start, stop);
	ret = sleep_get_scheduler_time(final, &scheduler, &mode);
	enable = n;
	return ret;
}

int sleep_init(int n, char *start, char*stop)
{
	int ret;
	message_t msg;
	sleep_scheduler_init(n, start,stop);
	enable = n;
	/********message body********/
	msg_init(&msg);
	msg.message = MSG_MANAGER_TIMER_ADD;
	msg.sender = SERVER_MANAGER;
	msg.arg_in.cat = 60*1000;
	msg.arg_in.dog = 0;
	msg.arg_in.duck = 0;
	msg.arg_in.handler = &sleep_check_scheduler;
	ret = global_common_send_message(SERVER_MANAGER, &msg);
	/****************************/
	return ret;
}

int sleep_release(void)
{
	int ret;
	message_t msg;
	/********message body********/
	msg_init(&msg);
	msg.message = MSG_MANAGER_TIMER_REMOVE;
	msg.sender = msg.receiver = SERVER_MANAGER;
	msg.arg_in.handler = sleep_check_scheduler;
	ret = global_common_send_message(SERVER_MANAGER, &msg);
	/****************************/
	memset(&scheduler, 0, sizeof(scheduler));
	return ret;
}
