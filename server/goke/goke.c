/*
 * header
 */
//system header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <malloc.h>
#include <sys/select.h>
#include <sys/prctl.h>
//program header
#include "../../server/manager/manager_interface.h"
#include "../../common/tools_interface.h"
#include "../../server/aliyun/aliyun_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../server/video/config.h"
//server header
#include "hisi/hisi.h"
#include "goke.h"
#include "goke_interface.h"


/*
 * static
 */
//variable
static message_buffer_t message;
static server_info_t info;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//function
//common
static void *server_func(void);

static int server_message_proc(void);

static void state_machine(void);

static void task_proc(void);

static void server_release_1(void);

static void server_release_2(void);

static void server_release_3(void);

static int server_init(void);

static int server_setup(void);

static int server_stop(void);

static int server_restart(void);

static void server_thread_termination(int sign);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */
static int goke_adjust_video_parameters(video_config_t *config) {
    int i, max;
    SIZE_S pstSize;
    int vpss;
    config->vpss.group_info.u32MaxW = 0;
    config->vpss.group_info.u32MaxH = 0;
    max = 0;
    for (i = MAX_STREAM_NUM - 1; i >= 0 ; i--) {
        if( config->profile.stream[i].enable) {
            if (!hisi_get_pic_size(config->profile.stream[i].size, &pstSize)) {
                config->profile.stream[i].width = pstSize.u32Width;
                config->profile.stream[i].height = pstSize.u32Height;
                if( pstSize.u32Width > config->vpss.group_info.u32MaxW ) {
                    config->vpss.group_info.u32MaxW = pstSize.u32Width;
                    config->vpss.group_info.u32MaxH = pstSize.u32Height;
                }
            }
            vpss = config->profile.stream[i].vpss_chn;
            if( config->profile.stream[i].frame_rate > max ) {
                max = config->profile.stream[i].frame_rate;
            }
            config->vpss.channel_info[vpss].u32Width = config->profile.stream[i].width;
            config->vpss.channel_info[vpss].u32Height = config->profile.stream[i].height;
            config->vpss.enabled[vpss] = 1;
            config->vpss.channel_info[vpss].bMirror = _config_.video_mirror;
        }
    }
    ISP_PUB_ATTR_GC2063.f32FrameRate = max;
    if( config->vpss.group_info.u32MaxW == 0 || config->vpss.group_info.u32MaxW == 0 ) {
        return -1;
    }
    return 0;
}

static void server_thread_termination(int sign) {
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_GOKE_SIGINT;
    msg.sender = msg.receiver = SERVER_GOKE;
    global_common_send_message(SERVER_MANAGER, &msg);
    /****************************/
}

static void server_release_1(void) {
    if (info.status2) {
        hisi_uninit_sys();
        info.status2 = 0;
    }
}

static void server_release_2(void) {
    msg_buffer_release2(&message, &mutex);
}

static void server_release_3(void) {
    msg_free(&info.task.msg);
    memset(&info, 0, sizeof(server_info_t));
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_MANAGER_EXIT_ACK;
    msg.sender = SERVER_GOKE;
    global_common_send_message(SERVER_MANAGER, &msg);
    /***************************/
}

static int server_init(void) {
    int ret = 0;
    if (misc_full_bit(info.init_status, GOKE_INIT_CONDITION_NUM))
        info.status = STATUS_WAIT;
    return ret;
}

static int server_setup(void) {
    int ret = 0;
    message_t msg;
    //init av system
    if( info.status2==0 ) {
        ret = hisi_register_vqe_module();
        if( ret ) {
            log_goke(DEBUG_WARNING,"%s: ai init vqe handle error with %x!", __FUNCTION__, ret);
            return -1;
        }
        ret = goke_adjust_video_parameters(&video_config);
        if (ret) {
            log_goke(DEBUG_SERIOUS, "goke parameter init fail");
            info.status = STATUS_ERROR;
            return -1;
        }
        ret = hisi_init_sys(&video_config);
        if (ret) {
            log_goke(DEBUG_SERIOUS, "av init fail");
            info.status = STATUS_ERROR;
            return -1;
        }
        info.status2 = 1;
    }
    info.status = STATUS_IDLE;
    return ret;
}

static int server_start(void) {
    info.status = STATUS_RUN;
    return 0;
}

static int server_stop(void) {
    int ret = 0;
    info.status = STATUS_IDLE;
    return ret;
}

static int server_restart(void) {
    int ret = 0;
    if (info.status2) {
        hisi_uninit_sys();
        info.status2 = 0;
    }
    info.status = STATUS_WAIT;
    return ret;
}

/*
 *
 *
 * goke server function
 *
 *
 */
static int goke_message_filter(message_t *msg) {
    int ret = 0;
    if (info.status >= STATUS_ERROR) { //only system message
        if (!msg_is_system(msg->message) && !msg_is_response(msg->message))
            return 1;
    }
    return ret;
}


