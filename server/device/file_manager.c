#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <malloc.h>
#include <link_visual_api.h>
#include "../../global/global_interface.h"
#include "../../common/tools_interface.h"
#include "../manager/config.h"
#include "../manager/manager_interface.h"
#include "gk_sd.h"
#include "file_manager.h"


static pthread_rwlock_t ilock = PTHREAD_RWLOCK_INITIALIZER;
static file_list_t flist[LV_STORAGE_RECORD_INITIATIVE+1];
static int init = 0;

static int file_manager_file_bisearch_start(int type, long long key) {
    int low = 0, mid, high = flist[type].num - 1;
    if (flist[type].start[0] > key) return 0;
    while (low <= high) {
        mid = (low + high) / 2;
        if (key < flist[type].start[mid]) {
            high = mid - 1;
        } else if (key > flist[type].start[mid]) {
            low = mid + 1;
        } else {
            return mid;
        }
    }
    if (key > flist[type].start[low])
        return low;
    else
        return low - 1;
}

static int file_manager_file_bisearch_stop(int type, long long key) {
    int low = 0, mid, high = flist[type].num - 1;
    if (flist[type].stop[flist[type].num - 1] < key) return flist[type].num - 1;
    while (low <= high) {
        mid = (low + high) / 2;
        if (key < flist[type].stop[mid]) {
            high = mid - 1;
        } else if (key > flist[type].stop[mid]) {
            low = mid + 1;
        } else {
            return mid;
        }
    }
    if (key < flist[type].stop[low])
        return low;
    else
        return low + 1;
}

static int file_manager_node_remove_last(file_list_node_t **head) {
    int ret = 0;
    file_list_node_t *current = *head;
    if (current->next == NULL) {
        free(current);
        return 0;
    }
    while (current->next->next != NULL) {
        current = current->next;
    }
    free(current->next);
    current->next = NULL;
    return ret;
}

static int file_manager_node_insert(file_list_node_t **fhead, unsigned int start, unsigned int stop) {
    file_list_node_t *current;
    file_list_node_t *node = malloc(sizeof(file_list_node_t));
    node->start = start;
    node->stop = stop;
    node->next = NULL;
    if (*fhead == NULL) *fhead = node;
    else {
        for (current = *fhead; current->next != NULL; current = current->next);
        current->next = node;
    }
    return 0;
}

int file_manager_node_clear(file_list_node_t **fhead) {
    if (*fhead == NULL) return 0;
    while ((*fhead)->next != NULL)
        file_manager_node_remove_last(fhead);
    file_manager_node_remove_last(fhead);
    *fhead = NULL;
    return 0;
}

int file_manager_search_file(char *fname, int timezone, file_list_node_t **fhead) {
    int ret = 0;
    int i, type;
    char *pos;
    long long int start, stop;
    char name[MAX_SYSTEM_STRING_SIZE];

    if (file_manager_node_clear(fhead)) {
        return -1;
    }
    //search type first
    type = -1;
    for( i = 0; i<= LV_STORAGE_RECORD_INITIATIVE; i++ ) {
        pos = NULL;
        pos = strstr(fname, manager_config.folder_prefix[i]);
        if( pos ) {
            type = i;
            pos += strlen(manager_config.folder_prefix[i]) + 1; //point to start time tag
            break;
        }
    }
    if( type == -1 ) {
        return -1;
    }
    if (flist[type].num <= 0) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    //search for start and stop
    char *pend;
    memset( name, 0, sizeof(name));
    memcpy( name, pos, 10);
    pos += 10;
    pos += 1;
    start = strtoll(name, &pend, 10 );
    memset( name, 0, sizeof(name));
    memcpy( name, pos, 10);
    stop = strtoll(name, &pend, 10 );

    ret = file_manager_node_insert(fhead, start, stop);
    return ret;
}

int file_manager_search_file_list(int type, long long start, long long end, file_list_node_t **fhead) {
    int ret = 0, start_index, stop_index;
    int i, num;
    if (file_manager_node_clear(fhead)) return -1;
    pthread_rwlock_rdlock(&ilock);
    if (flist[type].num <= 0) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    start_index = file_manager_file_bisearch_start(type,start);
    stop_index = file_manager_file_bisearch_stop(type,end);
    if (start_index < 0 || stop_index < 0) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    if ((start_index > (flist[type].num - 1)) || (stop_index > (flist[type].num - 1))) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    if (start_index > stop_index) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    num = 0;
    for (i = start_index; i <= stop_index; i++) {
        if (flist[type].stop[i] <= start) continue;
        if (flist[type].start[i] >= end) continue;
        num++;
        file_manager_node_insert(fhead, flist[type].start[i], flist[type].stop[i]);
    }
    if (num == 0) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    pthread_rwlock_unlock(&ilock);
    return ret;
}

