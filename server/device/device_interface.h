#ifndef SERVER_DEVICE_INTERFACE_H_
#define SERVER_DEVICE_INTERFACE_H_

/*
 * header
 */
#include "../manager/manager_interface.h"

/*
 * define
 */
#define		SERVER_DEVICE_VERSION_STRING		"alpha-8.0.0"

//control
typedef enum device_param_type_t {
    DEVICE_PARAM_SD_INFO = 0,
} device_param_type_t;

/*
 * structure
 */
/* if type  > 0,  volume must be -1;
 * if volume >= 0,  event must be 0;
 *
 * int_out : control mic volume or speaker volume
 * 			 0 -> mic
 * 			 1 -> speaker
 *
 * type:  0 -> no event, control by volume
 * 		  1 -> volume up
 * 		  2 -> volume down
 * 		  3 -> voulme mute
 * 		  4 -> volume unmute
 * volume: -1 -> no volume, control by event
 * 		   >= 0 && <= 100 -> set volume (range 0 ~ 100)
 */
//audio		------------------------------------------------------------
typedef struct audio_info_t {
	unsigned int 	in_out;
	unsigned int 	type;
			 int	volume;
} audio_info_t;

//part 		------------------------------------------------------------
typedef struct part_info
{
    char name[16];
    unsigned int size;  //KB in units
}part_info_t;

typedef struct part_msg_info
{
    int part_num;
    part_info_t part_info_sum[10];
}part_msg_info_t;

//iot struct ------------------------------------------------------------
typedef struct device_iot_config_t {
	int day_night_mode;
	int led1_onoff;
	int led2_onoff;
	int amp_on_off;
	audio_info_t 		audio_iot_info;
	part_msg_info_t		part_iot_info;
} device_iot_config_t;

/*
 * function
 */
int server_device_start(void);
int server_device_message(message_t *msg);

#endif
