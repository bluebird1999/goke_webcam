/*
 * header
 */
//system header
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <mp4v2/mp4v2.h>
#include <malloc.h>
#include <link_visual_api.h>
//program header
#include "../../server/manager/manager_interface.h"
#include "../../common/tools_interface.h"
#include "../../server/player/player_interface.h"
#include "../../server/aliyun/aliyun_interface.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../server/device/device_interface.h"
#include "../device/file_manager.h"
#include "../../server/device/gk_sd.h"
//server header
#include "player.h"
#include "config.h"
#include "player_interface.h"

/*
 * static
 */
//variable
static message_buffer_t message;
static server_info_t info;
static player_job_t jobs[ALIYUN_VISUAL_MAX_CHANNEL];
static pthread_rwlock_t ilock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

//function
//common
static void *server_func(void);

static int server_message_proc(void);

static void server_release_1(void);

static void server_release_2(void);

static void server_release_3(void);

static void server_thread_termination(void);

//specific
static int player_main(player_init_t *init, player_run_t *run, file_list_node_t *fhead, int speed);

static int *player_func(void *arg);

static int find_job_channel(int channel);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
//return channel with same service id, return empty one otherwise
static void init_visual_stream_config(int channel, int fps, int duration) {
    lv_video_param_s video_param;
    lv_audio_param_s audio_param;
    lv_stream_send_config_param_s send_config;
    memset(&video_param, 0, sizeof(lv_video_param_s));
    memset(&audio_param, 0, sizeof(lv_audio_param_s));

    pthread_rwlock_rdlock(&ilock);

    memset(&video_param, 0, sizeof(lv_video_param_s));
    video_param.format = LV_VIDEO_FORMAT_H264;
    video_param.fps = fps;
    video_param.duration = duration;

    memset(&audio_param, 0, sizeof(lv_audio_param_s));
    audio_param.format = LV_AUDIO_FORMAT_G711A;
    audio_param.channel = LV_AUDIO_CHANNEL_MONO;
    audio_param.sample_bits = LV_AUDIO_SAMPLE_BITS_16BIT;
    audio_param.sample_rate = LV_AUDIO_SAMPLE_RATE_8000;

    send_config.bitrate_kbps = 1500;
    send_config.video_param = &video_param;
    send_config.audio_param = &audio_param;
    lv_stream_send_config(channel, &send_config);

    pthread_rwlock_unlock(&ilock);
}

static int player_get_video_frame(player_init_t *init, player_run_t *run, int speed) {
    char NAL[5] = {0x00, 0x00, 0x00, 0x01};
    unsigned char *data = NULL;
    unsigned char *p = NULL;
    unsigned int size = 0, num = 0, framesize;
    MP4Timestamp start_time;
    MP4Duration duration;
    MP4Duration offset;
    char iframe = 0;
    int ret = 0;
    int flag = 0;
    message_t msg;
    int sample_time;
    //**************************************
    if (!run->mp4_file) {
        return -1;
    }
    if (run->video_index >= run->video_frame_num) {
        return -1;
    }
    sample_time = MP4GetSampleTime(run->mp4_file, run->video_track, run->video_index);
    sample_time = sample_time * 1000 / run->video_timescale;
    iframe = 0;
    ret = MP4ReadSample(run->mp4_file, run->video_track, run->video_index,
                        &data, &size, &start_time, &duration, &offset, &iframe);
    if (!ret) {
        if (data) {
            free(data);
        }
        run->video_index++;
        return -1;
    }
    if (!run->i_frame_read && !iframe) {
        if (data) {
            free(data);
        }
        run->video_index++;
        return 0;
    }
    if (data) {
        start_time = start_time * 1000 / run->video_timescale;
        offset = offset * 1000 / run->video_timescale;
        if (run->stop) {
            if ((start_time / 1000 + run->start) > run->stop) {
                free(data);
                return -1;
            }
        }
        if ( /*!run->i_frame_read && */ iframe) {
            /********message body********/
            unsigned char *mdata = NULL;
            mdata = calloc((size + run->slen + run->plen + 8), 1);
            if (mdata == NULL) {
                log_goke(DEBUG_SERIOUS, "calloc error, size = %d", (size + run->slen + run->plen + 8));
                free(data);
                return -1;
            }
            p = mdata;
            memcpy(p, NAL, 4);
            p += 4;
            memcpy(p, run->sps, run->slen);
            p += run->slen;
            memcpy(p, NAL, 4);
            p += 4;
            memcpy(p, run->pps, run->plen);
            p += run->plen;
            memcpy(p, data, size);
            memcpy(p, NAL, 4);
            log_goke(DEBUG_INFO, "first key frame.");
            lv_stream_send_media_param_s param = {{0}};
            param.common.type = LV_STREAM_MEDIA_VIDEO;
            param.common.p = (char *) mdata;
            param.common.len = (size + run->slen + run->plen + 8);
            param.common.timestamp_ms = start_time + run->start * 1000 + 1000;
            param.video.format = LV_VIDEO_FORMAT_H264;
            param.video.key_frame = 1;
            ret = lv_stream_send_media(init->channel, &param);
            if (ret) {
                log_goke(DEBUG_VERBOSE, "aliyun player video send failed, ret=%x, service_id = %d", ret,
                         init->channel);
            }
            if( mdata ) {
                free(mdata);
            }
            run->i_frame_read = 1;
            free(data);
        } else {
            if (((data[4] & 0x1f) == 0x07) || ((data[4] & 0x1f) == 0x08)) { //not sending any psp pps
                free(data);
                run->video_index++;
                return 0;
            }
            if (!iframe && speed == 16) {
                free(data);
                run->video_index++;
                return 0;
            }
            data[0] = 0x00;
            data[1] = 0x00;
            data[2] = 0x00;
            data[3] = 0x01;

            lv_stream_send_media_param_s param = {{0}};
            param.common.type = LV_STREAM_MEDIA_VIDEO;
            param.common.p = (char *) data;
            param.common.len = size;
            param.common.timestamp_ms = start_time + run->start * 1000 + 1000;
            param.video.format = LV_VIDEO_FORMAT_H264;
            param.video.key_frame = 0;
            ret = lv_stream_send_media(init->channel, &param);
            if (ret) {
                log_goke(DEBUG_VERBOSE, "aliyun player video send failed, ret=%x, service_id = %d", ret,
                         init->channel);
            }
            if( data ) {
                free(data);
            }

            run->i_frame_read = 1;
            run->video_sync = start_time;
        }
    }
    if ((speed == 1) || (speed == 2) ||
        (speed == 4) || (speed == 0)) {
        run->video_index++;
    } else if (speed == 16) {
        run->video_index++;
    }
    return 0;
}