int file_manager_check_file_list(int type, long long start, long long end) {
    int ret = 0, start_index, stop_index;
    int i, num;
    pthread_rwlock_rdlock(&ilock);
    if (flist[type].num <= 0) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    start_index = file_manager_file_bisearch_start(type, start);
    stop_index = file_manager_file_bisearch_stop(type,end);
    if (start_index < 0 || stop_index < 0) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    if ((start_index > (flist[type].num - 1)) || (stop_index > (flist[type].num - 1))) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    if (start_index > stop_index) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    num = 0;
    for (i = start_index; i <= stop_index; i++) {
        if (flist[type].stop[i] <= start)
            continue;
        if (flist[type].start[i] >= end)
            continue;;
        num++;
    }
    if (num == 0) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    pthread_rwlock_unlock(&ilock);
    return ret;
}

int file_manager_read_file_list(int timezone) {
    struct dirent **namelist;
    int n;
    char name[MAX_SYSTEM_STRING_SIZE];
    char this_path[MAX_SYSTEM_STRING_SIZE];
    char temp_path[MAX_SYSTEM_STRING_SIZE];
    char *p = NULL;

    if (init) {
        return 0;
    }
    memset(&flist, 0, sizeof(file_list_t));
    pthread_rwlock_rdlock(&ilock);
    //all folders
    for( int i = 0; i <= LV_STORAGE_RECORD_INITIATIVE; i++ ) {
        memset(this_path, 0, sizeof(this_path));
        sprintf(this_path, "%s%s/", manager_config.media_directory, manager_config.folder_prefix[i]);
        if (access(this_path, F_OK))
            mkdir(this_path, 0777);
        //scan
        n = scandir(this_path, &namelist, 0, alphasort);
        if (n < 0) {
            log_goke(DEBUG_SERIOUS, "@@@@@@@@@@@@@@@Open dir error %s", this_path);
            continue;
        }
        int index = 0;
        while (index < n) {
            if (strcmp(namelist[index]->d_name, ".") == 0 ||
                strcmp(namelist[index]->d_name, "..") == 0)
                goto EXIT;
            if (namelist[index]->d_type == 10)
                goto EXIT;
            if (namelist[index]->d_type == 4)
                goto EXIT;
            if ((strstr(namelist[index]->d_name, ".mp4") == NULL) &&
                (strstr(namelist[index]->d_name, ".jpg") == NULL)) {
                //remove file here.
                memset(name, 0, sizeof(name));
                sprintf(name, "%s%s", this_path, namelist[index]->d_name);
                remove(name);
                goto EXIT;
            }
            if (strstr(namelist[index]->d_name, "snap-") ||
                strstr(namelist[index]->d_name, "normal-") ||
                strstr(namelist[index]->d_name, "plan-") ||
                strstr(namelist[index]->d_name, "alarm-")) {
                //remove file with "snap-" and "normal-" and "motion" and "alarm-" here.
                memset(name, 0, sizeof(name));
                sprintf(name, "%s%s", this_path, namelist[index]->d_name);
                remove(name);
                goto EXIT;
            }
            if (strstr(namelist[index]->d_name, ".jpg")) {
                //remove file here.
                memset(&temp_path, 0, sizeof(temp_path));
                sprintf(temp_path, "%s%s", this_path, namelist[index]->d_name);
                int length = strlen(temp_path);
                temp_path[length - 3] = 'm';
                temp_path[length - 2] = 'p';
                temp_path[length - 1] = '4';
                if (access(temp_path, F_OK)) {
                    memset(name, 0, sizeof(name));
                    sprintf(name, "%s%s", this_path, namelist[index]->d_name);
                    remove(name);
                    goto EXIT;
                }
            }
            if (strstr(namelist[index]->d_name, ".jpg"))
                goto EXIT;
            p = strstr(namelist[index]->d_name, "1");    //first
            if (p == NULL)
                goto EXIT;
            char *pend;
            memset(name, 0, sizeof(name));
            memcpy(name, p, 10);
            long long int st, ed;
            st = strtoll( name, &pend, 10);
            p += 11;
            memset(name, 0, sizeof(name));
            memcpy(name, p, 10);
            ed = strtoll( name, &pend, 10);
            if (st >= ed) {
                memset(name, 0, sizeof(name));
                sprintf(name, "%s%s", this_path, namelist[index]->d_name);
                remove(name);
                goto EXIT;
            }
            flist[i].start[flist[i].num] = st;
            flist[i].stop[flist[i].num] = ed;
            flist[i].num++;
            EXIT:
            free(namelist[index]);
            index++;
            if (flist[i].num >= MAX_FILE_NUM) {
                flist[i].num = MAX_FILE_NUM;
                break;
            }
        }
        free(namelist);
    }
    pthread_rwlock_unlock(&ilock);
    init = 1;
    return 0;
}