/*
 *
 * message proc
 *
 *
 */
static int server_message_proc(void) {
    int ret = 0;
    message_t msg, send_msg;
    //condition
    pthread_mutex_lock(&mutex);
    if (message.head == message.tail) {
        if (info.status == info.old_status) {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    if (info.msg_lock) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    msg_init(&msg);
    ret = msg_buffer_pop(&message, &msg);
    pthread_mutex_unlock(&mutex);
    if (ret == 1)
        return 0;
    if( goke_message_filter(&msg) ) {
        log_goke(DEBUG_MAX, "GOKE message filtered: sender=%s, message=%s, head=%d, tail=%d was screened",
                 global_common_get_server_name(msg.sender),
                 global_common_message_to_string(msg.message), message.head, message.tail);
        msg_free(&msg);
        return -1;
    }
    log_goke(DEBUG_MAX, "GOKE message popped: sender=%s, message=%s, head=%d, tail=%d",
             global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
    switch (msg.message) {
        case MSG_MANAGER_EXIT:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case MSG_MANAGER_TIMER_ON:
            ((HANDLER) msg.arg_in.handler)();
            break;
        case MSG_GOKE_PROPERTY_GET:
            /********message body********/
            msg_init(&send_msg);
            send_msg.message = msg.message | 0x1000;
            send_msg.sender = send_msg.receiver = SERVER_GOKE;
            send_msg.arg_in.cat = msg.arg_in.cat;
            send_msg.result = 0;
            if (msg.arg_in.cat = GOKE_PROPERTY_STATUS) {
                send_msg.arg_in.dog = info.status;
            }
            ret = global_common_send_message(msg.receiver, &send_msg);
            /***************************/
            break;
        case MSG_MANAGER_EXIT_ACK:
            misc_clear_bit(&info.quit_status, msg.sender);
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "GOKE: server %s quit and quit lable now = %s",
                     global_common_get_server_name(msg.sender),
                     fp);
            if(fp) {
                free(fp);
            }
            break;
        case MSG_MANAGER_DUMMY:
            break;
        default:
            log_goke(DEBUG_SERIOUS, "GOKE not support message %s",
                     global_common_message_to_string(msg.message));
            break;
    }
    msg_free(&msg);
    return ret;
}

/*
 * task proc
 */
static void task_proc(void) {
    message_t msg;
    if( info.status == STATUS_NONE ) {
        info.status = STATUS_INIT;
    }
    else if( info.status == STATUS_WAIT ) {
        info.status = STATUS_SETUP;
    }
    else if( info.status == STATUS_IDLE ) {
        info.status = STATUS_START;
    }
    else if (info.status == STATUS_RUN) {
        //*due to message lock, this message won't be send indefinitely*//
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_GOKE_PROPERTY_NOTIFY;
        msg.sender = msg.receiver = SERVER_GOKE;
        msg.arg_in.cat = GOKE_PROPERTY_STATUS;
        msg.arg_in.dog = STATUS_RUN;
        global_common_send_message(SERVER_VIDEO, &msg);
        global_common_send_message(SERVER_AUDIO, &msg);
        /****************************/
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
            log_goke(DEBUG_INFO, "GOKE: switch to exit task!");
            if (info.task.msg.sender == SERVER_MANAGER) {
                info.quit_status &= (info.task.msg.arg_in.cat);
            } else {
                info.quit_status = 0;
            }
            char *fp = global_common_get_server_names( info.quit_status );
            log_goke(DEBUG_INFO, "GOKE: Exit precondition =%s", fp);
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
            log_goke(DEBUG_SERIOUS, "GOKE !!!!!!!unprocessed server status in state_machine = %d", info.status);
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
    misc_set_thread_name("server_goke");
    pthread_detach(pthread_self());
    msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_small, &mutex);
    info.quit_status = GOKE_EXIT_CONDITION;
    info.init = 1;
    while (!info.exit) {
        info.old_status = info.status;
        state_machine();
        task_proc();
        server_message_proc();
    }
    server_release_3();
    log_goke(DEBUG_INFO, "-----------thread exit: server_goke-----------");
    pthread_exit(0);
}

/*
 * internal interface
 */

/*
 * external interface
 */
int server_goke_start(void) {
    int ret = -1;
    ret = pthread_create(&info.id, NULL, server_func, NULL);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "goke server create error! ret = %d", ret);
        return ret;
    } else {
        log_goke(DEBUG_SERIOUS, "goke server create successful!");
        return 0;
    }
}

int server_goke_message(message_t *msg) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    if (!message.init) {
        log_goke(DEBUG_MAX, "GOKE server is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message) );
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    ret = msg_buffer_push(&message, msg);
    log_goke(DEBUG_MAX, "GOKE message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret,message.head, message.tail);
    if (ret != 0)
        log_goke(DEBUG_WARNING, "GOKE message push in error =%d", ret);
    else {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

