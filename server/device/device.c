/*
 * header
 */
//system header
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/statfs.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <linux/netlink.h>
#include <linux/input.h>
#include<sys/reboot.h>

//program header
#include "../../common/tools_interface.h"
#include "../../server/manager/manager_interface.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/video/video_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../server/aliyun/aliyun_interface.h"
#include "../../server/aliyun/aliyun.h"

//server header
#include "device.h"
#include "config.h"
#include "device_interface.h"
#include "file_manager.h"
#include "sd_control.h"
#include "gk_gpio.h"
#include "../../common/ntpc.h"

/*
 * static
 */
//variable
static int led1_sleep_t[3] = {1, 100000, 200000}; //us
static int led2_sleep_t[3] = {2, 200000, 800000}; //us
static int daynight_mode_func_exit = 0;
static int led_flash_func_exit = 0;
static int storage_detect_func_lock = 0;
static int daynight_mode_func_lock = 0;
static int led_flash_func_lock = 0;
static int sd_card_insert;
static int motor_reset_thread_flag = 0;
static int umount_server_flag = 0;
static int umount_flag = 0;
static int step_motor_init_flag = 0;
static int motor_check_flag = 0;
static device_config_t device_config_;
static server_info_t info;
static message_buffer_t message;
static message_t rev_msg_tmp;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static  pthread_rwlock_t	ilock = PTHREAD_RWLOCK_INITIALIZER;

//function
//common
static void *server_func(void);

static int server_message_proc(void);

static int server_none(void);

static int server_wait(void);

static int server_setup(void);

static int server_idle(void);

static int server_start(void);

static int server_run(void);

static int server_stop(void);

static int server_restart(void);

static int server_error(void);

static int server_release(void);

static void server_thread_termination(void);


/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * thread funcs
 */
static int *sd_func(void *arg) {
    int ret;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    misc_set_thread_name("server_device_sd");
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit(&info.thread_start, DEVICE_THREAD_SD);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_DEVICE);
    log_goke(DEBUG_INFO, "-----------thread start: server_sd-----------");
    while (1) {
        //status check
        pthread_rwlock_rdlock(&ilock);
        if (info.exit ||
            misc_get_bit(info.thread_exit, DEVICE_THREAD_SD)) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        pthread_rwlock_unlock(&ilock);
        if( sd_proc(&ilock) )
            break;
    }
    //release
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, DEVICE_THREAD_SD);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_DEVICE);
    log_goke(DEBUG_INFO, "-----------thread exit: server_sd-----------");
    pthread_exit(0);
}

static int *hotplug_func(void *arg) {
    char buf[4096] = {0};
    char address[32] = {0};
    int ret;
    int i;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    misc_set_thread_name("server_device_hotplug");
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit(&info.thread_start, DEVICE_THREAD_HOTPLUG);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_DEVICE);
    log_goke(DEBUG_INFO, "-----------thread start: server_hotplug-----------");
    //init socket
    const int buffersize = 1024;
    struct sockaddr_nl snl;

    bzero(&snl, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;

    int socket_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (socket_fd == -1) {
        perror("socket");
        goto REALEASE;
    }

    struct timeval tv_timeout;
    tv_timeout.tv_sec = 0;          //0s
    tv_timeout.tv_usec = 20*1000;   //20ms
    setsockopt(socket_fd,SOL_SOCKET,SO_RCVTIMEO,&tv_timeout, sizeof(tv_timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));

    ret = bind(socket_fd, (struct sockaddr *)&snl, sizeof(struct sockaddr_nl));
    if (ret < 0) {
        perror("bind");
        close(socket_fd);
        goto REALEASE;
    }

    while (1) {
        //status check
        pthread_rwlock_rdlock(&ilock);
        if (info.exit ||
            misc_get_bit(info.thread_exit, DEVICE_THREAD_HOTPLUG)) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        pthread_rwlock_unlock(&ilock);
        memset(buf, 0, sizeof(buf));
        ret = recv(socket_fd, &buf, sizeof(buf), 0);
        if( ret > 0 ) {
            for(i=0;i<ret;i++) {
                if (*(buf + i) == '\0') {
                    buf[i] = '\n';
                }
            }
            log_goke(DEBUG_INFO, "received %d bytes\n%s\n", ret, buf);
            //looking for sd hotplug info
            char *pos = NULL;
            pos = strstr(buf, SD_PLUG_ADD_CMD);
            if( pos ) { //found mmc insert action
                pos += strlen( SD_PLUG_ADD_CMD);
                char *org = pos;
                int num = 0;
                while( (*pos!='\0') && (*pos!='\n') && (*pos!='/') ) {
                    pos++;
                    num++;
                }
                if( num> 0 ) {
                    memset(address, 0 ,sizeof(address));
                    memcpy(address, org, num);
                    update_hotplug_event(1, address);
                    log_goke(DEBUG_INFO, "found sd insert address %s", address);
                }
            }
            else {
                pos = NULL;
                pos = strstr(buf, SD_PLUG_REMOVE_CMD);
                if( pos ) {
                    update_hotplug_event(0, 0);
                }
            }
        }
    }
    close(socket_fd);
    REALEASE:
    //release
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, DEVICE_THREAD_HOTPLUG);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_DEVICE);
    log_goke(DEBUG_INFO, "-----------thread exit: server_hotplug-----------");
    pthread_exit(0);
}

