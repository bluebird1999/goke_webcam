/*
 * header
 */
//system header
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>
#include <malloc.h>
//program header
//server header
#include "msg_buffer.h"
#include "../log.h"

/*
 * static
 */
//variable

//function;

//specific
/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */
/*
 * helper
 */
static int msg_buffer_swap_item(message_buffer_t *buff, int org, int dest) {
    message_t temp;
    //swap
    msg_init(&temp);
    msg_deep_copy(&temp, &(buff->buffer[org]));
    msg_free(&(buff->buffer[org]));
    msg_deep_copy(&(buff->buffer[org]), &(buff->buffer[dest]));
    msg_free(&(buff->buffer[dest]));
    msg_deep_copy(&(buff->buffer[dest]), &temp);
    msg_free(&temp);
    return 0;
}

/*
 * interface
 */
int msg_buffer_is_empty(message_buffer_t *buffer) {
    return (buffer->head == buffer->tail);
}

int msg_buffer_is_full(message_buffer_t *buffer) {
    return ((buffer->head - buffer->tail) & (buffer->size-1)) == (buffer->size-1);
}

int msg_buffer_num_items(message_buffer_t *buffer) {
    return ((buffer->head - buffer->tail) & (buffer->size-1));
}

void msg_copy(message_t *dest, message_t *source) {
    memcpy(dest, source, sizeof(message_t));
}

int msg_deep_copy(message_t *dest, message_t *source) {
    unsigned char *block;
    if (dest->arg_size > 0 && dest->arg != NULL) {
        free(dest->arg);
    }
    if (dest->extra_size > 0 && dest->extra != NULL) {
        free(dest->extra);
    }
    memcpy(dest, source, sizeof(message_t));
    if (source->arg_size > 0 && source->arg != NULL) {
        block = calloc(source->arg_size, 1);
//		printf("+++++++++++++++++++new allocation here size = %d", source->arg_size);
        if (block == NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!1buffer memory allocation failed for arg_size=%d!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
                   source->arg_size);
            printf("msg from %d and message = %x", source->sender, source->message);
            return -1;
        }
        //deep structure copy
        dest->arg = block;
        memcpy(dest->arg, source->arg, source->arg_size);
    }
    block = NULL;
    if (source->extra_size > 0 && source->extra != NULL) {
//		printf("+++++++++++++++++++new allocation here size = %d", source->extra_size);
        block = calloc(source->extra_size, 1);
        if (block == NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!buffer memory allocation failed for extra_size=%d!!!!!!!!!!!!!!!!!!!!!!!!!!!!",
                   source->extra_size);
            printf("msg from %d and message = %x", source->sender, source->message);
            return -1;
        }
        //deep structure copy
        dest->extra = block;
        memcpy(dest->extra, source->extra, source->extra_size);
    }
    return 0;
}

void msg_buffer_init(message_buffer_t *buff, int overflow,int size) {
    buff->head = 0;
    buff->tail = 0;
    buff->sequence = 0;
    buff->size = size;
    buff->overflow = overflow;
    pthread_rwlock_init(&buff->lock, NULL);
    buff->buffer = 0;
    buff->buffer = calloc( size, sizeof(message_t));
    if( buff->buffer == 0 ) {
        printf("message buffer init error (memory allocation failed) with size %d\n", size);
        return;
    }
    for (int i = 0; i < buff->size; i++) {
        if (buff->buffer[i].arg != NULL && buff->buffer[i].arg_size > 0) {
            free(buff->buffer[i].arg);
            buff->buffer[i].arg = NULL;
            buff->buffer[i].arg_size = 0;
        }
        if (buff->buffer[i].extra != NULL && buff->buffer[i].extra_size > 0) {
            free(buff->buffer[i].extra);
            buff->buffer[i].extra = NULL;
            buff->buffer[i].extra_size = 0;
        }
    }
    buff->init = 1;
}

void msg_buffer_init2(message_buffer_t *buff, int overflow, int size, pthread_mutex_t *mutex) {
    pthread_mutex_lock(mutex);
    msg_buffer_init(buff, overflow, size);
    pthread_mutex_unlock(mutex);
}


void msg_buffer_release(message_buffer_t *buff) {
    int i;
    buff->head = 0;
    buff->tail = 0;
    buff->sequence = 0;
    pthread_rwlock_destroy(&buff->lock);
    for (i = 0; i < buff->size; i++) {
        if (buff->buffer[i].arg != NULL && buff->buffer[i].arg_size > 0) {
            free(buff->buffer[i].arg);
            buff->buffer[i].arg = NULL;
            buff->buffer[i].arg_size = 0;
        }
        if (buff->buffer[i].extra != NULL && buff->buffer[i].extra_size > 0) {
            free(buff->buffer[i].extra);
            buff->buffer[i].extra = NULL;
            buff->buffer[i].extra_size = 0;
        }
    }
    free(buff->buffer);
    buff->buffer = 0;
    buff->init = 0;
}

