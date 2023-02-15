/*
 * header
 */
//system header
#include <stdio.h>
#include <pthread.h>
#include <syscall.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
//program header
//server header
#include "log.h"
#include "time.h"
/*
 * static
 */
//variable

//function;
static void log_wrapper(const char* format, va_list args_list, char *file, int line, int now_level, int level);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */
static void log_wrapper(const char* format, va_list args_list, char *file, int line, int now_level, int level)
{
	char timestr[32];
	time_get_now_str_format(timestr);
	printf("[%s]", timestr);
	if( now_level > DEBUG_SERIOUS )
		printf("[%s:%d]", file,line);
	switch(level){
		case DEBUG_SERIOUS:
			printf("[S] ");
			break;
		case DEBUG_WARNING:
			printf("[W] ");
			break;
		case DEBUG_INFO:
			printf("[I] ");
			break;
		case DEBUG_VERBOSE:
			printf("[V] ");
			break;
	}
    vprintf(format, args_list);
    printf("\n");
}

int log_new(char* file, int line, const now_level, int level, const char* format, ...)
{
	int ret = 0;
    va_list marker;
    if( level == 0 ) {
        return 0;
    }
    if( now_level>=level ) {
    	va_start(marker, format);
    	log_wrapper(format, marker, file, line, now_level, level);
    	va_end(marker);
    }
    return ret;
}
