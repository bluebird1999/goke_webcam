/*
 * header
 */
//system header
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <malloc.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/mount.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
//program header
#include "../../server/manager/manager_interface.h"
#include "../../common/tools_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/video/video_interface.h"
#include "../../server/device/device_interface.h"
#include "../device/file_manager.h"
#include "../../server/goke/goke_interface.h"
#include "../../server/aliyun/aliyun_interface.h"
#include "../../server/device/sd_control.h"
//server header
#include "recorder.h"
#include "recorder_interface.h"

/*
 * static
 */
//variable
static message_buffer_t message;
static server_info_t info;
static message_buffer_t video_buff[MAX_RECORDER_JOB];
static message_buffer_t audio_buff[MAX_RECORDER_JOB];
static recorder_job_t jobs[MAX_RECORDER_JOB];
static pthread_rwlock_t ilock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t vmutex[MAX_RECORDER_JOB] = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER,
                                                   PTHREAD_MUTEX_INITIALIZER};
static pthread_cond_t vcond[MAX_RECORDER_JOB] = {PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER,
                                                 PTHREAD_COND_INITIALIZER};
//function
//common
static void *server_func(void);

static int server_message_proc(void);

static void server_release_1(void);

static void server_release_2(void);

static void server_release_3(void);

static void server_thread_termination(int sign);

//specific
static int recorder_add_job(message_t *msg);

static int count_job_number(void);

static int *recorder_func(void *arg);

static int recorder_thread_init_mp4v2(recorder_job_t *ctrl);

static int recorder_thread_write_mp4_video(recorder_job_t *ctrl, unsigned char *data, int length, av_data_info_t *info);

static int recorder_thread_close(recorder_job_t *ctrl);

static int recorder_thread_check_finish(recorder_job_t *ctrl);

static int recorder_thread_error(recorder_job_t *ctrl);

static int recorder_thread_pause(recorder_job_t *ctrl);

static int recorder_thread_start_stream(recorder_job_t *ctrl);

static int recorder_thread_stop_stream(recorder_job_t *ctrl, int real_stop);

static int recorder_thread_check_and_exit_stream(recorder_job_t *ctrl, int real_stop);

static int recorder_start_init_recorder_job(void);

static int recorder_set_property(message_t *msg);

static void recorder_broadcast_session_exit(void);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static int recorder_set_property(message_t *msg) {
    int ret = 0;
    int temp;
    //
    if (msg->arg_in.cat == RECORDER_PROPERTY_SWITCH) {
        temp = *((int *) (msg->arg));
        if (_config_.recorder_enable != temp) {
            if( temp == 0) {
                info.status = STATUS_STOP;
            } else {
                info.status = STATUS_START;
            }
            _config_.recorder_enable = temp;
            config_set(&_config_);  //save
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.recorder_enable);
    } else {
        log_goke(DEBUG_WARNING, " Not support recorder property: =%d", msg->arg_in.cat);
    }
    return ret;
}

static int recorder_start_init_recorder_job(void) {
    message_t msg;
    recorder_init_t init;
    int ret = 0;
    if (!info.status2) {
        msg_init(&msg);
        msg.message = MSG_RECORDER_ADD;
        msg.sender = msg.receiver = SERVER_RECORDER;
        init.video_channel = 0;
        init.mode = RECORDER_MODE_BY_TIME;
        init.type = LV_STORAGE_RECORD_INITIATIVE;
        init.audio = recorder_config.normal_audio;
        memcpy(&(init.start), recorder_config.normal_start, strlen(recorder_config.normal_start));
        memcpy(&(init.stop), recorder_config.normal_end, strlen(recorder_config.normal_end));
        init.repeat = recorder_config.normal_repeat;
        init.repeat_interval = recorder_config.normal_repeat_interval;
        init.quality = recorder_config.normal_quality;
        msg.arg = &init;
        msg.arg_size = sizeof(recorder_init_t);
        ret = recorder_add_job(&msg);
        if (!ret) {
            info.status2 = 1;
        }
    }
    return ret;
}

static int recorder_thread_error(recorder_job_t *ctrl) {
    int ret = 0;
    log_goke(DEBUG_SERIOUS, "errors in this recorder thread, stop!");
    ctrl->run.exit = 1;
    return ret;
}

static int recorder_thread_start_stream(recorder_job_t *ctrl) {
    message_t msg;
    /********message body********/
    msg_init(&msg);
    msg.sender = msg.receiver = SERVER_RECORDER;
    msg.arg_in.wolf = ctrl->t_id;
    msg.arg_pass.wolf = ctrl->t_id;
    msg.arg_in.cat = ctrl->init.quality;
    msg.message = MSG_VIDEO_START;
    if (global_common_send_message(SERVER_VIDEO, &msg) != 0) {
        log_goke(DEBUG_SERIOUS, "video start failed from recorder!");
    }
    if (ctrl->init.audio) {
        msg.message = MSG_AUDIO_START;
        if (global_common_send_message(SERVER_AUDIO, &msg) != 0) {
            log_goke(DEBUG_SERIOUS, "audio start failed from recorder!");
        }
    }
}