void msg_buffer_release2(message_buffer_t *buff, pthread_mutex_t *mutex) {
    int i;
    pthread_mutex_lock(mutex);
    msg_buffer_release(buff);
    pthread_mutex_unlock(mutex);
}

int msg_buffer_probe_item(message_buffer_t *buff, int n, message_t *msg) {
    int index;
    if (msg_buffer_is_empty(buff)) {
        return 1;
    }
    index = ((buff->tail + n) & (buff->size-1) );
    if (index == buff->head) {
        return 1;
    }
    memcpy(msg, &(buff->buffer[index]), sizeof(message_t));
    return 0;
}

int msg_buffer_probe_item_extra(message_buffer_t *buff, int n, int *id, void **arg) {
    int index;
    if (msg_buffer_is_empty(buff)) {
        return 1;
    }
    index = ((buff->tail + n) & (buff->size-1) );
    if (index == buff->head) {
        return 1;
    }
    *id = buff->buffer[index].message;
    *arg = buff->buffer[index].arg_in.handler;
    return 0;
}

int msg_buffer_swap(message_buffer_t *buff, int org, int dest) {
    int org_index, dest_index, rundown_index, last_index;
    message_t temp;
    if (org == dest) return 1;
    org_index = ((buff->tail + org) & (buff->size-1));
    dest_index = ((buff->tail + dest) & (buff->size-1));
    //swap from the queue top
    msg_buffer_swap_item(buff, org_index, dest_index);
    //search upward till the top is found
    last_index = dest_index;
    rundown_index = ((last_index - 1) & (buff->size-1));
    while (rundown_index != org_index) {
        msg_buffer_swap_item(buff, rundown_index, last_index);
        last_index = rundown_index;
        rundown_index = ((last_index - 1) & (buff->size-1));
    }
    return 0;
}

int msg_buffer_pop(message_buffer_t *buff, message_t *data) {
    if (msg_buffer_is_empty(buff)) {
        return 1;
    }
    //copy message
    msg_deep_copy(data, &(buff->buffer[buff->tail]));
    // free the memory allocated in the push process
    if (buff->buffer[buff->tail].arg_size > 0 && buff->buffer[buff->tail].arg != NULL) {
        free(buff->buffer[buff->tail].arg);
        buff->buffer[buff->tail].arg = NULL;
        buff->buffer[buff->tail].arg_size = 0;
    }
    if (buff->buffer[buff->tail].extra_size > 0 && buff->buffer[buff->tail].extra != NULL) {
        free(buff->buffer[buff->tail].extra);
        buff->buffer[buff->tail].extra = NULL;
        buff->buffer[buff->tail].extra_size = 0;
    }
    //update tail
    buff->tail = ((buff->tail + 1) & (buff->size-1));
    return 0;
}

int msg_buffer_push(message_buffer_t *buff, message_t *data) {
    int ret = MSG_BUFFER_PUSH_SUCCESS;
    if (data == NULL) {
        return MSG_BUFFER_PUSH_FAIL;
    }
    //check and replace oldest item
    if (msg_buffer_is_full(buff)) {
        if (buff->overflow == MSG_BUFFER_OVERFLOW_YES) {
            //free old data
            if (buff->buffer[buff->tail].arg != NULL && buff->buffer[buff->tail].arg_size > 0) {
                free(buff->buffer[buff->tail].arg);
                buff->buffer[buff->tail].arg = NULL;
                buff->buffer[buff->tail].arg_size = 0;
            }
            if (buff->buffer[buff->tail].extra != NULL && buff->buffer[buff->tail].extra_size > 0) {
                free(buff->buffer[buff->tail].extra);
                buff->buffer[buff->tail].extra = NULL;
                buff->buffer[buff->tail].extra_size = 0;
            }
            buff->tail = ((buff->tail + 1) & (buff->size-1));
            printf("overflow happend------msg %x-------arg_size=%d extra_size = %d", data->message, data->arg_size,
                   data->extra_size);
            ret = MSG_BUFFER_PUSH_SUCCESS_WITH_OVERFLOW;
        } else {
            return MSG_BUFFER_PUSH_FAIL;
        }
    }
    buff->sequence++;
    //data copy
    msg_deep_copy(&(buff->buffer[buff->head]), data);
    buff->buffer[buff->head].sequence = buff->sequence;
    //renew head
    buff->head = ((buff->head + 1) & (buff->size-1));
    return ret;
}

int msg_init(message_t *data) {
    memset(data, 0, sizeof(message_t));
}

int msg_free(message_t *data) {
    if (data == NULL)
        return -1;
    if (data->arg != NULL && data->arg_size > 0) {
        free(data->arg);
        data->arg = NULL;
        data->arg_size = 0;
    }
    if (data->extra != NULL && data->extra_size > 0) {
        free(data->extra);
        data->extra = NULL;
        data->extra_size = 0;
    }
    return 0;
}

int msg_is_system(int id) {
    if ((id & 0xFFF0) == 0)
        return 1;
    else
        return 0;
}

int msg_is_response(int id) {
    if ((id & 0xF000))
        return 1;
    else
        return 0;
}
