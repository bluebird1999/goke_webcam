#ifndef SERVER_PLAYER_PLAYER_INTERFACE_H_
#define SERVER_PLAYER_PLAYER_INTERFACE_H_

/*
 * header
 */
#include <mp4v2/mp4v2.h>
#include <pthread.h>
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"
#include "config.h"

/*
 * define
 */
#define		SERVER_PLAYER_VERSION_STRING		"alpha-8.0"

//control command
#define		PLAYER_PROPERTY_SPEED				(0x0000 | PROPERTY_TYPE_GET | PROPERTY_TYPE_SET)

/*
 * structure
 */
typedef enum {
   PLAYER_FILEFOUND = 0x10,
   PLAYER_FILENOTFOUND,
   PLAYER_FINISH,
};


typedef struct player_init_t {
	char				switch_to_live;
	char				switch_to_live_audio;
	char				auto_exit;
	char				offset;
	char				speed;
	char				channel_merge;
	char				audio;
	unsigned long long 	start;
	unsigned long long 	stop;
	int                 channel;
	int					tid;
    int                 type;
	bool				keyonly;
	char 				filename[MAX_SYSTEM_STRING_SIZE];
} player_init_t;

/*
 * function
 */
int server_player_start(void);
int server_player_message(message_t *msg);
int server_player_video_message(message_t *msg);
int server_player_audio_message(message_t *msg);
void server_player_interrupt_routine(int param);

#endif
