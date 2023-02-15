/*
 * header
 */
//system header
#define __USE_XOPEN
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <malloc.h>

//program header

//server header
#include "time.h"
#include "misc.h"
/*
 * static
 */
//variable

//function;



/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */
unsigned long long time_get_now_ms(void)
{
    struct timeval tv;
    unsigned int time;
    gettimeofday(&tv, NULL);
    time = tv.tv_sec*1000  + tv.tv_usec / 1000;
    return time;
}

unsigned int time_get_ms(void)
{
    struct timeval tv;
    unsigned int time;
    gettimeofday(&tv, NULL);
    time = tv.tv_usec / 1000;
    return time;
}

void time_get_now_str(char *str)
{
    long ts;
    ts = time(NULL);
    struct tm tt = {0};
    localtime_r(&ts, &tt);
	sprintf(str, "%04d%02d%02d%02d%02d%02d", tt.tm_year+1900,
											 tt.tm_mon+1,
											 tt.tm_mday,
											 tt.tm_hour,
											 tt.tm_min,
											 tt.tm_sec);

    return;
}

void time_get_now_str_format(char *str)
{
    long ts;
    ts = time(NULL);
    struct tm tt = {0};
    localtime_r(&ts, &tt);
	sprintf(str, "%02d:%02d:%02d.%03d", 	tt.tm_hour,
											tt.tm_min,
											tt.tm_sec,
											time_get_ms());

    return;
}

long long int time_date_to_stamp(char *date)
{
//	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
//	strptime(date,"%Y%m%d%H%M%S",tmp_time);
//	time_t t = mktime(tmp_time);
//	free(tmp_time);
    struct 	tm tm;
    char	temp[4];
    memset(&tm, 0, sizeof(struct tm));
    misc_substr(temp, date, 0, 4);
    tm.tm_year = atoi( temp ) - 1900;
    misc_substr(temp, date, 4, 2);
    tm.tm_mon = atoi( temp ) - 1;
    misc_substr(temp, date, 6, 2);
    tm.tm_mday = atoi( temp );
    misc_substr(temp, date, 8, 2);
    tm.tm_hour = atoi( temp );
    misc_substr(temp, date, 10, 2);
    tm.tm_min = atoi( temp );
    misc_substr(temp, date, 12, 2);
    tm.tm_sec = atoi( temp );
	time_t t = mktime(&tm);
	return t;
}

long long int time_date_to_stamp_with_zone(char *date, int standard_zone, int zone)
{
//	struct tm* tmp_time = (struct tm*)malloc(sizeof(struct tm));
//	strptime(date,"%Y%m%d%H%M%S",tmp_time);
//	time_t t = mktime(tmp_time);
//	free(tmp_time);
    struct 	tm tm;
    char	temp[4];
    memset(&tm, 0, sizeof(struct tm));
    misc_substr(temp, date, 0, 4);
    tm.tm_year = atoi( temp ) - 1900;
    misc_substr(temp, date, 4, 2);
    tm.tm_mon = atoi( temp ) - 1;
    misc_substr(temp, date, 6, 2);
    tm.tm_mday = atoi( temp );
    misc_substr(temp, date, 8, 2);
    tm.tm_hour = atoi( temp );
    misc_substr(temp, date, 10, 2);
    tm.tm_min = atoi( temp );
    misc_substr(temp, date, 12, 2);
    tm.tm_sec = atoi( temp );
	time_t t = mktime(&tm);
	zone-=standard_zone;
	t += (zone * 360);
	return t;
}

long long int time_get_now_stamp(void)
{
    struct timeval tv;
    unsigned int time;
    gettimeofday(&tv, NULL);
    time = (unsigned int)tv.tv_sec;
    return time;
}

int time_stamp_to_date(long long int stamp, char *dd)
{
	struct tm tt = {0};
	localtime_r(&stamp, &tt);
//	strftime(dd, 32, "%04Y%02m%02d%02H%02M%02S", tmp_time);
	sprintf(dd, "%04d%02d%02d%02d%02d%02d", tt.tm_year+1900,
			tt.tm_mon+1,
			tt.tm_mday,
			tt.tm_hour,
			tt.tm_min,
			tt.tm_sec);
	return 0;
}

int time_stamp_to_date_with_zone(long long int stamp, char *dd, int standard_zone, int zone)
{
	struct tm tt = {0};
	zone-=standard_zone;
	stamp -= (zone * 360);
	localtime_r(&stamp, &tt);
//	strftime(dd, 32, "%04Y%02m%02d%02H%02M%02S", tmp_time);
	sprintf(dd, "%04d%02d%02d%02d%02d%02d", tt.tm_year+1900,
			tt.tm_mon+1,
			tt.tm_mday,
			tt.tm_hour,
			tt.tm_min,
			tt.tm_sec);
	return 0;
}


unsigned int time_getuptime_ms(void)
{
	struct timespec time = {0, 0};
	long long uptime = 0;
	unsigned int uptime_ms = 0;

	clock_gettime(CLOCK_MONOTONIC, &time);
	uptime = time.tv_sec * 1000000 + time.tv_nsec / 1000;
	uptime_ms = uptime/1000;
	return (unsigned int)uptime_ms;

}