int file_manager_get_file_by_day(int type, int number, unsigned long long sr_start, unsigned long long sr_stop,
                           lv_query_record_response_param_s *param) {
    int ret = 0, i, j, index;
    lv_query_record_response_day *list;
    unsigned int len = 0, num = 0;
    int start,end;
    char fname[MAX_SYSTEM_STRING_SIZE];

    pthread_rwlock_rdlock(&ilock);
    if (!init) {
        ret = -1;
    } else {
        if( type == LV_STORAGE_RECORD_ANY ) {
            start = 0;
            end = LV_STORAGE_RECORD_INITIATIVE;
        } else {
            start = end = type;
        }
        for(j=start;j<=end;j++) {
            for (i = 0; i < flist[j].num; i++) {
                if (flist[j].start[i] < sr_start) continue;
                if (flist[j].stop[i] > sr_stop) continue;
                num++;
            }
        }
        if ( (num > 0) && (number>0) ) {
            if( num > number ) {
                num = number;
            }
            len = num * sizeof(lv_query_record_response_day);
            list = malloc(len);
            if (!list) {
                log_goke(DEBUG_SERIOUS, "Fail to calloc. size = %d", len);
                ret = -1;
                goto EXIT;
            }
            index = 0;
            for(j=start;j<=end;j++) {
                for (i = 0; i < flist[j].num; i++) {
                    if (flist[j].start[i] < sr_start)
                        continue;
                    if (flist[j].stop[i] > sr_stop)
                        continue;
                    memset(fname, 0, sizeof(fname));
                    sprintf(fname, "%s%s/%lld_%lld.mp4", manager_config.media_directory,
                            manager_config.folder_prefix[j], flist[j].start[i], flist[j].stop[i]);
                    list[index].file_name = malloc(strlen(fname) + 1);
                    if (list[index].file_name == 0) {
                        log_goke(DEBUG_SERIOUS, " memroy allocation error!");
                        break;
                    }
                    memcpy(list[index].file_name, fname, strlen(fname));
                    list[index].file_name[strlen(fname)] = '\0';
                    list[index].file_size = misc_get_file_size(list[index].file_name);
                    list[index].start_time = flist[j].start[i];
                    list[index].stop_time = flist[j].stop[i];
                    list[index].record_type = j;
                    index++;
                    if( index == number ) {
                        goto FINISH;
                    }
                }
            }
            FINISH:
            param->by_day.num = num;
            param->by_day.days = list;
        } else {
            param->by_day.num = 0;
            param->by_day.days = 0;
        }
    }
    pthread_rwlock_unlock(&ilock);
    return ret;
    EXIT:
    param->by_day.num = 0;
    param->by_day.days = 0;
    pthread_rwlock_unlock(&ilock);
    return ret;
}
//
//static void *player_picture_func(void *arg) {
//    int ret = 0;
//    unsigned int empty[2] = {0, 0};
//    int i, num;
//    FILE *fp = NULL;
//    message_t send_msg;
//    unsigned int len, offset, all, size;
//    unsigned int start, fsize, cmd;
//    char *data = NULL;
//    char fname[2 * MAX_SYSTEM_STRING_SIZE];
//    char start_str[MAX_SYSTEM_STRING_SIZE];
//    char stop_str[MAX_SYSTEM_STRING_SIZE];
//    unsigned int begin[128], end[128];
//    pthread_detach(pthread_self());
//    log_goke(DEBUG_INFO, "---------------------player picture listing thread started!---------------");
//    misc_set_thread_name("player_picture_list");
//    message_t *msg = (message_t *) arg;
//    /********message body********/
//    msg_init(&send_msg);
//    memcpy(&(send_msg.arg_pass), &(msg->arg_pass), sizeof(message_arg_t));
//    send_msg.message = msg->message | 0x1000;
//    send_msg.sender = send_msg.receiver = SERVER_PLAYER;
//    /****************************/
//    cmd = msg->arg_in.duck;
//    if (!init) {
//        ret = -1;
//        goto send;
//    } else {
//        pthread_rwlock_rdlock(&ilock);
//        size = 0;
//        for (i = 0; (i < flist.num); i++) {
//            if (flist.start[i] < msg->arg_in.cat) continue;
//            if (flist.stop[i] > msg->arg_in.dog + 59) continue;
//            begin[size] = (unsigned int) flist.start[i];
//            end[size] = (unsigned int) flist.stop[i];
//            size++;
//            if (size > 128) {
//                log_goke(DEBUG_WARNING, "---exceed the 128 jpg file list limit, quit!---------");
//                break;
//            }
//        }
//        pthread_rwlock_unlock(&ilock);
//        for (i = 0; i < size; i++) {
//            memset(fname, 0, sizeof(fname));
//            memset(start_str, 0, sizeof(start_str));
//            memset(stop_str, 0, sizeof(stop_str));
//            time_stamp_to_date_with_zone(begin[i], start_str, 80, manager_config.timezone);
//            time_stamp_to_date_with_zone(end[i], stop_str, 80, manager_config.timezone);
//            sprintf(fname, "%s%s-%s_%s.jpg", recorder_config.directory, recorder_config.normal_prefix, start_str,
//                    stop_str);
//            fp = fopen(fname, "rb");
//            if (fp == NULL) {
//                log_goke(DEBUG_SERIOUS, "fopen error %s not exist!", fname);
//                continue;
//            }
//            if (0 != fseek(fp, 0, SEEK_END)) {
//                fclose(fp);
//                continue;
//            }
//            fsize = ftell(fp);
//            start = (unsigned int) begin[i];
//            all = sizeof(cmd) + sizeof(start) + sizeof(fsize) + fsize;
//            data = malloc(all);
//            if (!data) {
//                fclose(fp);
//                ret = -1;
//                goto send;
//            }
//            memset(data, 0, all);
//            if (0 != fseek(fp, 0, SEEK_SET)) {
//                free(data);
//                fclose(fp);
//                ret = -1;
//                goto send;
//            }
//            memcpy(data, &cmd, sizeof(cmd));
//            memcpy(data + sizeof(cmd), &start, sizeof(start));
//            memcpy(data + sizeof(cmd) + sizeof(start), &fsize, sizeof(fsize));
//            len = fsize;
//            offset = 0;
//            while (len > 0) {
//                ret = fread(data + sizeof(cmd) + sizeof(start) + sizeof(fsize) + offset, 1, len, fp);
//                if (ret >= 0) {
//                    offset += ret;
//                    len -= ret;
//                } else {
//                    log_goke(DEBUG_WARNING, "offset:%d  len:%d  ret:%d", offset, len, ret);
//                    break;
//                }
//            }
//            /***************************************/
//            send_msg.arg = data;
//            send_msg.arg_size = all;
//            send_msg.result = 0;
//            global_common_send_message(msg->sender, &send_msg);
//            /***************************************/
//            offset = 0;
//            num++;
//            free(data);
//            fclose(fp);
//            usleep(10000);
//        }
//        ret = -2;
//    }
//    send:
//    empty[0] = cmd;
//    send_msg.arg = empty;
//    send_msg.arg_size = sizeof(empty);
//    send_msg.result = 0;
//    global_common_send_message(msg->sender, &send_msg);
//    global_common_send_dummy(SERVER_PLAYER);
//    log_goke(DEBUG_INFO, "---------------------player picture listing thread exit---------------");
//    pthread_exit(0);
//}
//
//int player_get_picture_list(message_t *msg) {
//    int ret = 0;
//    unsigned int empty[2] = {0, 0};
//    static message_t smsg;
//    message_t send_msg;
//    pthread_t pid;
//    msg_init(&smsg);
//    memcpy(&smsg, msg, sizeof(message_t));
//    ret = pthread_create(&pid, NULL, player_picture_func, (void *) &smsg);
//    if (ret != 0) {
//        log_goke(DEBUG_SERIOUS, "player picture reading thread create error! ret = %d", ret);
//        /********message body********/
//        msg_init(&send_msg);
//        memcpy(&(send_msg.arg_pass), &(msg->arg_pass), sizeof(message_arg_t));
//        send_msg.message = msg->message | 0x1000;
//        send_msg.sender = send_msg.receiver = SERVER_PLAYER;
//        empty[0] = msg->arg_in.duck;
//        send_msg.result = 0;
//        send_msg.arg = empty;
//        send_msg.arg_size = sizeof(empty);
//        global_common_send_message(msg->sender, &send_msg);
//        /****************************/
//    }
//    return 0;
//}