static int player_get_audio_frame(player_init_t *init, player_run_t *run, int speed) {
    unsigned char *data = NULL;
    unsigned char *p = NULL;
    unsigned int size = 0, num = 0, framesize;
    MP4Timestamp start_time;
    MP4Duration duration;
    MP4Duration offset;
    char iframe = 0;
    int ret = 0;
    int flag = 0;
    message_t msg;
    int sample_time;
    //*************************************
    if (!run->mp4_file) {
        return -1;
    }
    if (run->audio_index >= run->audio_frame_num) {
        return -1;
    }
    sample_time = MP4GetSampleTime(run->mp4_file, run->audio_track, run->audio_index);
    sample_time = sample_time * 1000 / run->audio_timescale;
    iframe = 0;
    ret = MP4ReadSample(run->mp4_file, run->audio_track, run->audio_index,
                        &data, &size, &start_time, &duration, &offset, &iframe);
    if (!ret) {
        if (data) {
            free(data);
        }
        run->audio_index++;
        return -1;
    }
    if (!run->i_frame_read) {
        if (data) free(data);
        run->audio_index++;
        return 0;
    }
    if (data) {
        start_time = start_time * 1000 / run->audio_timescale;
        offset = offset * 1000 / run->audio_timescale;
        if (run->stop) {
            if ((start_time / 1000 + run->start) > run->stop) {
                free(data);
                return -1;
            }
        }
        lv_stream_send_media_param_s param = {{0}};
        param.common.type = LV_STREAM_MEDIA_AUDIO;
        param.common.p = (char *)data;
        param.common.len = size;
        param.common.timestamp_ms = start_time + run->start * 1000 + 1000;;
        param.audio.format = LV_AUDIO_FORMAT_G711A;
        ret = lv_stream_send_media(init->channel, &param);
        if( ret) {
            log_goke(DEBUG_VERBOSE, "aliyun player audio send failed, ret=%x, service_id = %d", ret,
                     init->channel);
        }
        if (data) {
            free(data);
        }
        run->audio_sync = start_time;
    }
    if ((speed == 1) || (speed == 2) ||
        (speed == 4) || (speed == 0)) {
        run->audio_index++;
    } else if (speed == 16) {
        run->audio_index += 30;    //2xfps for audio
    }
    return 0;
}

