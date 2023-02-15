/*
 *	Manager
 *	 Reside in main thread, manage all the servers *
 */

#ifndef SERVER_MANAGER_INTERFACE_H_
#define SERVER_MANAGER_INTERFACE_H_

/*
 * header
 */
#include <pthread.h>
#include "../../global/global_interface.h"
#include "../../common/tools_interface.h"
#include "config.h"

/*
 * define
 */
#define	SERVER_MANAGER_VERSION_STRING		"alpha-8.2"




#define TIMER_NUMBER  		32

/*
 * ------------message------------------
 */
#define		MSG_TYPE_GET					0
#define		MSG_TYPE_SET					1
#define		MSG_TYPE_SET_CARE				2


typedef enum manager_property_t {
    MANAGER_PROPERTY_SLEEP = 0,
    MANAGER_PROPERTY_DEEP_SLEEP_REPEAT,
    MANAGER_PROPERTY_DEEP_SLEEP_WEEKDAY,
    MANAGER_PROPERTY_DEEP_SLEEP_START,
    MANAGER_PROPERTY_DEEP_SLEEP_END,
    MANAGER_PROPERTY_TIMEZONE,
};

typedef void (*HANDLER)(void);
typedef void (*AHANDLER)(void *arg);
/*
 * server status
 */
/*
 *  property type
 */
#define	PROPERTY_TYPE_GET					(1 << 12)
#define	PROPERTY_TYPE_SET					(1 << 13)
#define	PROPERTY_TYPE_NOTIFY				(1 << 14)

/*
 * structure
 */
//sever status for local state machine
typedef enum server_status_t {
    STATUS_NONE = 0,		//none state;
    STATUS_INIT,            //init action;
	STATUS_WAIT,			//wait state;
	STATUS_SETUP,			//setup action;
	STATUS_IDLE,			//idle state;
	STATUS_START,			//start action;
	STATUS_RUN,				//running state;
	STATUS_STOP,			//stop action;
	STATUS_RESTART,			//restart action;
    STATUS_REINIT,          //reinit action
	STATUS_ERROR,			//error action;

	EXIT_INIT = 1000,		//quit start
	EXIT_SERVER,
	EXIT_STAGE1,
	EXIT_THREAD,
	EXIT_STAGE2,
	EXIT_FINISH,
} server_status_t;

typedef enum task_type_enum {
    TASK_TYPE_NONE = 0,
    TASK_TYPE_START,
    TASK_TYPE_STOP,
    TASK_TYPE_RESTART,
    TASK_TYPE_RECORDER_ADD,
    TASK_TYPE_BUTT,
} task_type_enum;

typedef enum task_finish_enum {
    TASK_FINISH_NO = 0,
    TASK_FINISH_FAIL,
    TASK_FINISH_SUCCESS,
} task_finish_enum;

typedef struct task_t {
	HANDLER   		func;
	message_t		msg;
	server_status_t	start;
	server_status_t	end;
    task_type_enum  type;
} task_t;

//server info
typedef struct server_info_t {
	server_status_t		    status;
	server_status_t		    old_status;
	pthread_t			    id;
	int					    init;
	int					    error;
	int					    msg_lock;
	int					    status2;
	int					    exit;
	task_t				    task;
	int					    thread_start;
	int					    thread_exit;
	int					    init_status;
    int                     quit_status;
	unsigned long long int	tick;
} server_info_t;

//server info tag (int only)
typedef enum server_info_tag_e {
    INFO_TAG_STATUS = 0,
    INFO_TAG_STATUS_OLD,
    INFO_TAG_MSG_LOCK,
    INFO_TAG_INIT,
    INFO_TAG_ERROR,
    INFO_TAG_EXIT,
    INFO_TAG_THREAD_START,
    INFO_TAG_THREAD_EXIT,
    INFO_TAG_STATUS2,
    INFO_TAG_INIT_STATUS,
}server_info_tag_e;

typedef struct timer_struct_t
{
    int       		tid;
    int				sender;
    unsigned int 	tick;
    int				delay;
    unsigned int    interval;
    int		      	oneshot;
    void   			*fpcallback;
    message_arg_t	arg;
} timer_struct_t;

/*
 * function
 */
#define log_goke(level, S, ...)	log_new(finename(__FILE__), __LINE__, _config_.debug_level, level, S, ## __VA_ARGS__)

int manager_message(message_t *msg);
int manager_proc(void);
int manager_init(void);

#endif /* MANAGER_MANAGER_INTERFACE_H_ */