static int recorder_thread_stop_stream(recorder_job_t *ctrl, int real_stop) {
    message_t msg;
    /********message body********/
    memset(&msg, 0, sizeof(message_t));
    msg.arg_in.wolf = ctrl->t_id;
    msg.arg_in.duck = real_stop;
    msg.arg_in.cat = ctrl->init.quality;
    msg.message = MSG_VIDEO_STOP;
    msg.sender = msg.receiver = SERVER_RECORDER;
    if (global_common_send_message(SERVER_VIDEO, &msg) != 0) {
        log_goke(DEBUG_WARNING, "video stop failed from recorder!");
    }
    if (ctrl->init.audio) {
        memset(&msg, 0, sizeof(message_t));
        msg.message = MSG_AUDIO_STOP;
        msg.sender = msg.receiver = SERVER_RECORDER;
        msg.arg_in.wolf = ctrl->t_id;
        msg.arg_in.duck = real_stop;
        if (global_common_send_message(SERVER_AUDIO, &msg) != 0) {
            log_goke(DEBUG_WARNING, "audio stop failed from recorder!");
        }
    }
}

static int recorder_thread_check_finish(recorder_job_t *ctrl) {
    int ret = 0;
    long long int now = 0;
    now = time_get_now_stamp();
    if (now >= ctrl->run.stop)
        ret = 1;
    return ret;
}