static int player_open_mp4(player_init_t *init, player_run_t *run, file_list_node_t *fhead) {
    int i, num_tracks;
    int diff;
    unsigned long long int sample_time;
    unsigned char **sps_header = NULL;
    unsigned char **pps_header = NULL;
    unsigned int *sps_size = NULL;
    unsigned int *pps_size = NULL;
    if (run->current == NULL)
        run->current = fhead;
    else
        run->current = run->current->next;
    if (run->mp4_file == NULL) {
        memset(run->file_path, 0, sizeof(run->file_path));
        sprintf(run->file_path, "%s%s/%d_%d.mp4", manager_config.media_directory,
                manager_config.folder_prefix[init->type], run->current->start, run->current->stop);
    } else
        return 0;
    run->mp4_file = MP4Read(run->file_path);
    if (!run->mp4_file) {
        log_goke(DEBUG_SERIOUS, "Player Read error....%s", run->file_path);
        return -1;
    }
    log_goke(DEBUG_INFO, "opened file %s", run->file_path);
    if (run->current == fhead) {
        run->start = init->start;
    } else {
        run->start = run->current->start;
    }
    if (run->current->next == NULL)
        run->stop = init->stop;
    else
        run->stop = 0;
    run->video_track = MP4_INVALID_TRACK_ID;
    num_tracks = MP4GetNumberOfTracks(run->mp4_file, NULL, 0);
    for (i = 0; i < num_tracks; i++) {
        MP4TrackId id = MP4FindTrackId(run->mp4_file, i, NULL, 0);
        char *type = MP4GetTrackType(run->mp4_file, id);
        if (MP4_IS_VIDEO_TRACK_TYPE(type)) {
            run->video_track = id;
            run->duration = MP4GetTrackDuration(run->mp4_file, id);
            run->video_frame_num = MP4GetTrackNumberOfSamples(run->mp4_file, id);
            run->video_timescale = MP4GetTrackTimeScale(run->mp4_file, id);
            run->duration = (int) (run->duration / run->video_timescale);
            run->fps = MP4GetTrackVideoFrameRate(run->mp4_file, id);
            run->width = MP4GetTrackVideoWidth(run->mp4_file, id);
            run->height = MP4GetTrackVideoHeight(run->mp4_file, id);
            log_goke(DEBUG_INFO, "video codec = %s", MP4GetTrackMediaDataName(run->mp4_file, id));
            run->video_index = 1;
            //sps pps
            char result = MP4GetTrackH264SeqPictHeaders(run->mp4_file, id, &sps_header, &sps_size,
                                                        &pps_header, &pps_size);
            memset(run->sps, 0, sizeof(run->sps));
            run->slen = 0;
            for (i = 0; sps_size[i] != 0; i++) {
                if (strlen(run->sps) <= 0 && sps_size[i] > 0) {
                    memcpy(run->sps, sps_header[i], sps_size[i]);
                    run->slen = sps_size[i];
                }
                free(sps_header[i]);
            }
            free(sps_header);
            free(sps_size);
            memset(run->pps, 0, sizeof(run->pps));
            run->plen = 0;
            for (i = 0; pps_size[i] != 0; i++) {
                if (strlen(run->pps) <= 0 && pps_size[i] > 0) {
                    memcpy(run->pps, pps_header[i], pps_size[i]);
                    run->plen = pps_size[i];
                }
                free(pps_header[i]);
            }
            free(pps_header);
            free(pps_size);
        } else if (MP4_IS_AUDIO_TRACK_TYPE(type)) {
            run->audio_track = id;
            run->audio_frame_num = MP4GetTrackNumberOfSamples(run->mp4_file, id);
            run->audio_timescale = MP4GetTrackTimeScale(run->mp4_file, id);
//			run->audio_codec = MP4GetTrackAudioMpeg4Type( run->mp4_file, id);
            log_goke(DEBUG_INFO, "audio codec = %s", MP4GetTrackMediaDataName(run->mp4_file, id));
            run->audio_index = 1;
        }
    }
    //seek
//    diff = ( run->current->start - run->start ) * 1000;
    diff = (run->current->start - (run->start + init->offset)) * 1000;
    for (i = 1; i <= run->video_frame_num; i++) {
        sample_time = MP4GetSampleTime(run->mp4_file, run->video_track, i);
        sample_time = sample_time * 1000 / run->video_timescale;
        if ((int) (diff + sample_time) >= 0) {
            if (i > 1) run->video_index = i;
            else run->video_index = 1;
            break;
        }
    }
    for (i = 1; i <= run->audio_frame_num; i++) {
        sample_time = MP4GetSampleTime(run->mp4_file, run->audio_track, i);
        sample_time = sample_time * 1000 / run->audio_timescale;
        if ((int) (diff + sample_time) >= 0) {
            if (i > 1) run->audio_index = i;
            else run->audio_index = 1;
            break;
        }
    }
    return 0;
}

static int player_close_mp4(player_run_t *run) {
    int ret = 0;
    file_list_node_t *current;
    if (run->mp4_file) {
        MP4Close(run->mp4_file, 0);
        run->mp4_file = NULL;
        log_goke(DEBUG_INFO, "------closed file=======%s", run->file_path);
        current = run->current;
        memset(run, 0, sizeof(player_run_t));
        run->current = current;
    }
    return ret;
}

