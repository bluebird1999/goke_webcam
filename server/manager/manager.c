/*
 * header
 */
//system header
#include <stdio.h>
#include <pthread.h>
#include <syscall.h>
#include <signal.h>
#include <sys/time.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
//program header
#include "../device/device_interface.h"
#include "../goke/goke_interface.h"
#include "../audio/audio_interface.h"
#include "../aliyun/aliyun_interface.h"
#include "../recorder/recorder_interface.h"
#include "../player/player_interface.h"
#include "../video/video_interface.h"
#include "../../common/tools_interface.h"
#include "../../global/global_interface.h"
//server header
#include "manager.h"
#include "manager_interface.h"
#include "timer.h"
#include "sleep.h"
#include "config.h"

/*
 * static
 */

//variable
static message_buffer_t message;
static server_info_t 	info;
static pthread_rwlock_t	ilock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t	mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	cond = PTHREAD_COND_INITIALIZER;

//function;
static int server_message_proc(void);
static void task_deep_sleep(void);
static void task_sleep(void);
static void task_default(void);
static void task_testing(void);
static void task_exit(void);
static void main_thread_termination(void);
//specific
static int manager_server_start(int server);
static void manager_sleep(void);
static void manager_send_wakeup(int server);
/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static void *manager_timer_func(void)
{
    signal(SIGINT, main_thread_termination);
    signal(SIGTERM, main_thread_termination);
	pthread_detach(pthread_self());
	//default task
    pthread_rwlock_wrlock(&ilock);
	misc_set_bit(&info.error, THREAD_TIMER);
    pthread_rwlock_unlock(&ilock);
	misc_set_thread_name("manager_timer");
	while( 1 ) {
        pthread_rwlock_rdlock(&ilock);
		if(info.exit ) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
		if( misc_get_bit(info.thread_exit, THREAD_TIMER) ) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        pthread_rwlock_unlock(&ilock);
		timer_proc(info.thread_start);
		usleep(100000);
	}
	timer_release();
    pthread_rwlock_wrlock(&ilock);
	misc_clear_bit(&info.error, THREAD_TIMER);
    pthread_rwlock_unlock(&ilock);
	global_common_send_dummy(SERVER_MANAGER);
	log_goke(DEBUG_INFO, "-----------thread exit: timer-----------");
	pthread_exit(0);
}

static int manager_routine_proc(void)
{
	if( manager_config.cache_clean ) {
		system("sync");
		usleep(1000);
		system("echo 3 > /proc/sys/vm/drop_caches");
	}
}

static int manager_routine_init(void)
{
	int ret;
	message_t msg;
	/********message body********/
	msg_init(&msg);
	msg.message = MSG_MANAGER_TIMER_ADD;
	msg.sender = SERVER_MANAGER;
	msg.arg_in.cat = 60*1000;
	msg.arg_in.dog = 0;
	msg.arg_in.duck = 0;
	msg.arg_in.handler = manager_routine_proc;
	ret = global_common_send_message(SERVER_MANAGER, &msg);
	/****************************/
	return ret;
}

static int manager_get_property(message_t *msg)
{
	int ret = 0, st;
	int temp;
	message_t send_msg;
    /********message body********/
	msg_init(&send_msg);
	memcpy(&(send_msg.arg_pass), &(msg->arg_pass),sizeof(message_arg_t));
	send_msg.message = msg->message | 0x1000;
	send_msg.sender = send_msg.receiver = SERVER_MANAGER;
	send_msg.arg_in.cat = msg->arg_in.cat;
	send_msg.result = 0;
	/****************************/
	if( send_msg.arg_in.cat == MANAGER_PROPERTY_SLEEP) {
		if( manager_config.sleep.enable == 0 )
			temp = 1;
		else if( manager_config.sleep.enable == 1 )
			temp = 0;
		else if( manager_config.sleep.enable == 2 )
			temp = 2;
		else
			log_goke(DEBUG_WARNING, "----current sleeping status = %d", manager_config.running_mode );
		send_msg.arg = (void*)(&temp);
		send_msg.arg_size = sizeof(temp);
	}
	ret = global_common_send_message( msg->receiver, &send_msg);
	return ret;
}

