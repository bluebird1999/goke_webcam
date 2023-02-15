#ifndef COMMON_LOG_H_
#define COMMON_LOG_H_

/*
 * header
 */
#include <string.h>

/*
 * define
 */

typedef enum global_debug_level_t {
    DEBUG_NONE,					//0
    DEBUG_SERIOUS,
    DEBUG_WARNING,
    DEBUG_INFO,
    DEBUG_VERBOSE,
} global_debug_level_t;

#define finename(x) ( strrchr( x, '/' ) ? ( strrchr(x,'/') + 1 ) : x )

/*
 * structure
 */

/*
 * function
 */

int log_new(char* file, int line, int now_level, int level, const char* format, ...);


#endif