static int player_main(player_init_t *init, player_run_t *run, file_list_node_t *fhead, int speed) {
    int ret = 0;
    int ret_video, ret_audio;
    if (run->mp4_file == NULL) {
        ret = player_open_mp4(init, run, fhead);
        if (ret) {
            goto NEXT_STREAM;
        }
        init_visual_stream_config(init->channel, run->fps, run->duration);
    }
    if (!run->vstream_wait)
        ret_video = player_get_video_frame(init, run, speed);
    if (!run->astream_wait)
        ret_audio = player_get_audio_frame(init, run, speed);
    if (run->i_frame_read) {
        if (run->video_sync > run->audio_sync + DEFAULT_SYNC_DURATION) {
            run->vstream_wait = 1;
            run->astream_wait = 0;
        } else if (run->audio_sync > run->video_sync + DEFAULT_SYNC_DURATION) {
            run->astream_wait = 1;
            run->vstream_wait = 0;
        } else {
            run->astream_wait = 0;
            run->vstream_wait = 0;
        }
    }
    if (ret_video != 0 || ret_audio != 0) {
        run->astream_wait = 0;
        run->vstream_wait = 0;
    }
    if (ret_video && ret_audio) {
        goto NEXT_STREAM;
    }
    if (init->speed == 0)
        usleep(60000);
    else if (init->speed == 1)
        usleep(30000);
    else if (init->speed == 2)
        usleep(10000);
    else if (init->speed == 4)
        usleep(3000);
    else if (init->speed == 16)
        usleep(2000);
    return 0;
    NEXT_STREAM:
    player_close_mp4(run);
    if (run->current->next == NULL) {
        ret = -1;
    }
    return ret;
}

static int find_job_channel(int channel) {
    int i;
    for (i = 0; i < ALIYUN_VISUAL_MAX_CHANNEL; i++) {
        if (jobs[i].status > PLAYER_THREAD_NONE) {
            if (jobs[i].init.channel == channel) {
                return i;
            }
        }
    }
    return -1;
}

static int player_add_job(message_t *msg) {
    int i = -1, id = -1, num = 0;
    int ret = 0, same = 0;
    pthread_t pid;
    player_init_t *init = NULL;
    int file_ret = PLAYER_FILEFOUND;

    if (info.status != STATUS_RUN) {
        return -1;
    }

    pthread_rwlock_wrlock(&ilock);
    init = ((player_init_t *) msg->arg);
    if (init == NULL) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    if ( (strlen(init->filename) > 0) && (init->start == 0) ) {//by file name
        if (access(init->filename, F_OK)) {
            pthread_rwlock_unlock(&ilock);
            return -1;
        }
    } else {
        if (file_manager_check_file_list(init->type, init->start, init->stop)) {
            log_goke( DEBUG_WARNING, "not finding valid vod files!!!!!!!!!!!!!!!!!!!!!!!");
            pthread_rwlock_unlock(&ilock);
            return -1;
        }
    }
    id = find_job_channel(init->channel);
    if (id != -1) {
        same = 1;
    } else {
        for (i = 0; i < ALIYUN_VISUAL_MAX_CHANNEL; i++) {
            if (jobs[i].status == PLAYER_THREAD_NONE) {
                id = i;
                break;
            }
        }
    }
    if (id == -1) {
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    if (same) {
        memset(&(jobs[id].init), 0, sizeof(player_init_t));
        memcpy(&(jobs[id].init), init, sizeof(player_init_t));
        jobs[id].init.tid = id;
        jobs[id].status = PLAYER_THREAD_INITED;
        jobs[id].audio = init->audio;
        jobs[id].run = 1;
        jobs[id].speed = init->speed;
        jobs[id].restart = 1;
        jobs[id].exit = 0;
        jobs[id].type = init->type;
        log_goke(DEBUG_INFO, "player thread updated successful!");
    } else {
        //start the thread
        memset(&jobs[id], 0, sizeof(player_job_t));
        memcpy(&(jobs[id].init), init, sizeof(player_init_t));
        jobs[id].init.tid = id;
        ret = pthread_create(&pid, NULL, player_func, (void *) &(jobs[id].init));
        if (ret != 0) {
            log_goke(DEBUG_SERIOUS, "player thread create error! ret = %d", ret);
            pthread_rwlock_unlock(&ilock);
            return -1;
        }
        jobs[id].status = PLAYER_THREAD_INITED;
        jobs[id].channel = init->channel;
        jobs[id].sid = id;
        jobs[id].audio = init->audio;
        jobs[id].run = 1;       //temporary
        jobs[id].speed = init->speed;
        jobs[id].restart = 0;
        jobs[id].exit = 0;
        jobs[id].type = init->type;
        log_goke(DEBUG_INFO, "player thread create successful!");
    }
    pthread_rwlock_unlock(&ilock);
    return ret;
}

static int player_start_job(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].run = 1;
    pthread_rwlock_unlock(&ilock);
    return 0;
}

static int player_stop_job(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].exit = 1;
    pthread_rwlock_unlock(&ilock);
    return 0;
}

static int player_pause_job(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].pause = 1;
    pthread_rwlock_unlock(&ilock);
    return 0;
}