static int recorder_thread_close(recorder_job_t *ctrl) {
    char oldname[MAX_SYSTEM_STRING_SIZE * 4];
    char snapname[MAX_SYSTEM_STRING_SIZE * 4];
    char start[MAX_SYSTEM_STRING_SIZE * 2];
    char stop[MAX_SYSTEM_STRING_SIZE * 2];
    char alltime[MAX_SYSTEM_STRING_SIZE * 4];
    message_t msg;
    int ret = 0;
    if (ctrl->run.mp4_file != MP4_INVALID_FILE_HANDLE) {
        log_goke(DEBUG_INFO, "+++MP4Close");
        MP4Close(ctrl->run.mp4_file, MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
        ctrl->run.mp4_file = MP4_INVALID_FILE_HANDLE;
    } else {
        return -1;
    }

    memset(oldname, 0, sizeof(oldname));
    memset(snapname, 0, sizeof(snapname));
    strcpy(oldname, ctrl->run.file_path);
    memset(start, 0, sizeof(start));
    time_stamp_to_date_with_zone(ctrl->run.start, start, 80, manager_config.timezone);
    sprintf(snapname, "%s%s/snap-%s.jpg", manager_config.media_directory,
            manager_config.folder_prefix[ ctrl->init.type], start);
    if ((ctrl->run.last_write - ctrl->run.real_start) < recorder_config.min_length) {
        log_goke(DEBUG_WARNING, "Recording file %s is too short, removed!", ctrl->run.file_path);
        //remove file here.
        remove(ctrl->run.file_path);
        //remove snapshot file as well
        if ((access(snapname, F_OK)) == 0) {
            remove(snapname);
        }
        return -1;
    }
    ctrl->run.real_stop = ctrl->run.last_write;
    memset(start, 0, sizeof(start));
    memset(stop, 0, sizeof(stop));
    time_stamp_to_date_with_zone(ctrl->run.real_start, start, 80, manager_config.timezone);
    time_stamp_to_date_with_zone(ctrl->run.last_write, stop, 80, manager_config.timezone);
    memset(ctrl->run.file_path, 0, sizeof(ctrl->run.file_path));
    sprintf(ctrl->run.file_path, "%s%s/%s_%s.jpg", manager_config.media_directory,
            manager_config.folder_prefix[ ctrl->init.type], start, stop);
    ret = rename(snapname, ctrl->run.file_path);
    memset(ctrl->run.file_path, 0, sizeof(ctrl->run.file_path));
    sprintf(ctrl->run.file_path, "%s%s/%s_%s.mp4", manager_config.media_directory,
            manager_config.folder_prefix[ ctrl->init.type], start, stop);
    ret|= rename(oldname, ctrl->run.file_path);
    if (ret) {
        log_goke(DEBUG_WARNING, "rename recording file %s to %s failed.", oldname, ctrl->run.file_path);
    } else {
        file_manager_add_file(ctrl->init.type, ctrl->run.real_start, &ctrl->run.last_write);
        /***************************/
        log_goke(DEBUG_INFO, "Record file is %s", ctrl->run.file_path);
    }
    return ret;
}

static int recorder_thread_write_mp4_video(recorder_job_t *ctrl, unsigned char *p_data,
                                           int data_length, av_data_info_t *avinfo) {
    nalu_unit_t nalu;
    int ret;
    int frame_len;
    MP4Duration duration, offset;
    memset(&nalu, 0, sizeof(nalu_unit_t));
    int pos = 0, len = 0;
    while ((len = h264_read_nalu(p_data, data_length, pos, &nalu)) != 0) {
        switch (nalu.type) {
            case 0x07:
                if (!ctrl->run.sps_read) {
                    ctrl->run.video_track = MP4AddH264VideoTrack(ctrl->run.mp4_file, 90000,
                                                                 MP4_INVALID_DURATION,
                                                                 avinfo->width,
                                                                 avinfo->height,
                                                                 nalu.data[1], nalu.data[2], nalu.data[3], 3);
                    if (ctrl->run.video_track == MP4_INVALID_TRACK_ID) {
                        return -1;
                    }
                    ctrl->run.sps_read = 1;
                    MP4SetVideoProfileLevel(ctrl->run.mp4_file, 0x7F);
                    MP4AddH264SequenceParameterSet(ctrl->run.mp4_file, ctrl->run.video_track, nalu.data, nalu.size);
                }
                break;
            case 0x08:
                if (ctrl->run.pps_read) break;
                if (!ctrl->run.sps_read) break;
                ctrl->run.pps_read = 1;
                MP4AddH264PictureParameterSet(ctrl->run.mp4_file, ctrl->run.video_track, nalu.data, nalu.size);
                break;
            case 0x1:
            case 0x5:
                if (!ctrl->run.sps_read || !ctrl->run.pps_read) {
                    return -2;
                }
                int nlength = nalu.size + 4;
                unsigned char *data = (unsigned char *) malloc(nlength);
                if (!data) {
                    log_goke(DEBUG_SERIOUS, "mp4_video_frame_write malloc failed");
                    return -1;
                }
                data[0] = nalu.size >> 24;
                data[1] = nalu.size >> 16;
                data[2] = nalu.size >> 8;
                data[3] = nalu.size & 0xff;
                memcpy(data + 4, nalu.data, nalu.size);
                int key = (nalu.type == 0x5 ? 1 : 0);
                if (!key && !ctrl->run.first_frame) {
                    free(data);
                    return -2;
                }
                if (ctrl->run.first_frame) {
                    duration = (avinfo->timestamp - ctrl->run.last_vframe_stamp) * 90000 / 1000;
                    offset = (avinfo->timestamp - ctrl->run.first_frame_stamp) * 90000 / 1000;
                } else {
                    duration = 90000 / avinfo->fps;
                    offset = 0;
                }
                ret = MP4WriteSample(ctrl->run.mp4_file, ctrl->run.video_track, data, nlength,
                                     duration, 0, key);
                if (!ret) {
                    free(data);
                    return -1;
                }
                if (!ctrl->run.first_frame && key) {
                    ctrl->run.real_start = time_get_now_stamp();
                    ctrl->run.fps = avinfo->fps;
                    ctrl->run.width = avinfo->width;
                    ctrl->run.height = avinfo->height;
                    ctrl->run.first_frame_stamp = avinfo->timestamp;
                    ctrl->run.first_frame = 1;
                }
                ctrl->run.last_write = time_get_now_stamp();
                ctrl->run.last_vframe_stamp = avinfo->timestamp;
                free(data);
                break;
            default :
                break;
        }
        pos += len;
    }
    return 0;
}

static int recorder_thread_init_mp4v2(recorder_job_t *ctrl) {
    int ret = 0;
    char fname[MAX_SYSTEM_STRING_SIZE * 2];
    char timestr[MAX_SYSTEM_STRING_SIZE];
    memset(fname, 0, sizeof(fname));
    time_stamp_to_date_with_zone(ctrl->run.start, timestr, 80, manager_config.timezone);
    sprintf(fname, "%s%s/%s-%s", manager_config.media_directory, manager_config.folder_prefix[ctrl->init.type],
            manager_config.folder_prefix[ctrl->init.type], timestr);
    ctrl->run.mp4_file = MP4CreateEx(fname, 0, 1, 1, 0, 0, 0, 0);
    if (ctrl->run.mp4_file == MP4_INVALID_FILE_HANDLE) {
        printf("MP4CreateEx file failed.");
        return -1;
    }
    //set time scale
    MP4SetTimeScale(ctrl->run.mp4_file, 90000);
    if (ctrl->init.audio) {
        ctrl->run.audio_track = MP4AddALawAudioTrack(ctrl->run.mp4_file,
                                                     recorder_config.quality[ctrl->init.quality].audio_sample);
        if (ctrl->run.audio_track == MP4_INVALID_TRACK_ID) {
            printf("add audio track failed.");
            return -1;
        }
        MP4SetTrackIntegerProperty(ctrl->run.mp4_file, ctrl->run.audio_track, "mdia.minf.stbl.stsd.alaw.channels", 1);
        MP4SetTrackIntegerProperty(ctrl->run.mp4_file, ctrl->run.audio_track, "mdia.minf.stbl.stsd.alaw.sampleSize",16);
        MP4SetAudioProfileLevel(ctrl->run.mp4_file, 0x02);
    }
    memset(ctrl->run.file_path, 0, sizeof(ctrl->run.file_path));
    strcpy(ctrl->run.file_path, fname);
    //snapshot
    memset(fname, 0, sizeof(fname));
    sprintf(fname, "%s%s/snap-%s.jpg", manager_config.media_directory,
            manager_config.folder_prefix[ctrl->init.type], timestr);
    /**********************************************/
    message_t msg;
    msg_init(&msg);
    msg.sender = msg.receiver = SERVER_RECORDER;
    msg.arg_in.cat = SNAP_TYPE_NORMAL;
    msg.arg = fname;
    msg.arg_size = strlen(fname) + 1;
    server_video_snap_message(&msg);
    /**********************************************/
    return ret;
}

static int recorder_thread_pause(recorder_job_t *ctrl) {
    int ret = 0;
    long long int temp1 = 0, temp2 = 0;
    if (ctrl->init.repeat == 0) {
        ctrl->run.exit = 1;
        return 0;
    } else {
        temp1 = ctrl->run.real_stop;
        temp2 = ctrl->run.stop - ctrl->run.start;
        memset(&ctrl->run, 0, sizeof(recorder_run_t));
        if (temp1 == 0)
            temp1 = time_get_now_stamp();
        ctrl->run.start = temp1 + ctrl->init.repeat_interval;
        ctrl->run.stop = ctrl->run.start + temp2;
        ctrl->status = RECORDER_THREAD_STARTED;
        log_goke(DEBUG_SERIOUS, "-------------add recursive recorder---------------------");
        log_goke(DEBUG_SERIOUS, "now=%lld", time_get_now_stamp());
        log_goke(DEBUG_SERIOUS, "start=%lld", ctrl->run.start);
        log_goke(DEBUG_SERIOUS, "end=%lld", ctrl->run.stop);
        log_goke(DEBUG_SERIOUS, "--------------------------------------------------");
    }
    if (time_get_now_stamp() <= (ctrl->run.start - MAX_BETWEEN_RECODER_PAUSE)) {
        recorder_thread_check_and_exit_stream(ctrl, 1);
    } else {
        recorder_thread_check_and_exit_stream(ctrl, 0);
    }
    return ret;
}

static int recorder_thread_run(recorder_job_t *ctrl) {
    message_t vmsg, amsg;
    int ret_video = 1, ret_audio = 1, ret;
    unsigned char *p;
    av_data_info_t *avinfo;
    MP4Duration duration, offset;
    //condition
    pthread_mutex_lock(&vmutex[ctrl->t_id]);
    if ((video_buff[ctrl->t_id].head == video_buff[ctrl->t_id].tail) &&
        (audio_buff[ctrl->t_id].head == audio_buff[ctrl->t_id].tail)) {
        pthread_cond_wait(&vcond[ctrl->t_id], &vmutex[ctrl->t_id]);
    }
    msg_init(&vmsg);
    ret_video = msg_buffer_pop(&video_buff[ctrl->t_id], &vmsg);
    //read audio frame
    msg_init(&amsg);
    ret_audio = msg_buffer_pop(&audio_buff[ctrl->t_id], &amsg);
    pthread_mutex_unlock(&vmutex[ctrl->t_id]);
    if (!ret_audio) {
        avinfo = (av_data_info_t*) (amsg.extra);
        p = (unsigned char *) (amsg.arg);
        if (ctrl->run.first_frame) {
            if (ctrl->run.first_audio) {
                duration = (avinfo->timestamp - ctrl->run.last_aframe_stamp) * 90000 / 1000;
                offset = (avinfo->timestamp - ctrl->run.first_frame_stamp) * 90000 / 1000;
            } else {
                duration = 32 * 90000 / 1000;    //8k mono channel
                offset = 0;
                ctrl->run.first_audio = 1;
            }
            ret = MP4WriteSample(ctrl->run.mp4_file, ctrl->run.audio_track,
                                 p, avinfo->size,
                                 avinfo->size, 0, 1);
            if (!ret) {
                ret = ERR_ERROR;
                log_goke(DEBUG_WARNING, "MP4WriteSample audio failed.");
                goto close_exit;
            }
            ctrl->run.last_aframe_stamp = avinfo->timestamp;
        }
    }
    if (!ret_video) {
        avinfo = (av_data_info_t*) (vmsg.extra);
        p = (unsigned char *) (vmsg.arg);
        if (ctrl->run.first_frame && avinfo->fps != ctrl->run.fps) {
            log_goke(DEBUG_SERIOUS, "the video fps has changed, stop recording!");
            ret = ERR_ERROR;
            goto close_exit;
        }
        if (ctrl->run.first_frame && (avinfo->width != ctrl->run.width
                                      || avinfo->height != ctrl->run.height)) {
            log_goke(DEBUG_SERIOUS, "the video dimention has changed, stop recording!");
            ret = ERR_ERROR;
            goto close_exit;
        }
        ret = recorder_thread_write_mp4_video(ctrl, p, avinfo->size, avinfo);
        if (ret == -1) {
            log_goke(DEBUG_WARNING, "MP4WriteSample video failed.");
            ret = ERR_NO_DATA;
            goto exit;
        }
        if (recorder_thread_check_finish(ctrl)) {
            log_goke(DEBUG_INFO, "------------stop=%lld------------", time_get_now_stamp());
            log_goke(DEBUG_INFO, "recording finished!");
            goto close_exit;
        }
    }
    exit:
    if (!ret_video)
        msg_free(&vmsg);
    if (!ret_audio)
        msg_free(&amsg);
    return ret;
    close_exit:
    ret = recorder_thread_close(ctrl);
    ctrl->status = RECORDER_THREAD_PAUSE;
    if (!ret_video)
        msg_free(&vmsg);
    if (!ret_audio)
        msg_free(&amsg);
    return ret;
}

static int recorder_thread_started(recorder_job_t *ctrl) {
    int ret;
    if (time_get_now_stamp() >= ctrl->run.start) {
        log_goke(DEBUG_SERIOUS, "------------start=%lld------------", time_get_now_stamp());
        ret = recorder_thread_init_mp4v2(ctrl);
        if (ret) {
            log_goke(DEBUG_SERIOUS, "init mp4v2 failed!");
            ctrl->status = RECORDER_THREAD_ERROR;
        } else {
            ctrl->status = RECORDER_THREAD_RUN;
            recorder_thread_start_stream(ctrl);
        }
    }
    return ret;
}

static int *recorder_func(void *arg) {
    recorder_job_t ctrl;
    int i, finish_bit;
    message_t send_msg;
    char fname[MAX_SYSTEM_STRING_SIZE];
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    pthread_rwlock_wrlock(&ilock);
    memcpy(&ctrl, (recorder_job_t *) arg, sizeof(recorder_job_t));
    sprintf(fname, "%d%d-%lld", ctrl.t_id, ctrl.init.video_channel, time_get_now_stamp());
    misc_set_thread_name(fname);
    misc_set_bit(&info.thread_start, ctrl.t_id);
    pthread_rwlock_unlock(&ilock);
    //***data init
    msg_buffer_init2(&video_buff[ctrl.t_id], MSG_BUFFER_OVERFLOW_NO,
                     manager_config.msg_buffer_size_media, &vmutex[ctrl.t_id]);
    msg_buffer_init2(&audio_buff[ctrl.t_id], MSG_BUFFER_OVERFLOW_NO,
                     manager_config.msg_buffer_size_media, &vmutex[ctrl.t_id]);
    if (ctrl.init.start[0] == '0')
        ctrl.run.start = time_get_now_stamp();
    else {
        ctrl.run.start = time_date_to_stamp(ctrl.init.start);// - manager_config.timezone * 3600;
    }
    if (ctrl.init.stop[0] == '0')
        ctrl.run.stop = ctrl.run.start + recorder_config.max_length;
    else {
        ctrl.run.stop = time_date_to_stamp(ctrl.init.stop);// - manager_config.timezone * 3600;
    }
    if ((ctrl.run.stop - ctrl.run.start) < recorder_config.min_length ||
        (ctrl.run.stop - ctrl.run.start) > recorder_config.max_length)
        ctrl.run.stop = ctrl.run.start + recorder_config.max_length;
    log_goke(DEBUG_INFO, "-------------add new recorder---------------------");
    log_goke(DEBUG_INFO, "now=%lld", time_get_now_stamp());
    log_goke(DEBUG_INFO, "start=%lld", ctrl.run.start);
    log_goke(DEBUG_INFO, "end=%lld", ctrl.run.stop);
    log_goke(DEBUG_INFO, "recorder channel=%d", ctrl.t_id);
    log_goke(DEBUG_INFO, "--------------------------------------------------");
    ctrl.status = RECORDER_THREAD_STARTED;
    while (1) {
        if (info.exit) break;
        if (ctrl.run.exit) break;
        if (misc_get_bit(info.thread_exit, ctrl.t_id)) break;
        switch (ctrl.status) {
            case RECORDER_THREAD_STARTED:
                recorder_thread_started(&ctrl);
                break;
            case RECORDER_THREAD_RUN:
                recorder_thread_run(&ctrl);
                break;
            case RECORDER_THREAD_PAUSE:
                recorder_thread_pause(&ctrl);
                break;
            case RECORDER_THREAD_ERROR:
                recorder_thread_error(&ctrl);
                break;
        }
        usleep(1000);
    }
    //release
    pthread_rwlock_wrlock(&ilock);
    msg_buffer_release2(&video_buff[ctrl.t_id], &vmutex[ctrl.t_id]);
    msg_buffer_release2(&audio_buff[ctrl.t_id], &vmutex[ctrl.t_id]);
    if (ctrl.run.mp4_file != MP4_INVALID_FILE_HANDLE) {
        log_goke(DEBUG_INFO, "+++MP4Close");
        MP4Close(ctrl.run.mp4_file, MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
        ctrl.run.mp4_file = MP4_INVALID_FILE_HANDLE;
    }
    recorder_thread_stop_stream(&ctrl, 1);
    misc_clear_bit(&info.thread_start, ctrl.t_id);
    misc_clear_bit(&info.thread_exit, ctrl.t_id);
    memset(&jobs[ctrl.t_id], 0, sizeof(recorder_job_t));
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_RECORDER);
    log_goke(DEBUG_INFO, "-----------thread exit: record %s-----------", fname);
    pthread_exit(0);
}

static int recorder_thread_check_and_exit_stream(recorder_job_t *ctrl, int real_stop) {
    int ret = 0;
    ret = recorder_thread_stop_stream(ctrl, real_stop);
    return ret;
}

static int count_job_number(void) {
    int i, num = 0;
    for (i = 0; i < MAX_RECORDER_JOB; i++) {
        if (jobs[i].status > 0)
            num++;
    }
    return num;
}

static int recorder_add_job(message_t *msg) {
    message_t send_msg;
    int i = -1;
    int ret = 0;
    pthread_t pid;
    /********message body********/
    msg_init(&send_msg);
    send_msg.message = msg->message | 0x1000;
    send_msg.sender = send_msg.receiver = SERVER_RECORDER;
    /***************************/
    pthread_rwlock_wrlock(&ilock);
    if (!_config_.recorder_enable || (count_job_number() == MAX_RECORDER_JOB)) {
        send_msg.result = -1;
        ret = global_common_send_message(msg->receiver, &send_msg);
        pthread_rwlock_unlock(&ilock);
        return -1;
    }
    for (i = 0; i < MAX_RECORDER_JOB; i++) {
        if (jobs[i].status == RECORDER_THREAD_NONE) {
            memset(&jobs[i], 0, sizeof(recorder_job_t));
            jobs[i].t_id = i;
            jobs[i].status = RECORDER_THREAD_INITED;
            //start the thread
            memcpy(&(jobs[i].init), msg->arg, sizeof(recorder_init_t));
            ret = pthread_create(&pid, NULL, recorder_func, (void *) &jobs[i]);
            if (ret != 0) {
                log_goke(DEBUG_SERIOUS, "recorder thread create error! ret = %d", ret);
                jobs[i].status = RECORDER_THREAD_NONE;
                jobs[i].t_id = -1;
            } else {
                misc_clear_bit(&info.thread_exit, i);
                log_goke(DEBUG_INFO, "recorder thread create successful!");
                jobs[i].status = RECORDER_THREAD_STARTED;
            }
            break;
        }
    }
    pthread_rwlock_unlock(&ilock);
    send_msg.result = 0;
    ret = global_common_send_message(msg->receiver, &send_msg);
    return ret;
}

static void server_thread_termination(int sign) {
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_RECORDER_SIGINT;
    msg.sender = msg.receiver = SERVER_RECORDER;
    global_common_send_message(SERVER_MANAGER, &msg);
    /****************************/
}

static void recorder_broadcast_session_exit(void) {
    for (int i = 0; i < MAX_RECORDER_JOB; i++) {
        pthread_mutex_lock(&vmutex[i]);
        pthread_cond_signal(&vcond[i]);
        pthread_mutex_unlock(&vmutex[i]);
    }
}

static void server_release_1(void) {
    recorder_broadcast_session_exit();
}

static void server_release_2(void) {
    msg_buffer_release2(&message, &mutex);
    memset(&jobs, 0, sizeof(recorder_job_t));
}

static void server_release_3(void) {
    msg_free(&info.task.msg);
    memset(&info, 0, sizeof(server_info_t));
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_MANAGER_EXIT_ACK;
    msg.sender = SERVER_RECORDER;
    global_common_send_message(SERVER_MANAGER, &msg);
    /***************************/
}

/*
 *
 *
 */
static int recorder_message_filter(message_t *msg) {
    int ret = 0;
    if (info.status >= STATUS_ERROR) { //only system message
        if (!msg_is_system(msg->message) && !msg_is_response(msg->message))
            return 1;
    }
    return ret;
}

static int server_message_proc(void) {
    int ret = 0;
    int i;
    int finish_flag = 0;
    message_t msg;
    message_t send_msg;
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
    msg_init(&send_msg);
    ret = msg_buffer_pop(&message, &msg);
    pthread_mutex_unlock(&mutex);
    if (ret == 1)
        return 0;
    if( recorder_message_filter(&msg) ) {
        log_goke(DEBUG_VERBOSE, "RECORDER message filtered: sender=%s, message=%s, head=%d, tail=%d was screened",
                 global_common_get_server_name(msg.sender),
                 global_common_message_to_string(msg.message), message.head, message.tail);
        msg_free(&msg);
        return -1;
    }
    log_goke(DEBUG_VERBOSE, "RECORDER message popped: sender=%s, message=%s, head=%d, tail=%d",
             global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
    switch (msg.message) {
        case MSG_RECORDER_ADD:
            if( info.status == STATUS_RUN ) {
                recorder_add_job(&msg);
            }
            break;
        case MSG_RECORDER_ADD_ACK:
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
		case MSG_DEVICE_GET_PARA_ACK: {
            if (msg.arg_in.cat == DEVICE_PARAM_SD_INFO) {
                char fname[MAX_SYSTEM_STRING_SIZE*4];
                if( msg.arg_in.dog ) {
                    memset(fname, 0, sizeof(fname));
                    sprintf(fname, "%s", manager_config.media_directory);
                    if( access(fname, F_OK) )
                        mkdir(fname,0777);
                    for( int i = 0; i <= LV_STORAGE_RECORD_INITIATIVE; i++ ) {
                        memset(fname, 0, sizeof(fname));
                        sprintf(fname, "%s%s", manager_config.media_directory, manager_config.folder_prefix[i]);
                        if (access(fname, F_OK))
                            mkdir(fname, 0777);
                    }
                    misc_set_bit( &info.init_status, RECORDER_INIT_CONDITION_DEVICE_SD);
                }
            }
			break;
		}
        case MSG_ALIYUN_TIME_SYNCHRONIZED:
            misc_set_bit( &info.init_status, RECORDER_INIT_CONDITION_TIME_SYCHRONIZED);
            break;
        case MSG_ALIYUN_GET_TIME_SYNC_ACK:
            if( msg.arg_in.cat == 1 ) {
                misc_set_bit( &info.init_status, RECORDER_INIT_CONDITION_TIME_SYCHRONIZED);
            } else {
                misc_clear_bit( &info.init_status, RECORDER_INIT_CONDITION_TIME_SYCHRONIZED);
            }
            break;
        case MSG_MANAGER_EXIT_ACK:
            misc_clear_bit(&info.quit_status, msg.sender);
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "RECORDER: server %s quit and quit lable now = %s",
                     global_common_get_server_name(msg.sender),
                     fp);
            if(fp) {
                free(fp);
            }
            break;
        case MSG_MANAGER_DUMMY:
            break;
        case MSG_DEVICE_SD_FORMAT_START:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.status = STATUS_REINIT;
            break;
        case MSG_DEVICE_SD_FORMAT_START_ACK:
            break;
        case MSG_RECORDER_PROPERTY_SET:
            recorder_set_property(&msg);
            break;
        default:
            log_goke(DEBUG_SERIOUS, "RECORDER not support message %s",
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
	if( !misc_get_bit( info.init_status, RECORDER_INIT_CONDITION_DEVICE_SD)  ) {
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_DEVICE_GET_PARA;
        msg.sender = msg.receiver = SERVER_RECORDER;
        msg.arg_in.cat = DEVICE_PARAM_SD_INFO;
        ret = global_common_send_message(SERVER_DEVICE, &msg);
        /***************************/
		usleep(MESSAGE_RESENT_SLEEP);
	}
    if( !misc_get_bit( info.init_status, RECORDER_INIT_CONDITION_TIME_SYCHRONIZED)  ) {
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_ALIYUN_GET_TIME_SYNC;
        msg.sender = msg.receiver = SERVER_RECORDER;
        ret = global_common_send_message(SERVER_ALIYUN, &msg);
        /***************************/
        usleep(MESSAGE_RESENT_SLEEP);
    }
    if (misc_full_bit(info.init_status, RECORDER_INIT_CONDITION_NUM)) {
        info.status = STATUS_WAIT;
        info.tick = 0;
    }
    return ret;
}

static int server_setup(void) {
    info.status = STATUS_IDLE;
    return 0;
}

static int server_start(void) {
    info.status = STATUS_RUN;
    return 0;
}

static int server_stop(void) {
    int i;
    for (i = 0; i < MAX_RECORDER_JOB; i++) {
        misc_set_bit(&info.thread_exit, i);
    }
    recorder_broadcast_session_exit();
    global_common_send_dummy(SERVER_RECORDER);
    while( info.thread_start != 0) {
        usleep(MESSAGE_RESENT_SLEEP);       //!!!possible dead lock here
    }
    info.status2 = 0;
    info.status = STATUS_IDLE;
    return 0;
}

static int server_restart(void) {
    int i;
    for (i = 0; i < MAX_RECORDER_JOB; i++) {
        misc_set_bit(&info.thread_exit, i);
    }
    recorder_broadcast_session_exit();
    global_common_send_dummy(SERVER_RECORDER);
    while( info.thread_start != 0) {
        usleep(MESSAGE_RESENT_SLEEP);       //!!!possible dead lock here
    }
    info.status2 = 0;
    info.status = STATUS_WAIT;
    return 0;
}

static int server_reinit(void) {
    int i;
    for (i = 0; i < MAX_RECORDER_JOB; i++) {
        misc_set_bit(&info.thread_exit, i);
    }
    recorder_broadcast_session_exit();
    global_common_send_dummy(SERVER_RECORDER);
    while( info.thread_start != 0) {
        usleep(MESSAGE_RESENT_SLEEP);       //!!!possible dead lock here
    }
    info.status = STATUS_NONE;
    log_goke( DEBUG_INFO, "server recorder reinit success and enter init...");
    sd_set_response( E_SD_ACTION_FORMAT, SERVER_RECORDER );
    memset(&info, 0, sizeof(info));
}

/*
 *
 *
 * task proc
 *
 *
 *
 */
static void task_proc(void) {
    //default
    if (info.status == STATUS_NONE) {
        info.status = STATUS_INIT;
    } else if (info.status == STATUS_WAIT) {
        if( _config_.recorder_enable ) {
            info.status = STATUS_SETUP;
        }
    } else if (info.status == STATUS_IDLE) {
        info.status = STATUS_START;
    }
    return;
}

/*
 *
 *
 * state machine
 *
 *
 */
static void state_machine(void) {
    switch (info.status) {
        case STATUS_NONE:
        case STATUS_WAIT:
        case STATUS_IDLE:
            break;
        case STATUS_RUN:
            if( !info.status2 ) {
                recorder_start_init_recorder_job();
                info.status2 = 1;
            }
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
            log_goke(DEBUG_INFO, "RECORDER: switch to exit task!");
            if (info.task.msg.sender == SERVER_MANAGER) {
                info.quit_status &= (info.task.msg.arg_in.cat);
            } else {
                info.quit_status = 0;
            }
            char *fp = global_common_get_server_names( info.quit_status );
            log_goke(DEBUG_INFO, "RECORDER: Exit precondition =%s", fp);
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
            recorder_broadcast_session_exit();
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
            log_goke(DEBUG_SERIOUS, "RECORDER !!!!!!!unprocessed server status in state machine = %d", info.status);
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
    misc_set_thread_name("server_recorder");
    pthread_detach(pthread_self());
    msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_small, &mutex);
    info.quit_status = RECORDER_EXIT_CONDITION;
    info.init = 1;
    //default task
    while (!info.exit) {
        info.old_status = info.status;
        state_machine();
        task_proc();
        server_message_proc();
    }
    server_release_3();
    log_goke(DEBUG_INFO, "-----------thread exit: server_recorder-----------");
    pthread_exit(0);
}

/*
 * internal interface
 */

/*
 * external interface
 */
int server_recorder_start(void) {
    int ret = -1;
    ret = pthread_create(&info.id, NULL, server_func, NULL);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "recorder server create error! ret = %d", ret);
        return ret;
    } else {
        log_goke(DEBUG_INFO, "recorder server create successful!");
        return 0;
    }
}

int server_recorder_message(message_t *msg) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    if (!message.init) {
        log_goke(DEBUG_VERBOSE, "RECORDER server is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message) );
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    ret = msg_buffer_push(&message, msg);
    log_goke(DEBUG_VERBOSE, "RECORDER message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret,message.head, message.tail);
    if (ret != 0)
        log_goke(DEBUG_WARNING, "RECORDER message push in error =%d", ret);
    else {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

int server_recorder_video_message(message_t *msg) {
    int ret = 0, id = -1;
    id = msg->arg_in.wolf;
    pthread_mutex_lock(&vmutex[id]);
    if ((!video_buff[id].init)) {
        log_goke(DEBUG_WARNING, "recorder video [ch=%d] is not ready for message processing!", id);
        pthread_mutex_unlock(&vmutex[id]);
        return -1;
    }
    ret = msg_buffer_push(&video_buff[id], msg);
//    log_goke(DEBUG_VERBOSE, "RECORDER_VIDEO message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
//             global_common_get_server_name(msg->sender),
//             global_common_message_to_string(msg->message),
//             ret,message.head, message.tail);
    if (ret == MSG_BUFFER_PUSH_FAIL )
        log_goke(DEBUG_WARNING, "message push in recorder video error =%d", ret);
    else {
        pthread_cond_signal(&vcond[id]);
    }
    pthread_mutex_unlock(&vmutex[id]);
    return ret;
}

int server_recorder_audio_message(message_t *msg) {
    int ret = 0, id = -1;
    id = msg->arg_in.wolf;
    pthread_mutex_lock(&vmutex[id]);
    if ((!audio_buff[id].init)) {
        log_goke(DEBUG_WARNING, "recorder audio [ch=%d] is not ready for message processing!", id);
        pthread_mutex_unlock(&vmutex[id]);
        return -1;
    }
    ret = msg_buffer_push(&audio_buff[id], msg);
    if (ret == MSG_BUFFER_PUSH_FAIL )
        log_goke(DEBUG_WARNING, "message push in recorder audio error =%d", ret);
    else {
        pthread_cond_signal(&vcond[id]);
    }
    pthread_mutex_unlock(&vmutex[id]);
    return ret;
}