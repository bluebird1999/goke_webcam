/*
 * header
 */
//system header
#include <stdio.h>
#include <unistd.h>
//program header
#include "server/manager/manager_interface.h"
#include "server/goke/goke_interface.h"
#include "server/audio/audio_interface.h"
#include "server/recorder/recorder_interface.h"
#include "server/player/player_interface.h"
#include "server/video/video_interface.h"
#include "server/device/device_interface.h"
//server header
//server header

/*
 * static
 */



/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */



/*
 * helper
 */



/*
 * 	Main function, entry point
 *
 * 		NING,  2020-08-13
 *
 */
/**/
int main(int argc, char *argv[]) {
    printf("++++++++++++++++++++++++++++++++++++++++++\r");
    printf("   webcam started\r");
    printf("---version---\r");
    printf("%10s: %s\r", "program", APPLICATION_VERSION_STRING);

    config_get(&_config_);
    manager_init();

/*
 * main loop
 */
    while (1) {
        /*
         * manager proc
         */
        manager_proc();
    }
    return 0;
}