static int player_unpause_job(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].pause = 0;
    pthread_rwlock_unlock(&ilock);
    return 0;
}
static int player_start_audio(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    if (jobs[id].status == PLAYER_THREAD_NONE) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].audio = 1;
    pthread_rwlock_unlock(&ilock);
    return 0;
}

static int player_stop_audio(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    if (jobs[id].status == PLAYER_THREAD_NONE) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].audio = 0;
    pthread_rwlock_unlock(&ilock);
    return 0;
}

static int player_seek_job(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].status = PLAYER_THREAD_INITED;
    jobs[id].run = 1;
    jobs[id].init.offset = msg->arg_in.cat;
    jobs[id].restart = 1;
    jobs[id].exit = 0;
    log_goke(DEBUG_INFO, "player thread updated successful!");
    pthread_rwlock_unlock(&ilock);
    return 0;
}

static int player_set_param_job(message_t *msg) {
    int id = -1;
    id = find_job_channel(msg->arg_in.wolf);
    if (id == -1) {
        return -1;
    }
    pthread_rwlock_wrlock(&ilock);
    jobs[id].status = PLAYER_THREAD_INITED;
    jobs[id].run = 1;
    jobs[id].init.speed = msg->arg_in.chick;
    jobs[id].init.offset = msg->arg_in.cat;
    jobs[id].init.keyonly = msg->arg_in.dog;
    jobs[id].restart = 1;
    jobs[id].exit = 0;
    log_goke(DEBUG_INFO, "player thread updated successful!");
    pthread_rwlock_unlock(&ilock);
    return 0;
}

static int *player_func(void *arg) {
    int tid;
    player_run_t run;
    char fname[MAX_SYSTEM_STRING_SIZE];
    message_t msg;
    file_list_node_t *fhead = NULL;
    player_init_t init;
    int status, send_finish = 0;
    int st, play, restart, speed, pause;
    //thread preparation
    pthread_detach(pthread_self());
    pthread_rwlock_wrlock(&ilock);
    memset(&init, 0, sizeof(player_init_t));
    memcpy(&init, (player_init_t *) (arg), sizeof(player_init_t));
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    tid = init.tid;
    misc_set_bit(&info.thread_start, tid);
    sprintf(fname, "%d%d-%d", tid, init.channel, time_get_now());
    misc_set_thread_name(fname);
    pthread_rwlock_unlock(&ilock);
    log_goke(DEBUG_INFO, "------------add new player----------------");
    log_goke(DEBUG_INFO, "now=%lld", time_get_now());
    log_goke(DEBUG_INFO, "start=%lld", init.start);
    log_goke(DEBUG_INFO, "end=%lld", init.stop);
    log_goke(DEBUG_INFO, "--------------------------------------------------");
    //data init
    memset(&run, 0, sizeof(player_run_t));
    status = PLAYER_THREAD_INITED;
    /*
     * send back player request success response here
     *
     */
    while (1) {
        pthread_rwlock_wrlock(&ilock);
        if (info.exit || jobs[tid].exit) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        if (misc_get_bit(info.thread_exit, tid)) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        play = jobs[tid].run;
        restart = jobs[tid].restart;
        speed = jobs[tid].speed;
        pause = jobs[tid].pause;
        if (restart) {
            memcpy(&init, (player_init_t *) &(jobs[tid].init), sizeof(player_init_t));
            if (status >= PLAYER_THREAD_RUN) {
                player_close_mp4(&run);
                memset(&run, 0, sizeof(run));
                file_manager_node_clear(&fhead);
            }
            status = PLAYER_THREAD_INITED;
            jobs[tid].restart = 0;
        }
        pthread_rwlock_unlock(&ilock);
        switch (status) {
            case PLAYER_THREAD_INITED:
                if ( (strlen(init.filename) > 0) && (init.start == 0) ) {
                    if(file_manager_search_file( init.filename, manager_config.timezone, &fhead) != 0) {
                        status = PLAYER_THREAD_ERROR;
                    }
                }
                else {
                    if (file_manager_search_file_list(init.type, init.start, init.stop, &fhead) != 0) {
                        status = PLAYER_THREAD_ERROR;
                    }
                }
                if( status != PLAYER_THREAD_ERROR ) {
                    status = PLAYER_THREAD_SYNC;
                    /********message body********/
                    msg_init(&msg);
                    msg.message = MSG_PLAYER_REQUEST_SYNC;
                    msg.sender = msg.receiver = SERVER_PLAYER;
                    msg.arg_in.cat = init.channel;
                    msg.arg_in.dog = tid;
                    global_common_send_message(SERVER_VIDEO, &msg);
                    global_common_send_message(SERVER_AUDIO, &msg);
                    /***************************/
                    log_goke(DEBUG_INFO, "player initialized!");
                }
                break;
            case PLAYER_THREAD_SYNC:
                if (misc_full_bit(jobs[tid].live_sync, PLAYER_SYNC_BUTT)) {
                    status = PLAYER_THREAD_IDLE;
                    jobs[tid].live_sync = 0;
                }
                break;
            case PLAYER_THREAD_IDLE:
                if (play) {
                    status = PLAYER_THREAD_RUN;
                    log_goke(DEBUG_INFO, "player started!");
                }
                break;
            case PLAYER_THREAD_RUN:
                if (pause) {
                    status = PLAYER_THREAD_PAUSE;
                    log_goke(DEBUG_INFO, "player paused!");
                } else {
                    if (player_main(&init, &run, fhead, speed)) {
                        status = PLAYER_THREAD_FINISH;
                        log_goke(DEBUG_INFO, "player finished!");
                    }
                }
                break;
            case PLAYER_THREAD_PAUSE:
                if (!pause) {
                    status = PLAYER_THREAD_RUN;
                    log_goke(DEBUG_INFO, "player unpaused!");
                }
                break;
            case PLAYER_THREAD_FINISH:
                log_goke(DEBUG_INFO, "finished the playback for thread = %d", tid);
                info.status = PLAYER_THREAD_INITED;
                pthread_rwlock_wrlock(&ilock);
                jobs[tid].exit = 1;
                pthread_rwlock_unlock(&ilock);
                send_finish = 1;
                break;
            case PLAYER_THREAD_ERROR:
                log_goke(DEBUG_SERIOUS, "error within thread = %d", tid);
                info.status = PLAYER_THREAD_INITED;
                pthread_rwlock_wrlock(&ilock);
                jobs[tid].exit = 1;
                pthread_rwlock_unlock(&ilock);
                break;
        }
    }
    //release
    file_manager_node_clear(&fhead);
    if (run.mp4_file) {
        MP4Close(run.mp4_file, 0);
        run.mp4_file = NULL;
        log_goke(DEBUG_INFO, "------closed file=======%s", run.file_path);
    }
    if( send_finish ) {
        //***send out aliyun vod finish cmd
        int ret = lv_stream_send_cmd(init.channel, LV_STORAGE_RECORD_COMPLETE);
        log_goke( DEBUG_INFO, " vod file player finished with ret = %d", ret);
    }
    //***restart live stream if required
    if( 0 ) {
        msg_init(&msg);
        msg.message = MSG_VIDEO_START;
        msg.sender = msg.receiver = SERVER_ALIYUN;
        msg.arg_in.wolf = msg.arg_pass.wolf = init.channel;
        global_common_send_message( SERVER_VIDEO, &msg);
        msg.message = MSG_AUDIO_START;
        global_common_send_message( SERVER_AUDIO, &msg);
    }
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, tid);
    memset(&jobs[tid], 0, sizeof(player_job_t));
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_PLAYER);
    log_goke(DEBUG_INFO, "-----------thread exit: player %s-----------", fname);
    pthread_exit(0);
}

