#ifndef SERVER_RECORDER_RECORDER_H_
#define SERVER_RECORDER_RECORDER_H_

/*
 * header
 */

/*
 * define
 */
#define		THREAD_EPOLL					4
#define		MAX_BETWEEN_RECODER_PAUSE		3		//3s
#define		TIMEOUT							3		//3s
#define		MIN_SD_SIZE_IN_MB				64		//64M

#define		ERR_NONE						0
#define		ERR_NO_DATA						-1
#define		ERR_TIME_OUT					-2
#define		ERR_LOCK						-3
#define		ERR_ERROR						-4

typedef enum recorder_init_condition_t {
    RECORDER_INIT_CONDITION_DEVICE_SD = 0,
    RECORDER_INIT_CONDITION_TIME_SYCHRONIZED,
    RECORDER_INIT_CONDITION_NUM,
} recorder_init_condition_t;

#define RECORDER_EXIT_CONDITION         (0)

/*
 * structure
 */

/*
 * function
 */

#endif
