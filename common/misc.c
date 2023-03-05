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
//program header
//server header
#include "misc.h"
#include "log.h"
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

int misc_generate_random_id(void) {
    struct timeval t1 = {0, 0};
    int value = 0;
    int max = 20000;
    int min = 10000;

    unsigned int srandVal;
    gettimeofday(&t1, NULL);
    srandVal = t1.tv_sec + t1.tv_usec;
    srand(srandVal);

    //[10000, 20000]
    value = rand() % (max + 1 - min) + min;

    return value;
}

void misc_set_thread_name(char *name) {
    prctl(PR_SET_NAME, (unsigned long) name, 0, 0, 0);
    pid_t tid;
    tid = syscall(SYS_gettid);
}

int misc_get_bit(int a, int bit) {
    return (a >> bit) & 0x01;
}

int misc_set_bit(int *a, int bit) {
    *a &= ~(0x1 << bit);
    *a |= 0x1 << bit;
}

int misc_clear_bit(int *a, int bit) {
    *a &= ~(0x1 << bit);
}

int misc_full_bit(int a, int num) {
    if (a == ((1 << num) - 1))
        return 1;
    else
        return 0;
}


int misc_substr(char dst[], char src[], int start, int len) {
    int i;
    for (i = 0; i < len; i++) {
        dst[i] = src[start + i];
    }
    dst[i] = '\0';
    return i;
}

int misc_arm_address_check(unsigned int address) {
    if (address == 0)
        return 1;
/*	else if( (address | 0x3) )
		return 1;
		*/
    else if ((address > 0x80000000))
        return 1;
    else if ((address < 0x1000))
        return 1;
    else
        return 0;
}

int misc_str_hex_2_hex(unsigned char *dst, char *src, int src_len) {
    int i;
    unsigned char temp = 0;

    for (i = 0; i < src_len / 2; i++) {
        if ('0' <= src[2 * i] && src[2 * i] <= '9') {
            temp = src[2 * i] - '0';
            temp = temp << 4;
        } else if ('a' <= src[2 * i] && src[2 * i] <= 'f') {
            temp = src[2 * i] - 'a' + 10;
            temp = temp << 4;
        } else if ('A' <= src[2 * i] && src[2 * i] <= 'F') {
            temp = src[2 * i] - 'A' + 10;
            temp = temp << 4;
        }

        if ('0' <= src[2 * i + 1] && src[2 * i + 1] <= '9') {
            temp += src[2 * i + 1] - '0';
        } else if ('a' <= src[2 * i + 1] && src[2 * i + 1] <= 'f') {
            temp += src[2 * i + 1] - 'a' + 10;
        } else if ('A' <= src[2 * i + 1] && src[2 * i + 1] <= 'F') {
            temp += src[2 * i + 1] - 'A' + 10;
        }
        dst[i] = temp;
    }

    return i;
}

int misc_get_file_size(char *file_name) {
    int ret;
    FILE *fp;
    int size = 0;
    fp = fopen(file_name, "r+");
    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    ret = fseek(fp, 0L, SEEK_SET);
    if (ret == -1) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return size;
}