static void server_thread_termination(void) {
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_PLAYER_SIGINT;
    msg.sender = msg.receiver = SERVER_PLAYER;
    global_common_send_message(SERVER_MANAGER, &msg);
    /****************************/
}

static void player_broadcast_thread_exit(void) {
}

static void server_release_1(void) {
}

static void server_release_2(void) {
    msg_buffer_release2(&message, &mutex);
    memset(&jobs, 0, sizeof(jobs));
}

static void server_release_3(void) {
    msg_free(&info.task.msg);
    memset(&info, 0, sizeof(server_info_t));
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_MANAGER_EXIT_ACK;
    msg.sender = SERVER_PLAYER;
    global_common_send_message(SERVER_MANAGER, &msg);
    /***************************/
}

/*
 *
 */
static int player_message_filter(message_t *msg) {
    int ret = 0;
    if (info.status >= STATUS_ERROR) { //only system message
        if (!msg_is_system(msg->message) && !msg_is_response(msg->message)) {
            return 1;
        }
    }
    return ret;
}

static int server_message_proc(void) {
    int ret = 0;
    message_t msg, send_msg;
    char name[MAX_SYSTEM_STRING_SIZE];
    //condition
    pthread_mutex_lock(&mutex);
    if (message.head == message.tail) {
        if ((info.status == info.old_status)) {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    if (info.msg_lock) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    msg_init(&msg);
    ret = msg_buffer_pop(&message, &msg);
    pthread_mutex_unlock(&mutex);
    if (ret == 1)
        return 0;
    if( player_message_filter(&msg) ) {
        log_goke(DEBUG_MAX, "PLAYER message filtered: sender=%s, message=%s, head=%d, tail=%d was screened",
                 global_common_get_server_name(msg.sender),
                 global_common_message_to_string(msg.message), message.head, message.tail);
        msg_free(&msg);
        return -1;
    }
    log_goke(DEBUG_MAX, "PLAYER message popped: sender=%s, message=%s, head=%d, tail=%d",
             global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
    switch (msg.message) {
        case MSG_PLAYER_REQUEST:
            msg_init(&info.task.msg);
            msg_deep_copy(&info.task.msg, &msg);
            player_add_job(&msg);
            break;
        case MSG_PLAYER_START:
            player_start_job(&msg);
            break;
        case MSG_PLAYER_STOP:
            player_stop_job(&msg);
            break;
        case MSG_PLAYER_PAUSE:
            player_pause_job(&msg);
            break;
        case MSG_PLAYER_UNPAUSE:
            player_unpause_job(&msg);
            break;
        case MSG_PLAYER_AUDIO_START:
            player_start_audio(&msg);
            break;
        case MSG_PLAYER_AUDIO_STOP:
            player_stop_audio(&msg);
            break;
        case MSG_PLAYER_SEEK:
            player_seek_job(&msg);
            break;
        case MSG_PLAYER_SET_PARAM:
            player_set_param_job(&msg);
            break;
        case MSG_MANAGER_EXIT:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case MSG_MANAGER_TIMER_ON:
            ((HANDLER) msg.arg_in.handler)();
            break;
        case MSG_PLAYER_PROPERTY_SET:
            break;
        case MSG_MANAGER_EXIT_ACK:
            misc_clear_bit(&info.quit_status, msg.sender);
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "PLAYER: server %s quit and quit lable now = %s",
                     global_common_get_server_name(msg.sender),
                     fp);
            if(fp) {
                free(fp);
            }
            break;
        case MSG_PLAYER_RELAY:
            msg_init(&send_msg);
            send_msg.message = msg.message | 0x1000;
            send_msg.sender = send_msg.receiver = SERVER_PLAYER;
            memcpy(&send_msg.arg_pass, &msg.arg_pass, sizeof(message_arg_t));
            memcpy(&send_msg.arg_in, &msg.arg_in, sizeof(message_arg_t));
            global_common_send_message(msg.receiver, &send_msg);
            break;
        case MSG_ALIYUN_TIME_SYNCHRONIZED:
            misc_set_bit( &info.init_status, PLAYER_INIT_CONDITION_TIME_SYCHRONIZED);
            break;
        case MSG_ALIYUN_GET_TIME_SYNC_ACK:
            if( msg.arg_in.cat == 1 ) {
                misc_set_bit( &info.init_status, PLAYER_INIT_CONDITION_TIME_SYCHRONIZED);
            } else {
                misc_clear_bit( &info.init_status, PLAYER_INIT_CONDITION_TIME_SYCHRONIZED);
            }
            break;
        case MSG_MANAGER_DUMMY:
            break;
        case MSG_DEVICE_SD_FORMAT_START:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.status = STATUS_REINIT;
            break;
        case MSG_DEVICE_GET_PARA_ACK:
            if (msg.arg_in.cat == DEVICE_PARAM_SD_INFO) {
                if (msg.arg_in.dog) {
                    misc_set_bit(&info.init_status, PLAYER_INIT_CONDITION_DEVICE_SD); //fake device response
                }
            }
            break;
        case MSG_PLAYER_REQUEST_SYNC_ACK:
            if( msg.sender == SERVER_VIDEO ) {
                if( msg.result == 0 ) {
                    pthread_rwlock_wrlock(&ilock);
                    misc_set_bit( &jobs[msg.arg_in.dog].live_sync, PLAYER_SYNC_VIDEO);
                    pthread_rwlock_unlock(&ilock);
                }
            } else if( msg.sender == SERVER_AUDIO ) {
                if( msg.result == 0 ) {
                    pthread_rwlock_wrlock(&ilock);
                    misc_set_bit( &jobs[msg.arg_in.dog].live_sync, PLAYER_SYNC_AUDIO);
                    pthread_rwlock_unlock(&ilock);
                }
            }
            break;
        default:
            log_goke(DEBUG_SERIOUS, "PLAYER not support message %s",
                     global_common_message_to_string(msg.message));
            break;
    }
    msg_free(&msg);
    return ret;
}

/*
 *
 */
static int server_init(void) {
    int ret = 0;
    message_t msg;
    if (!misc_get_bit(info.init_status, PLAYER_INIT_CONDITION_DEVICE_SD)) {
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_DEVICE_GET_PARA;
        msg.sender = msg.receiver = SERVER_PLAYER;
        msg.arg_in.cat = DEVICE_PARAM_SD_INFO;
        ret = global_common_send_message(SERVER_DEVICE, &msg);
        /***************************/
        sleep(1);
    }
    if (!misc_get_bit(info.init_status, PLAYER_INIT_CONDITION_TIME_SYCHRONIZED)) {
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_ALIYUN_GET_TIME_SYNC;
        msg.sender = msg.receiver = SERVER_PLAYER;
        ret = global_common_send_message(SERVER_ALIYUN, &msg);
        /***************************/
        sleep(1);
    }
    if (misc_full_bit(info.init_status, PLAYER_INIT_CONDITION_NUM)) {
        info.status = STATUS_WAIT;
        info.tick = 0;
    }
    return ret;
}

int server_setup(void) {
    info.status = STATUS_IDLE;
    return 0;
}

int server_start(void) {
    info.status = STATUS_RUN;
    return 0;
}

int server_stop(void) {
    return 0;
}

int server_restart(void) {
    int i;
    for (i = 0; i < ALIYUN_VISUAL_MAX_CHANNEL; i++) {
        misc_set_bit(&jobs[i].exit, i);
    }
    global_common_send_dummy(SERVER_PLAYER);
    while( info.thread_start != 0) {
        usleep(MESSAGE_RESENT_SLEEP);       //!!!possible dead lock here
    }
    info.status = STATUS_WAIT;
    return 0;
}

int server_reinit(void) {
    int i;
    for (i = 0; i < ALIYUN_VISUAL_MAX_CHANNEL; i++) {
        misc_set_bit(&jobs[i].exit, i);
    }
    global_common_send_dummy(SERVER_PLAYER);
    while( info.thread_start != 0) {
        usleep(MESSAGE_RESENT_SLEEP);       //!!!possible dead lock here
    }
    info.status = STATUS_NONE;
    log_goke( DEBUG_INFO, "server player reinit success and enter init...");
    sd_set_response( E_SD_ACTION_FORMAT, SERVER_PLAYER );
    memset(&info, 0, sizeof(info));
}

/*
 * task proc
 */
static void task_proc(void) {
    //default
    if (info.status == STATUS_NONE) {
        info.status = STATUS_INIT;
    } else if (info.status == STATUS_WAIT) {
        info.status = STATUS_SETUP;
    } else if (info.status == STATUS_IDLE) {
        info.status = STATUS_START;
    }
    return;
}

/*
 * state machine
 */
static void state_machine(void) {
    switch (info.status) {
        case STATUS_NONE:
        case STATUS_WAIT:
        case STATUS_IDLE:
        case STATUS_RUN:
            break;
        case STATUS_INIT:
            server_init();
            break;
        case STATUS_SETUP:
            server_setup();
            break;
        case STATUS_START:
            server_start();
            break;
        case STATUS_STOP:
            server_stop();
            break;
        case STATUS_RESTART:
            server_restart();
            break;
        case STATUS_REINIT:
            server_reinit();
            break;
        case STATUS_ERROR:
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case EXIT_INIT:
            log_goke(DEBUG_INFO, "PLAYER: switch to exit task!");
            if (info.task.msg.sender == SERVER_MANAGER) {
                info.quit_status &= (info.task.msg.arg_in.cat);
            } else {
                info.quit_status = 0;
            }
            char *fp = global_common_get_server_names( info.quit_status );
            log_goke(DEBUG_INFO, "PLAYER: Exit precondition =%s", fp);
            if( fp ) {
                free(fp);
            }
            info.status = EXIT_SERVER;
            break;
        case EXIT_SERVER:
            if (!info.quit_status)
                info.status = EXIT_STAGE1;
            break;
        case EXIT_STAGE1:
            server_release_1();
            info.status = EXIT_THREAD;
            break;
        case EXIT_THREAD:
            info.thread_exit = info.thread_start;
            player_broadcast_thread_exit();
            if (!info.thread_start)
                info.status = EXIT_STAGE2;
            break;
        case EXIT_STAGE2:
            server_release_2();
            info.status = EXIT_FINISH;
            break;
        case EXIT_FINISH:
            info.exit = 1;
            info.status = STATUS_NONE;
            break;
        default:
            log_goke(DEBUG_SERIOUS, "PLAYER !!!!!!!unprocessed server status in state_machine = %d", info.status);
            break;
    }
    return;
}


/*
 * server entry point
 */
static void *server_func(void) {
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    misc_set_thread_name("server_player");
    pthread_detach(pthread_self());
    msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_small, &mutex);
    info.quit_status = PLAYER_EXIT_CONDITION;
    info.init = 1;
    while (!info.exit) {
        info.old_status = info.status;
        state_machine();
        task_proc();
        server_message_proc();
    }
    server_release_3();
    log_goke(DEBUG_INFO, "-----------thread exit: server_player-----------");
    pthread_exit(0);
}

/*
 * internal interface
 */

/*
 * external interface
 */
int server_player_start(void) {
    int ret = -1;
    ret = pthread_create(&info.id, NULL, server_func, NULL);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "player server create error! ret = %d", ret);
        return ret;
    } else {
        log_goke(DEBUG_INFO, "player server create successful!");
        return 0;
    }
}

int server_player_message(message_t *msg) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    if (!message.init) {
        log_goke(DEBUG_MAX, "PLAYER server is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message) );
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    ret = msg_buffer_push(&message, msg);
    log_goke(DEBUG_MAX, "PLAYER message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret,message.head, message.tail);
    if (ret != 0)
        log_goke(DEBUG_WARNING, "PLAYER message push in error =%d", ret);
    else {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}