static int manager_set_property(message_t *msg)
{
	int ret=0, mode = -1;
	message_t send_msg;
    /********message body********/
	msg_init(&send_msg);
	memcpy(&(send_msg.arg_pass), &(msg->arg_pass),sizeof(message_arg_t));
	send_msg.message = msg->message | 0x1000;
	send_msg.sender = send_msg.receiver = SERVER_MANAGER;
	send_msg.arg_in.cat = msg->arg_in.cat;
	/****************************/
	send_msg.arg_in.wolf = 0;
	if( msg->arg_in.cat == MANAGER_PROPERTY_SLEEP ) {
		int temp = *((int*)(msg->arg));
		if( ( (temp == 1) && ( manager_config.sleep.enable == 0) ) ||
			( (temp == 0) && ( manager_config.sleep.enable == 1) ) ||
			( (temp == 2) && ( manager_config.sleep.enable == 2) ) ) {
			ret = 0;
		}
		else {
			if( temp == 1 ) {
				if( info.status == STATUS_RUN ) {
					manager_config.running_mode = RUNNING_MODE_NORMAL;
					manager_config.sleep.enable = 0;
					config_set(&_config_);
					manager_wakeup();
					send_msg.arg_in.wolf = 1;
				}
				else {
					log_goke(DEBUG_INFO,"still in previous normal enter processing");
				}
			}
			else if( temp == 0 ) {
				if( info.status == STATUS_RUN ) {
					manager_config.running_mode = RUNNING_MODE_SLEEP;
					manager_config.sleep.enable = 1;
					config_set(&_config_);
					manager_sleep();
					send_msg.arg_in.wolf = 1;
				}
				else {
					log_goke(DEBUG_INFO,"still in previous sleep leaving processing");
				}
			}
		}
		send_msg.arg = (void*)(&temp);
		send_msg.arg_size = sizeof(temp);
	}
	else if( msg->arg_in.cat == MANAGER_PROPERTY_TIMEZONE ) {
		if( msg->arg_in.cat != manager_config.timezone ) {
			log_goke(DEBUG_INFO,"++++++++Set the timezone to %d++++++++", msg->arg_in.cat);
			manager_config.timezone = msg->arg_in.cat;
			config_set(&_config_);
		}
	}
	/***************************/
	send_msg.result = ret;
	ret = global_common_send_message(msg->receiver, &send_msg);
	/***************************/
	return ret;
}

static void manager_send_wakeup(int server)
{
	message_t msg;
	msg_init(&msg);
	msg.message = MSG_MANAGER_WAKEUP;
	msg.sender = msg.receiver = SERVER_MANAGER;
	global_common_send_message(server, &msg);
}

static void test_func(void)
{
	if( info.task.func == task_testing )
		info.status = STATUS_STOP;
}

static void manager_sleep(void)
{
	info.status = STATUS_NONE;
	info.task.func = task_sleep;
}

static int manager_server_start(int server)
{
	int ret=0;
	switch(server) {
        case SERVER_DEVICE:
            if( !server_device_start() )
                misc_set_bit(&info.thread_start, SERVER_DEVICE);
            break;
        case SERVER_GOKE:
            if( !server_goke_start() )
                misc_set_bit(&info.thread_start, SERVER_GOKE);
            break;
        case SERVER_VIDEO:
            if( !server_video_start() )
                misc_set_bit(&info.thread_start, SERVER_VIDEO);
            break;
        case SERVER_AUDIO:
            if( !server_audio_start() )
                misc_set_bit(&info.thread_start, SERVER_AUDIO);
            break;
		case SERVER_ALIYUN:
			if( !server_aliyun_start() )
				misc_set_bit(&info.thread_start, SERVER_ALIYUN);
			break;
		case SERVER_RECORDER:
			if( !server_recorder_start() )
				misc_set_bit(&info.thread_start, SERVER_RECORDER);
			break;
		case SERVER_PLAYER:
			if( !server_player_start() )
				misc_set_bit(&info.thread_start, SERVER_PLAYER);
			break;
	}
	return ret;
}

static int manager_server_stop(int server)
{
	int ret=0;
	message_t msg;
	msg_init(&msg);
	msg.message = MSG_MANAGER_EXIT;
	msg.sender = msg.receiver = SERVER_MANAGER;
	msg.arg_in.cat = info.thread_start & manager_config.server_sleep;
	ret = global_common_send_message(server, &msg);
	return ret;
}

