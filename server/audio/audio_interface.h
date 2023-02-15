#ifndef SERVER_AUDIO_AUDIO_INTERFACE_H_
#define SERVER_AUDIO_AUDIO_INTERFACE_H_

/*
 * header
 */
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"
#include "../../server/goke/hisi/hisi.h"

/*
 * define
 */
#define		SERVER_AUDIO_VERSION_STRING			"alpha-8.1"

#define		AUDIO_PROPERTY_SERVER_STATUS		(0x000 | PROPERTY_TYPE_GET)
#define		AUDIO_PROPERTY_FORMAT				(0x001 | PROPERTY_TYPE_GET)
#define		AUDIO_PROPERTY_SPEAKER				(0x002 | PROPERTY_TYPE_GET)

#define     SPEAKER_CTL_INTERCOM_START      	0x0030
#define     SPEAKER_CTL_INTERCOM_STOP       	0x0040
#define     SPEAKER_CTL_INTERCOM_DATA        	0x0050

#define     SPEAKER_CTL_DEV_START_FINISH        0x0060
#define     SPEAKER_CTL_ZBAR_SCAN_SUCCEED       0x0070
#define     SPEAKER_CTL_WIFI_CONNECT            0x0080
#define     SPEAKER_CTL_INTERNET_CONNECT_DEFEAT	0x0081
#define     SPEAKER_CTL_ZBAR_SCAN	            0x0090
#define     SPEAKER_CTL_INSTALLING		        0x0091
#define     SPEAKER_CTL_INSTALLEND				0x0092
#define     SPEAKER_CTL_INSTALLFAILED		    0x0093
#define     SPEAKER_CTL_RESET				    0x0094
#define 	SPEAKER_PLAYBACK_CHN_NUM			0x0010


/*
 * structure
 */
typedef enum audio_resource_type_e {
    AUDIO_RESOURCE_SCAN_START = 0,
    AUDIO_RESOURCE_SCAN_SUCCESS,
    AUDIO_RESOURCE_BINDING_SUCCESS,
    AUDIO_RESOURCE_START,
    AUDIO_RESOURCE_BUTT,
} audio_resource_type_e;

/*
 * function
 */
int server_audio_start(void);
int server_audio_message(message_t *msg);

#endif