/*
 * helper
 */
static void device_check_reset(void) {
	static int iCounts = 0,iCounts2 = 0,iUptime = 0;
    int ret = 0,iValue = 0;
	char cmd[128];
	FILE *fp = NULL,*fpcmd = NULL;
	if(iUptime <= 30000)
	{
		iUptime = time_getuptime_ms();
	}
	if((++iCounts2 > 20) && (iUptime > 30000))
	{//每10s操作一次
    #ifdef RELEASE_VERSION
		memset(cmd,0,sizeof(cmd));
		snprintf(cmd,sizeof(cmd),"/bin/echo 3 > /proc/sys/vm/drop_caches");
		fpcmd = popen(cmd,"r");
		if(fpcmd)
		{
			pclose(fpcmd);
		}
    #endif
		iCounts2 = 0;
	}

    ret = gk_gpio_getvalue(GK_GPIO_RESET,&iValue);
	if(ret == 0)
	{
		if(iValue == 0)
		{
			iCounts++;
			log_goke(DEBUG_SERIOUS,"Reset iCounts:%d",iCounts);
			if(iCounts >= 10)
			{
				memset(cmd,0,sizeof(cmd));
				snprintf(cmd,sizeof(cmd),"/bin/rm -f %s",WIFI_INFO_CONF);
				fp = popen(cmd,"r");
				if(fp)
				{
					pclose(fp);
				}
				
				memset(cmd,0,sizeof(cmd));
				snprintf(cmd,sizeof(cmd),"/bin/rm -f %s",WPA_SUPPLICANT_CONF_NAME);
				fp = popen(cmd,"r");
				if(fp)
				{
					pclose(fp);
				}
				
				memset(cmd,0,sizeof(cmd));
				snprintf(cmd,sizeof(cmd),"/bin/cp -f %s %s",WPA_SUPPLICANT_CONF_BAK_NAME,WPA_SUPPLICANT_CONF_NAME);
				fp = popen(cmd,"r");
				if(fp)
				{
					pclose(fp);
				}
				memset(cmd,0,sizeof(cmd));
				snprintf(cmd,sizeof(cmd),"/bin/sync");
				fp = popen(cmd,"r");
				if(fp)
				{
					pclose(fp);
				}
				usleep(200000);
				reboot(0x1234567);
			}
		}
		else
		{
			iCounts = 0;
		}
	}
}

static void server_thread_termination(void) {
    message_t msg;
    memset(&msg, 0, sizeof(message_t));
    msg.message = MSG_DEVICE_SIGINT;

    manager_message(&msg);
}

static int server_release(void) {
    while (storage_detect_func_lock || led_flash_func_lock || daynight_mode_func_lock) {
//		log_goke(DEBUG_SERIOUS, "server_release device ---- ");
        usleep(50 * 1000); //50ms
    }
    memset(&info, 0, sizeof(server_info_t));
    return 0;
}

static int device_message_filter(message_t  *msg)
{
    int ret = 0;
    if( info.status >= STATUS_ERROR ) { //only system message
        if( !msg_is_system(msg->message) && !msg_is_response(msg->message) )
            return 1;
    }
    return ret;
}

