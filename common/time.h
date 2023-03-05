#ifndef COMMON_TIME_H_
#define COMMON_TIME_H_

/*
 * header
 */
#include <time.h>

/*
 * define
 */

/*
 * structure
 */
typedef struct time_struct_t{
    unsigned short  year;
    unsigned short  month;
    unsigned short  day;
    unsigned short  hour;
    unsigned short  minute;
    unsigned short  second;
} time_struct_t;

typedef struct scheduler_time_t{
    int start_hour;
    int start_min;
	int start_sec;
    int stop_hour;
    int stop_min;
	int stop_sec;
} scheduler_time_t;

typedef struct scheduler_slice_t {
	time_t start;
	time_t end;
} scheduler_slice_t;

/*
 * function
 */
unsigned long long time_get_now(void);
unsigned long long time_get_now_ms(void);
void time_get_now_str(char *str);
void time_get_now_str_format(char *str);
long long int time_date_to_stamp(char *date);
long long int time_get_now_stamp(void);
int time_stamp_to_date(long long int stamp, char *date);
unsigned int time_get_ms(void);
int time_stamp_to_date_with_zone(long long int stamp, char *dd, int standard_zone, int zone);
long long int time_date_to_stamp_with_zone(char *date, int standard_zone, int zone);
unsigned int time_getuptime_ms(void);

#endif
