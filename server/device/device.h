#ifndef SERVER_DEVICE_H_
#define SERVER_DEVICE_H_

/*
 * header
 */


/*
 * define
 */
#define MOTOR_STEP_X 	1
#define MOTOR_STEP_Y 	2

#define VOLUME_MIC		0
#define VOLUME_SPEAKER	1

#define DAY_NIGHT_LIM	3000

#define MOTOR_ROTATE	2
#define MOTOR_AUTO		1
#define MOTOR_STOP		0

typedef enum device_init_condition_e {
    DEVICE_INIT_CONDITION_NUM = 0,
};

typedef enum device_thread_id_enum {
    DEVICE_THREAD_SD,
    DEVICE_THREAD_HOTPLUG,
    DEVICE_THREAD_MOTOR,
    DEVICE_THREAD_IO,
    DEVICE_THREAD_BUTT,
} device_thread_id_enum;

#define		DEVICE_EXIT_CONDITION			    0

/*
 * structure
 */


/*
 * function
 */

#endif