static int server_message_proc(void) {
    int ret = 0;
    message_t msg, send_msg;
    //condition
    pthread_mutex_lock(&mutex);
    if( message.head == message.tail ) {
        if( info.status == info.old_status	) {
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
    if( ret == 1)
        return 0;
    if( device_message_filter(&msg) ) {
        log_goke(DEBUG_VERBOSE, "DEVICE message filtered: sender=%s, message=%s, head=%d, tail=%d was screened",
                 global_common_get_server_name(msg.sender),
                 global_common_message_to_string(msg.message), message.head, message.tail);
        msg_free(&msg);
        return -1;
    }
	if(MSG_MANAGER_TIMER_ON != msg.message)
    log_goke(DEBUG_VERBOSE, "DEVICE message popped: sender=%s, message=%s, head=%d, tail=%d",
             global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
    switch (msg.message) {
        case MSG_MANAGER_EXIT:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case MSG_DEVICE_GET_PARA:
            /********message body********/
            msg_init(&send_msg);
            send_msg.message = msg.message | 0x1000;
            send_msg.sender = send_msg.receiver = SERVER_DEVICE;
            send_msg.arg_in.cat = msg.arg_in.cat;
            send_msg.result = 0;
            if (msg.arg_in.cat == DEVICE_PARAM_SD_INFO) {
                int st = sd_get_status();
                log_goke(DEBUG_INFO, "DEVICE: check sd status = %d", st);
                if( sd_get_file_status() &&  st == SD_ALIYUN_STATUS_NORMAL) {
                    send_msg.arg_in.dog = 1;
                }
            }
            ret = global_common_send_message(msg.receiver, &send_msg);
            break;
        case MSG_MANAGER_TIMER_ON:
            ((HANDLER) msg.arg_in.handler)();
            break;
        case MSG_MANAGER_EXIT_ACK:
            misc_clear_bit(&info.quit_status, msg.sender);
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "DEVICE: server %s quit and quit lable now = %s",
                     global_common_get_server_name(msg.sender),
                     fp);
            if(fp) {
                free(fp);
            }
            break;
        default:
            log_goke(DEBUG_SERIOUS, "DEVICE not support message %s",
                     global_common_message_to_string(msg.message));
            break;
    }
    msg_free(&msg);
    return ret;
}

/*
 * state machine
 */
static void server_release_1(void)
{
}

static void server_release_2(void)
{
    msg_buffer_release2(&message, &mutex);
}

static void server_release_3(void)
{
    msg_free(&info.task.msg);
    memset(&info, 0, sizeof(server_info_t));
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_MANAGER_EXIT_ACK;
    msg.sender = SERVER_DEVICE;
    global_common_send_message(SERVER_MANAGER, &msg);
    /***************************/
}

static int server_init(void)
{
    int ret = 0;
    if( misc_full_bit( info.init_status, DEVICE_INIT_CONDITION_NUM ) )
        info.status = STATUS_WAIT;
    return ret;
}

static int server_setup(void) {
    int ret = 0;
    message_t msg;
    pthread_t pid;
    //start sd thread
    ret = pthread_create(&pid, NULL, sd_func, 0);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "sd thread create error! ret = %d", ret);
        return ret;
    }
    //start hotplug thread
    ret = pthread_create(&pid, NULL, hotplug_func, 0);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "hotplug thread create error! ret = %d", ret);
        return ret;
    }

	gk_gpio_init();
    //ircut
    if(_config_.day_night_mode == 0) {//0:白天
        gk_ircut_daymode();
    }
    else if(_config_.day_night_mode == 1) {//1:夜市
        gk_ircut_nightmode();
    }
    else {//2:自动

    }
    usleep(200*1000);
    gk_ircut_release();
    //speaker
    if(_config_.speaker_enable ) {
        gk_enable_speaker();
    } else {
        gk_disable_speaker();
    }
    //led
    if(_config_.led_enable) {
        gk_led1_on();   //red
        gk_led2_off();  //green
    }
	msg_init(&msg);
    msg.message = MSG_MANAGER_TIMER_ADD;
    msg.sender = SERVER_DEVICE;
    msg.arg_in.cat = 500;   //500ms
    msg.arg_in.dog = 0;
    msg.arg_in.duck = 0;
    msg.arg_in.handler = &device_check_reset;
    global_common_send_message(SERVER_MANAGER, &msg);

    /****************************/
    info.status = STATUS_IDLE;
    return ret;
}