static int heart_beat_proc(void)
{
	int ret = 0;
	long long int tick = 0;
	tick = time_get_now_stamp();
	if( (tick - info.tick) > 3 ) {
		info.tick = tick;
		system("top -n 1 | grep 'Load average:'");
		system("top -n 1 | grep 'webcam'");
		system("top -n 1 | grep 'CPU'");
	}
	return ret;
}

static void manager_kill_all(message_arg_t *msg)
{
	log_goke(DEBUG_INFO, "%%%%%%%%forcefully kill all%%%%%%%%%");
	exit(0);
}

static void manager_delayed_start(message_arg_t *msg)
{
	manager_server_start(msg->cat);
}

static void manager_broadcast_thread_exit(void)
{
}

static void server_release_1(void)
{
}

static void server_release_2(void)
{
	sleep_release();
	msg_buffer_release2(&message, &mutex);
}

static void server_release_3(void)
{
	msg_free(&info.task.msg);
	memset(&info, 0, sizeof(server_info_t));
}

static int server_message_proc(void)
{
	int ret = 0;
	message_t msg;
	message_t send_msg;
//condition
	pthread_mutex_lock(&mutex);
	if( message.head == message.tail ) {
		if( info.status == info.old_status ) {
			pthread_cond_wait(&cond,&mutex);
		}
	}
	if( info.msg_lock ) {
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	msg_init(&msg);
	ret = msg_buffer_pop(&message, &msg);
	pthread_mutex_unlock(&mutex);
	if( ret == 1 ) {
		return 0;
	}
    log_goke(DEBUG_VERBOSE, "MANAGER message popped: sender=%s, message=%s, head=%d, tail=%d",
             global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
	msg_init(&info.task.msg);
	msg_copy(&info.task.msg, &msg);
	switch(msg.message){
		case MSG_MANAGER_SIGINT:
		case MSG_ALIYUN_SIGINT:
        case MSG_GOKE_SIGINT:
        case MSG_DEVICE_SIGINT:
		case MSG_VIDEO_SIGINT:
		case MSG_AUDIO_SIGINT:
		case MSG_RECORDER_SIGINT:
		case MSG_PLAYER_SIGINT:
			if( info.task.func != task_exit) {
				info.task.func = task_exit;
				info.status = EXIT_INIT;
				manager_config.running_mode = RUNNING_MODE_EXIT;
			}
			else {
                char *fp = global_common_get_server_names( info.thread_start );
                log_goke(DEBUG_INFO, "already inside the manager exit task!,current running server %s", fp);
                if( fp ) {
                    free(fp);
                }
			}
			break;
		case MSG_MANAGER_EXIT_ACK:
			misc_clear_bit(&info.thread_start, msg.sender);
			if( manager_config.running_mode == RUNNING_MODE_NORMAL ){
				if( manager_config.fail_restart ) {
					/********message body********/
					msg_init(&send_msg);
					send_msg.message = MSG_MANAGER_TIMER_ADD;
					send_msg.sender = SERVER_MANAGER;
					send_msg.arg_in.cat = manager_config.fail_restart_interval * 1000;
					send_msg.arg_in.dog = 0;
					send_msg.arg_in.duck = 1;
					send_msg.arg_in.handler = manager_delayed_start;
					send_msg.arg_pass.cat = msg.sender;
					global_common_send_message(SERVER_MANAGER, &send_msg);
					/*****************************/
				}
                char *fp = global_common_get_server_names( info.thread_start );
                log_goke(DEBUG_INFO, "%s quitted! remaining server: %s",
                         global_common_get_server_name(msg.sender),
                         fp);
                if( fp ) {
                    free(fp);
                }
			}
			//to do: other mode
			break;
		case MSG_MANAGER_TIMER_ADD:
			if( timer_add(msg.arg_in.handler, msg.arg_in.cat, msg.arg_in.dog, msg.arg_in.duck, msg.sender, msg.arg_pass) )
				log_goke(DEBUG_WARNING, "add timer failed!");
			break;
		case MSG_MANAGER_TIMER_ON:
			( *( void(*)(message_arg_t*) ) msg.arg_in.handler )(&(msg.arg_pass));
			break;
		case MSG_MANAGER_TIMER_REMOVE:
			if( timer_remove(msg.arg_in.handler) ) {
				log_goke(DEBUG_WARNING, "remove timer failed!");
			}
			break;
		case MSG_MANAGER_HEARTBEAT:
			log_goke(DEBUG_VERBOSE, "---heartbeat---at:%d",time_get_now_stamp());
			log_goke(DEBUG_VERBOSE, "---from: %d---status: %d---thread: %d---init: %d", msg.sender, msg.arg_in.cat, msg.arg_in.dog, msg.arg_in.duck);
			break;
		case MSG_MANAGER_PROPERTY_GET:
			manager_get_property(&msg);
			break;
		case MSG_MANAGER_PROPERTY_SET:
			manager_set_property(&msg);
			break;
		case MSG_MANAGER_DUMMY:
			break;
		default:
			log_goke(DEBUG_SERIOUS, "not processed message = %x", msg.message);
			break;
	}
	msg_free(&msg);
	return ret;
}

/*
 * state machine
 */
static void task_sleep(void)
{
	message_t msg;
	int i;
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			for(i=0;i<SERVER_BUTT;i++) {
				if( misc_get_bit( info.thread_start, i)) {
					if(misc_get_bit( manager_config.server_sleep, i)) {
						if( (i != SERVER_ALIYUN) )
							manager_server_stop(i);
					}
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			if( info.thread_start == ((~manager_config.server_sleep) & 0x7FFF)) {
				info.status = STATUS_START;
				log_goke(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", info.task.msg.sender, info.thread_start);
			}
			else {
				if( info.task.msg.message == MSG_MANAGER_EXIT_ACK) {
					msg_init(&msg);
					msg.message = MSG_MANAGER_EXIT_ACK;
					msg.sender = msg.receiver = info.task.msg.sender;
					for(i=0;i<SERVER_BUTT;i++) {
						if( misc_get_bit( info.thread_start, i) ) {
							global_common_send_message(i, &msg);
						}
					}
					log_goke(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", msg.sender, info.thread_start);
				}
			}
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		default:
			log_goke(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_default = %d", info.status);
			break;
	}
}

static void task_deep_sleep(void)
{
	message_t msg;
	int i;
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			for(i=0;i<SERVER_BUTT;i++) {
				if( misc_get_bit( info.thread_start, i) ) {
					manager_server_stop(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			if( info.thread_start == 0 ) {
				info.status = STATUS_START;
				log_goke(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", info.task.msg.sender, info.thread_start);
			}
			else {
				if( info.task.msg.message == MSG_MANAGER_EXIT_ACK) {
					msg_init(&msg);
					msg.message = MSG_MANAGER_EXIT_ACK;
					msg.sender = msg.receiver = info.task.msg.sender;
					for(i=0;i<SERVER_BUTT;i++) {
						if( misc_get_bit( info.thread_start, i) ) {
							global_common_send_message(i, &msg);
						}
					}
					log_goke(DEBUG_INFO, "sleeping process quiter is %d and the after status = %x", msg.sender, info.thread_start);
				}
			}
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		default:
			log_goke(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_default = %d", info.status);
			break;
	}
}

static void task_default(void)
{
	int i;
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			for(i=0;i<SERVER_BUTT;i++) {
				if( misc_get_bit( manager_config.server_start, i) ) {
					if( misc_get_bit( info.thread_start, i) != 1 )
						manager_server_start(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			info.status = STATUS_START;
			break;
		case STATUS_START:
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		case STATUS_STOP:
			break;
		case STATUS_RESTART:
			break;
		case STATUS_ERROR:
			info.task.func = task_exit;
			info.status = EXIT_INIT;
			manager_config.running_mode = RUNNING_MODE_EXIT;
			break;
		default:
			log_goke(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_default = %d", info.status);
			break;
	}
}

static void task_testing(void)
{
	int i;
	message_t	msg;
	switch( info.status )
	{
		case STATUS_NONE:
			info.status = STATUS_WAIT;
			break;
		case STATUS_WAIT:
			info.status = STATUS_SETUP;
			break;
		case STATUS_SETUP:
			for(i=0;i<SERVER_BUTT;i++) {
				if( misc_get_bit( manager_config.server_start, i) ) {
					if( misc_get_bit( info.thread_start, i) != 1 )
						manager_server_start(i);
				}
			}
			info.status = STATUS_IDLE;
			break;
		case STATUS_IDLE:
			info.status = STATUS_START;
			break;
		case STATUS_START:
			/********message body********/
			msg_init(&msg);
			msg.message = MSG_MANAGER_TIMER_ADD;
			msg.sender = SERVER_MANAGER;
			msg.arg_in.cat = 1000*20;
			msg.arg_in.dog = 0;
			msg.arg_in.duck = 1;
			msg.arg_in.handler = &test_func;
			global_common_send_message(SERVER_MANAGER, &msg);
			info.status = STATUS_RUN;
			break;
		case STATUS_RUN:
			break;
		case STATUS_STOP:
			for(i=0;i<SERVER_BUTT;i++) {
				if( misc_get_bit( info.thread_start, i)) {
					if(misc_get_bit( manager_config.server_sleep, i)) {
						manager_server_stop(i);
					}
				}
			}
			info.status = STATUS_RESTART;
			break;
		case STATUS_RESTART:
			if( info.thread_start == ((~manager_config.server_sleep) & 0x7FFF)) {
				info.status = STATUS_NONE;
				log_goke(DEBUG_INFO, "tesing process quiter is %d and the after status = %x", info.task.msg.sender, info.thread_start);
			}
			else {
				if( info.task.msg.message == MSG_MANAGER_EXIT_ACK) {
					msg_init(&msg);
					msg.message = MSG_MANAGER_EXIT_ACK;
					msg.sender = msg.receiver = info.task.msg.sender;
					for(i=0;i<SERVER_BUTT;i++) {
						if( misc_get_bit( info.thread_start, i) ) {
							global_common_send_message(i, &msg);
						}
					}
					log_goke(DEBUG_INFO, "tesing process quiter is %d and the after status = %x", msg.sender, info.thread_start);
				}
			}
			break;
		case STATUS_ERROR:
			info.task.func = task_exit;
			info.status = EXIT_INIT;
			manager_config.running_mode = RUNNING_MODE_EXIT;
			break;
		default:
			log_goke(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_testing = %d", info.status);
			break;
	}
}

/****************************/
/*
 * exit: *->exit
 */
static void task_exit(void)
{
	message_t msg;
	int i;
	switch( info.status ){
		case EXIT_INIT:
			log_goke(DEBUG_INFO,"MANAGER: switch to exit task!");
			msg_init(&msg);
			msg.message = MSG_MANAGER_EXIT;
			msg.sender = msg.receiver = SERVER_MANAGER;
			msg.arg_in.cat = info.thread_start;
			for(i=0;i<SERVER_BUTT;i++) {
				if( misc_get_bit( info.thread_start, i) ) {
					global_common_send_message(i, &msg);
				}
			}
            char *fp = global_common_get_server_names( info.thread_start );
            log_goke(DEBUG_INFO, "sigint request get, sender=%s, exit code = %s",
                     global_common_get_server_name(info.task.msg.sender),
                     fp);
            if( fp ) {
                free(fp);
            }
			if( info.status2==0 ) {
				/********message body********/
				msg_init(&msg);
				msg.message = MSG_MANAGER_TIMER_ADD;
				msg.sender = SERVER_MANAGER;
				msg.arg_in.cat = 10000;
				msg.arg_in.dog = 0;
				msg.arg_in.duck = 1;
				msg.arg_in.handler = &manager_kill_all;
//				global_common_send_message(SERVER_MANAGER, &msg);
			}
			/****************************/
			info.status = EXIT_SERVER;
			break;
		case EXIT_SERVER:
			if( info.thread_start == 0 ) {	//quit all
                char *fp = global_common_get_server_names( info.thread_start );
                log_goke(DEBUG_INFO, "termination process quiter is %s and the after status = %s",
                         global_common_get_server_name(info.task.msg.sender),
                         fp);
                if( fp ) {
                    free(fp);
                }
				info.status = EXIT_STAGE1;
			}
			else {
				if( info.task.msg.message == MSG_MANAGER_EXIT_ACK) {
					msg_init(&msg);
					msg.message = MSG_MANAGER_EXIT_ACK;
					msg.sender = msg.receiver = info.task.msg.sender;
					for(i=0;i<SERVER_BUTT;i++) {
						if( misc_get_bit( info.thread_start, i) ) {
							global_common_send_message(i, &msg);
						}
					}
                    char *fp = global_common_get_server_names( info.thread_start );
                    log_goke(DEBUG_INFO, "termination process quiter is %s and the after status = %s",
                             global_common_get_server_name(msg.sender),
                             fp);
                    if( fp ) {
                        free(fp);
                    }
				}
			}
			break;
		case EXIT_STAGE1:
			server_release_1();
			info.status = EXIT_THREAD;
			break;
		case EXIT_THREAD:
			info.thread_exit = info.thread_start;
			manager_broadcast_thread_exit();
			if( !info.thread_start )
				info.status = EXIT_STAGE2;
			break;
			break;
		case EXIT_STAGE2:
			server_release_2();
			info.status = EXIT_FINISH;
			break;
		case EXIT_FINISH:
			if( info.status2 ) {
				info.status2 = 0;
				info.task.func = task_default;
				manager_config.running_mode = RUNNING_MODE_NORMAL;
				info.msg_lock = 0;
			}
			else {
				info.exit = 1;
			}
			info.status = STATUS_NONE;
			break;
		default:
			log_goke(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in task_exit = %d", info.status);
			break;
		}
	return;
}

static void main_thread_termination(void)
{
    message_t msg;
    /********message body********/
    msg_init(&msg);
    msg.message = MSG_MANAGER_SIGINT;
	msg.sender = msg.receiver = SERVER_MANAGER;
    /****************************/
    global_common_send_message(SERVER_MANAGER, &msg);
}


/*
 * internal interface
 */
void manager_deep_sleep(void)
{
	info.status = STATUS_NONE;
	info.task.func = task_deep_sleep;
    manager_config.running_mode = RUNNING_MODE_DEEP_SLEEP;
    manager_config.sleep.enable = 2;
}

void manager_wakeup(void)
{
	info.status = STATUS_NONE;
	info.task.func = task_testing;
	manager_send_wakeup(SERVER_ALIYUN);
}


/*
 * external interface
 */
int manager_init(void)
{
	int ret = 0;
	pthread_t	id;
    signal(SIGINT, main_thread_termination);
    signal(SIGTERM, main_thread_termination);
    _universal_tag_  = time_get_now_ms();
	timer_init();
    memset(&info, 0, sizeof(server_info_t));
	ret = pthread_create(&id, NULL, manager_timer_func, NULL);
	if(ret != 0) {
		log_goke(DEBUG_SERIOUS, "timer create error! ret = %d",ret);
		 return ret;
	 }
	else {
		log_goke(DEBUG_INFO, "timer create successful!");
	}
	msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_normal, &mutex);
	info.init = 1;
	//sleep timer
	sleep_init(manager_config.sleep.enable, manager_config.sleep.start, manager_config.sleep.stop);
	manager_routine_init();
	//default task
	if( manager_config.running_mode == RUNNING_MODE_SLEEP ) {
		info.task.func = task_sleep;
	}
	else if( manager_config.running_mode == RUNNING_MODE_NORMAL ) {
		info.task.func = task_default;
	}
	else if( manager_config.running_mode == RUNNING_MODE_TESTING ) {
		info.task.func = task_testing;
	}
}

int manager_proc(void)
{
	if ( !info.exit ) {
		info.old_status = info.status;
		info.task.func();
		server_message_proc();
	}
	else {
		server_release_3();
		log_goke(DEBUG_INFO, "-----------main exit-----------");
		exit(0);
	}
}

int manager_message(message_t *msg)
{
	int ret=0;
	pthread_mutex_lock(&mutex);
	ret = msg_buffer_push(&message, msg);
    log_goke(DEBUG_VERBOSE, "MANAGER message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret,message.head, message.tail);
	if( ret!=0 )
		log_goke(DEBUG_WARNING, "MANAGER message push in error =%d", ret);
	else {
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutex);
	return ret;
}
