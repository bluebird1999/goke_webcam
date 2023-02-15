#ifndef SERVER_PLAYER_CONFIG_H_
#define SERVER_PLAYER_CONFIG_H_

/*
 * header
 */
#include "../../global/global_interface.h"
/*
 * define
 */

/*
 * structure
 */
typedef struct player_config_t {
	int					status;
    char				enable;
    char				offset;
    char   				path[MAX_SYSTEM_STRING_SIZE*2];
    char   				prefix[MAX_SYSTEM_STRING_SIZE];
} player_config_t;

/*
 * function
 */

#endif
