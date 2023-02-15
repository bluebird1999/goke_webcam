#ifndef COMMON_FILE_MANAGER_H
#define COMMON_FILE_MANAGER_H

#include <link_visual_api.h>

#define		MAX_FILE_NUM							1440

typedef struct file_list_node_t {
    unsigned int					start;
    unsigned int					stop;
    struct file_list_node_t*    	next;
} file_list_node_t;

typedef struct file_list_t {
    unsigned long long start[MAX_FILE_NUM];
    unsigned long long stop[MAX_FILE_NUM];
    unsigned int	num;
    unsigned int	begin;
    unsigned int	end;
} file_list_t;

int file_manager_get_file_by_day(int type, int num, unsigned long long sr_start, unsigned long long sr_stop, lv_query_record_response_param_s* param);
int file_manager_search_file_list(int type, long long start, long long end, file_list_node_t **fhead);
int file_manager_search_file(char *fname, int timezone, file_list_node_t **fhead);
int file_manager_node_clear(file_list_node_t **fhead);
int file_manager_check_file_list(int type, long long start, long long end);
int file_manager_read_file_list(int timezone);
int file_manager_check_sd(void);
int file_manager_clean_disk(void);
int file_manager_add_file(int type, unsigned long long start, unsigned long long stop);
int file_manager_set_reinit(void);

#endif