int file_manager_add_file(int type, unsigned long long start, unsigned long long stop) {
    int ret = 0;
    pthread_rwlock_wrlock(&ilock);
    if (flist[type].num >= MAX_FILE_NUM) {
        ret = -1;
    } else {
        flist[type].start[flist[type].num] = start;
        flist[type].stop[flist[type].num] = stop;
        flist[type].num++;
    }
    pthread_rwlock_unlock(&ilock);
    return ret;
}

int file_manager_check_sd(void)
{
    int ret;
	if(sd_get_status() == SD_ALIYUN_STATUS_NORMAL)
		ret = 0;
	else
		ret = -1;
    return ret;
}

int file_manager_clean_disk(void)
{
    struct dirent **namelist;
    int 	n;
    char 	path[MAX_SYSTEM_STRING_SIZE*4];
    char 	name[MAX_SYSTEM_STRING_SIZE*4];
    char 	thisdate[MAX_SYSTEM_STRING_SIZE];
    unsigned long long int cutoff_date;
    char 	*p = NULL;
    unsigned long long int start, block_time, now;
    int		i = 0, j = 0;
    int		deleted = 0;
    pthread_rwlock_wrlock(&ilock);
    now = time_get_now();
    block_time = 0;
    memset(thisdate, 0, sizeof(thisdate));
    time_stamp_to_date_with_zone(now, thisdate, 80, manager_config.timezone);
    log_goke(DEBUG_INFO, "----------current time is %s-------", thisdate);
    restart:
    log_goke(DEBUG_INFO, "----------start sd cleanning job-------");
    cutoff_date = now - 1 * 86400 + block_time;
    memset(thisdate, 0, sizeof(thisdate));
    time_stamp_to_date_with_zone(cutoff_date, thisdate, 80, manager_config.timezone);
    log_goke(DEBUG_INFO, "----------delete media file before %s-------", thisdate);
    for( j=0; j<= LV_STORAGE_RECORD_INITIATIVE;j++) {
        memset(path, 0, sizeof(path));
        log_goke(DEBUG_INFO, "----------inside %s directory-------", manager_config.folder_prefix[j]);
        sprintf(path, "%s%s/", manager_config.media_directory, manager_config.folder_prefix[j]);

        n = scandir(path, &namelist, 0, alphasort);
        if(n < 0) {
            log_goke(DEBUG_SERIOUS, "Open dir error %s", path);
            return -1;
        }
        else {
            int index=0;
            while(index < n) {
                if(strcmp(namelist[index]->d_name,".") == 0 ||
                   strcmp(namelist[index]->d_name,"..") == 0 )
                    goto exit;
                if(namelist[index]->d_type == 10) goto exit;
                if(namelist[index]->d_type == 4) goto exit;
                if ( strstr(namelist[index]->d_name, "snap-") ||
                     strstr(namelist[index]->d_name, "normal-") ||
                     strstr(namelist[index]->d_name, "plan-") ||
                     strstr(namelist[index]->d_name, "alarm-") ) {
                    goto exit;
                }
                if( (strstr(namelist[index]->d_name,".mp4") == NULL) &&
                    (strstr(namelist[index]->d_name,".jpg") == NULL)	) {
                    //remove file here.
					memset(name, 0, sizeof(name));
					sprintf(name, "%s%s", path, namelist[index]->d_name);
					remove( name );
					deleted++;
					log_goke(DEBUG_INFO, "---removed %s---", name);
                    goto exit;
                }
                p = NULL;
                p = strstr( namelist[index]->d_name, "1");	//first
                if(p){
                    char *pend;
                    memset(name, 0, sizeof(name));
                    memcpy(name, p, 10);
                    start = strtoll( name, &pend, 10);
                    if( start < cutoff_date) {
                        //remove
                        memset(name, 0, sizeof(name));
                        sprintf(name, "%s%s", path, namelist[index]->d_name);
                        remove( name );
                        log_goke(DEBUG_MAX, "---removed %s---", name);
                        deleted++;
                    }
                }
                exit:
                free(namelist[index]);
                index++;
            }
        }
        free(namelist);
        namelist = NULL;
        i++;
    }
    if( deleted == 0 ) {
        block_time += 3600*4;	//4 hours step
        if( block_time < 86400 ) {
            i = 0;
            goto restart;
        } else {
            block_time -= 1*3600;   // 1 hour before
            i = 0;
            goto restart;
        }
    }
    if( deleted ) {
        log_goke(DEBUG_INFO, "---removed %d files from sd!!!---", deleted);
    }
    pthread_rwlock_unlock(&ilock);
    return 0;
}

int file_manager_set_reinit(void) {
    pthread_rwlock_wrlock(&ilock);
    if(init) {
        init = 0;
        memset(flist, 0, sizeof(flist));
    }
    pthread_rwlock_unlock(&ilock);
    return 0;
}