static int server_start(void) {
    int ret = 0;
    if (ret) {
        log_goke(DEBUG_SERIOUS, "adjust_input_audio_volume failed");
        goto RESTART;
    }

    RESTART:
    if (ret)
        info.status = STATUS_RESTART;
    else
        info.status = STATUS_RUN;
    return ret;
}



static int server_stop(void) {
    int ret = 0;
    return ret;
}

static int server_restart(void) {
    int ret = 0;
    info.exit = 1;
    server_release();
    info.status = STATUS_NONE;
    return ret;
}

/*
 * task proc
 */
static void task_proc(void) {
    if( info.status == STATUS_NONE ) {
        info.status = STATUS_INIT;
    }
    else if( info.status == STATUS_WAIT ) {
        info.status = STATUS_SETUP;
    }
    else if( info.status == STATUS_IDLE ) {
        info.status = STATUS_START;
    }
    return;
}

/*
 * state machine
 */
static void state_machine(void) {
    switch (info.status) {
        case STATUS_NONE:
        case STATUS_WAIT:
        case STATUS_IDLE:
        case STATUS_RUN:
            break;
        case STATUS_INIT:
            server_init();
            break;
        case STATUS_SETUP:
            server_setup();
            break;
        case STATUS_START:
            server_start();
            break;
        case STATUS_STOP:
            server_stop();
            break;
        case STATUS_RESTART:
            server_restart();
            break;
        case STATUS_ERROR:
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case EXIT_INIT:
            log_goke(DEBUG_INFO, "DEVICE: switch to exit task!");
            if (info.task.msg.sender == SERVER_MANAGER) {
                info.quit_status &= (info.task.msg.arg_in.cat);
            } else {
                info.quit_status = 0;
            }
            char *fp = global_common_get_server_names( info.quit_status );
            log_goke(DEBUG_INFO, "DEVICE: Exit precondition =%s", fp);
            if( fp ) {
                free(fp);
            }
            info.status = EXIT_SERVER;
            break;
        case EXIT_SERVER:
            if (!info.quit_status)
                info.status = EXIT_STAGE1;
            break;
        case EXIT_STAGE1:
            server_release_1();
            info.status = EXIT_THREAD;
            break;
        case EXIT_THREAD:
            info.thread_exit = info.thread_start;
            if (!info.thread_start)
                info.status = EXIT_STAGE2;
            break;
        case EXIT_STAGE2:
            server_release_2();
            info.status = EXIT_FINISH;
            break;
        case EXIT_FINISH:
            info.exit = 1;
            info.status = STATUS_NONE;
            break;
        default:
            log_goke(DEBUG_SERIOUS, "DEVICE !!!!!!!unprocessed server status in state_machine = %d", info.status);
            break;
    }
    return;
}

/*
 * server entry point
 */
static void *server_func(void) {
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    misc_set_thread_name("server_device");
    pthread_detach(pthread_self());
    msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_large, &mutex);
    info.quit_status = DEVICE_EXIT_CONDITION;
    info.init = 1;
    while (!info.exit) {
        info.old_status = info.status;
        state_machine();
        task_proc();
        server_message_proc();
    }
    server_release_3();
    log_goke(DEBUG_INFO, "-----------thread exit: server_device-----------");
    pthread_exit(0);
}

/*
 * external interface
 */
int server_device_start(void) {
    int ret = -1;
    ret = pthread_create(&info.id, NULL, server_func, NULL);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "device server create error! ret = %d", ret);
        return ret;
    } else {
        log_goke(DEBUG_SERIOUS, "device server create successful!");
        return 0;
    }
}

int server_device_message(message_t *msg) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    if (!message.init) {
        log_goke(DEBUG_VERBOSE, "DEVICE server is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message) );
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    ret = msg_buffer_push(&message, msg);
	if(MSG_MANAGER_TIMER_ON != msg->message)
    log_goke(DEBUG_VERBOSE, "DEVICE message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret,message.head, message.tail);
    if (ret != 0)
        log_goke(DEBUG_WARNING, "DEVICE message push in error =%d", ret);
    else {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}
