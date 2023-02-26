/*
 * header
 */
//system header
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <malloc.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <malloc.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include <zbar.h>
#include <link_visual_api.h>
#include <iot_export_linkkit.h>
#include <iot_export.h>
#include <iot_import.h>
//program header
#include "../../server/manager/manager_interface.h"
#include "../../common/tools_interface.h"
#include "../../server/aliyun/aliyun_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../server/goke/goke_interface.h"
#include "../../server/goke/hisi/hisi.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/device/gk_gpio.h"
#include "../../server/player/player_interface.h"
//server header
#include "video.h"
#include "osd.h"
#include "video_interface.h"
#include "config.h"
#include "goke_osd.h"
#include "isp.h"
/*
 * static
 */
//variable
static message_buffer_t message;
static server_info_t info;
static video_running_info_t running_info;
static pthread_rwlock_t ilock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t snap_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t snap_cond = PTHREAD_COND_INITIALIZER;
static message_buffer_t snap_buff;
static long long int last_report = 0;
static video_channel_t channel[MAX_AV_CHANNEL];

//function
//common
static void *server_func(void);

static int server_message_proc(void);

static void state_machine(void);

static void task_proc(void);

static void server_release_1(void);

static void server_release_2(void);

static void server_release_3(void);

static int server_init(void);

static int server_setup(void);

static int server_stop(void);

static int server_restart(void);

static void server_thread_termination(int sign);

//specific
static int write_video_buffer(av_data_info_t *avinfo, char *data, int id, int target, int type);

static int video_adjust_parameters(video_config_t *config);

static int *video_isp_func(void *arg);

static int *video_stream_func(void *arg);

static int video_init(void);

static int video_start(void);

static int video_stop(void);

static int video_uninit(void);
/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 *
 *
 * channel
 *
 *
 *
 */
static void ircut_opt_release(void)
{
    gk_ircut_release();
    log_goke(DEBUG_INFO, "ircut_opt_release!");
}

static void init_visual_stream_config(int id) {
    lv_video_param_s video_param;
    lv_audio_param_s audio_param;
    lv_stream_send_config_param_s send_config;
    memset(&video_param, 0, sizeof(lv_video_param_s));
    memset(&audio_param, 0, sizeof(lv_audio_param_s));

    pthread_rwlock_rdlock(&ilock);
    //video
    int source = channel[id].stream_type;
    if (video_config.profile.stream[source].type == PT_H264) {
        video_param.format = LV_VIDEO_FORMAT_H264;
    } else if (video_config.profile.stream[source].type == PT_H265) {
        video_param.format = LV_VIDEO_FORMAT_H265;
    } else {
        log_goke(DEBUG_WARNING, "video codec not supported!");
    }
    video_param.fps = video_config.profile.stream[source].frame_rate;
    //audio: for simplicity, do it here as audio is fixed source
    if (audio_config.payload_type == PT_G711A) {
        audio_param.format = LV_AUDIO_FORMAT_G711A;
    } else if (audio_config.payload_type == PT_G711U) {
        audio_param.format = LV_AUDIO_FORMAT_G711U;
    } else if (audio_config.payload_type == PT_AAC) {
        audio_param.format = LV_AUDIO_FORMAT_AAC;
    } else {
        log_goke(DEBUG_WARNING, "audio codec not supported!");
    }
    if (audio_config.aio_attr.enSoundmode == AUDIO_SOUND_MODE_MONO) {
        audio_param.channel = LV_AUDIO_CHANNEL_MONO;
    }
    if (audio_config.aio_attr.enBitwidth == AUDIO_BIT_WIDTH_16) {
        audio_param.sample_bits = LV_AUDIO_SAMPLE_BITS_16BIT;
    } else if (audio_config.aio_attr.enBitwidth == AUDIO_BIT_WIDTH_8) {
        audio_param.sample_bits = LV_AUDIO_SAMPLE_BITS_8BIT;
    }
    if (audio_config.aio_attr.enSamplerate == AUDIO_SAMPLE_RATE_8000) {
        audio_param.sample_rate = LV_AUDIO_SAMPLE_RATE_8000;
    } else if (audio_config.aio_attr.enSamplerate == AUDIO_SAMPLE_RATE_16000) {
        audio_param.sample_rate = LV_AUDIO_SAMPLE_RATE_16000;
    }
    if (source == VIDEO_STREAM_HIGH) {
        send_config.bitrate_kbps = 1536;
    } else {
        send_config.bitrate_kbps = 512;
    }
    send_config.video_param = &video_param;
    send_config.audio_param = &audio_param;

    lv_stream_send_config(channel[id].service_id, &send_config);
    pthread_rwlock_unlock(&ilock);
}

static int get_old_channel(int id, server_type_t type) {
    int i;
    for (i = 0; i < MAX_AV_CHANNEL; i++) {
        if ((channel[i].service_id == id) &&
            (channel[i].channel_type == type) &&
            (channel[i].status == CHANNEL_VALID)) {
            return i;
        }
    }
    return -1;
}

static int get_new_channel(int id, server_type_t type) {
    int i;
    for (i = 0; i < MAX_AV_CHANNEL; i++) {
        if ((channel[i].service_id == id) &&
            (channel[i].channel_type == type) &&
            (channel[i].status == CHANNEL_VALID)) {
            return i;
        }
    }
    for (i = 0; i < MAX_AV_CHANNEL; i++) {
        if (channel[i].status != CHANNEL_VALID) {
            channel[i].service_id = id;
            channel[i].channel_type = type;
            channel[i].status = CHANNEL_VALID;
            return i;
        }
    }
    return -1;
}

int audio_video_synchronize(int id) {
    int i;
    for (i = 0; i < MAX_AV_CHANNEL; i++) {
        if ((channel[i].service_id == id) && (channel[i].status == CHANNEL_VALID)) {
            return channel[i].require_key;
        }
    }
    return 1;
}

/*
 *
 *
 * qrcode
 *
 *
 *
 */
static int get_jpeg_data(void *data, int size, int *width, int *height,
                         void **raw) {
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr err;          //the error handler
    /* More stuff */
    int ret;
    JSAMPARRAY buffer;      /* Output row buffer */
    unsigned char *rowptr[1];  // pointer to an array
    int row_stride;     /* physical row width in output buffer */
//    FILE *infile;
//     if ((infile = fopen("/mnt/media/test0.jpeg", "rb")) == NULL) {
//        fprintf(stderr, "can't open\n");
//        return 0;
//    }
    /* Step 1: allocate and initialize JPEG decompression object */
    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&err);
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);
    /* Step 2: specify data source (eg, a file) */
    jpeg_mem_src(&cinfo, data, size);
//    jpeg_stdio_src(&cinfo, infile);
    /* Step 3: read file parameters with jpeg_read_header() */
    (void) jpeg_read_header(&cinfo, TRUE);
    /* Step 4: set parameters for decompression */
    cinfo.out_color_space = JCS_GRAYSCALE;
    /* Step 5: Start decompressor */
    (void) jpeg_start_decompress(&cinfo);
    *width = cinfo.image_width;
    *height = cinfo.image_height;
    row_stride = cinfo.output_width * cinfo.output_components;
    *raw = (void *) malloc(cinfo.output_width * cinfo.output_height * 3);
    long counter = 0;
    //step 6, read the image line by line
    unsigned bpl = cinfo.output_width * cinfo.output_components;
    JSAMPROW buf = (void *) *raw;
    JSAMPARRAY line = &buf;
    for (; cinfo.output_scanline < cinfo.output_height; buf += bpl) {
        jpeg_read_scanlines(&cinfo, line, 1);
        /* FIXME pad out to dst->width */
    }
    /* Step 7: Finish decompression */
    (void) jpeg_finish_decompress(&cinfo);
    /* Step 8: Release JPEG decompression object */
    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);
    /* And we're done! */
    return 0;
}

static int zbar_process(VENC_STREAM_S *stream, int width, int height) {
    int ret = -1, i;
    int packet_start, packet_size, total_size;
    zbar_image_scanner_t *scanner = NULL;
    char *result;
    char *data = 0;
    message_t msg;
    void *raw = NULL;
    total_size = 0;
    for (i = 0; i < stream->u32PackCount; i++) {
        packet_size = stream->pstPack[i].u32Len - stream->pstPack[i].u32Offset;
        total_size += packet_size;
    }
    data = malloc(total_size);
    if (data == 0) {
        log_goke(DEBUG_SERIOUS, "memory allocation failed!");
    }
    total_size = 0;
    for (i = 0; i < stream->u32PackCount; i++) {
        packet_start = stream->pstPack[i].pu8Addr + stream->pstPack[i].u32Offset;
        packet_size = stream->pstPack[i].u32Len - stream->pstPack[i].u32Offset;
        memcpy(&data[total_size], packet_start, packet_size);
        total_size += packet_size;
    }
    //get image data
    get_jpeg_data(data, total_size, &width, &height, &raw);
    /* configure the reader */
    scanner = zbar_image_scanner_create();
    zbar_image_t *image = zbar_image_create();
    zbar_image_set_format(image, *(int *) "Y800");
    zbar_image_set_size(image, width, height);

    zbar_image_set_data(image, raw, width * height, 0);
//    zbar_image_t *jpeg_image = zbar_image_convert(image, *(int *) "Y800" );

    zbar_image_scanner_set_config(scanner, ZBAR_NONE, ZBAR_CFG_ENABLE, 1);
    zbar_image_scanner_set_config(scanner, ZBAR_NONE, ZBAR_CFG_X_DENSITY, 1);
    zbar_image_scanner_set_config(scanner, ZBAR_NONE, ZBAR_CFG_Y_DENSITY, 1);
    /* scan the image for barcodes */
    int n = zbar_scan_image(scanner, image);
    log_goke(DEBUG_INFO, "result1 n = %d\r", n);
    /* extract results */
    if (n == 1) {
        const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
        for (; symbol; symbol = zbar_symbol_next(symbol)) {
            /* do something useful with results */
            zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
            const char *data = zbar_symbol_get_data(symbol);
            log_goke(DEBUG_INFO, "=========================================decoded %s symbol \"%s\"\r",
                     zbar_get_symbol_name(typ), data);
            result = calloc(strlen(data), 1);
            if (result) {
                strncpy(result, data, strlen(data));
                //play sound
                /****************************/
                msg_init(&msg);
                msg.message = MSG_AUDIO_PLAY_SOUND;
                msg.arg_in.cat = AUDIO_RESOURCE_SCAN_SUCCESS;
                global_common_send_message(SERVER_AUDIO, &msg);
                /****************************/
                //send back scan result
                /****************************/
                msg_init(&msg);
                msg.message = MSG_ALIYUN_QRCODE_RESULT;
                msg.result = 1;         //success
                msg.arg = result;
                msg.arg_size = strlen(data);
                global_common_send_message(SERVER_ALIYUN, &msg);
                /****************************/
                ret = 0;
                free(result);
            }
        }
    } else {
        /****************************/
        //send back scan result
        /****************************/
//        msg_init(&msg);
//        msg.message = MSG_ALIYUN_QRCODE_RESULT;
//        msg.result = 0;         //success
//        global_common_send_message(SERVER_ALIYUN, &msg);
        /****************************/
    }
    /* clean up */
    free(data);
    free(raw);
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);
    return ret;
}

int video_test_full_thread(void) {
    if (!misc_get_bit(info.thread_start, VIDEO_THREAD_ISP)) {
        return 0;
    }
    if (!misc_get_bit(info.thread_start, VIDEO_THREAD_ISP_EXTRA)) {
        return 0;
    }
    if (!misc_get_bit(info.thread_start, VIDEO_THREAD_OSD)) {
        return 0;
    }
    if ( video_config.profile.stream[ID_MD].enable && !misc_get_bit(info.thread_start, VIDEO_THREAD_MD) ) {
        return 0;
    }
    for (int i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (!misc_get_bit(info.thread_start, VIDEO_THREAD_ENCODER + i) &&
            video_config.profile.stream[i].enable) {
            return 0;
        }
    }
    return 1;
}

/*
 *
 *
 * thread funcs
 *
 *
 */
static int *video_isp_func(void *arg) {
    video_config_t config;
    server_status_t st;
    int ret;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    misc_set_thread_name("server_video_isp");
    pthread_rwlock_wrlock(&ilock);
    memcpy(&video_config, (video_config_t *) arg, sizeof(video_config_t));
    misc_set_bit(&info.thread_start, VIDEO_THREAD_ISP);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread start: server_isp-----------");
    //data init
    ret = MPI_ISP_Run(0);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_SERIOUS, "HI_MPI_ISP_Run failed with %#x!", ret);
    }
    //release
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, VIDEO_THREAD_ISP);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread exit: server_isp-----------");
    pthread_exit(0);
}

static void *video_isp_extra_func(void *pArgs) {
    HI_S32 ret;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    misc_set_thread_name("server_isp_extra_func");
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit(&info.thread_start, VIDEO_THREAD_ISP_EXTRA);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread start: server_isp_extra -----------");
    while (1) {
        //status check
        pthread_rwlock_rdlock(&ilock);
        if (info.exit ||
            misc_get_bit(info.thread_exit, VIDEO_THREAD_ISP_EXTRA) ) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        if (info.status != STATUS_RUN ) {
            pthread_rwlock_unlock(&ilock);
            continue;
        }
        if ( !misc_get_bit(info.thread_start, VIDEO_THREAD_ISP) ) {
            pthread_rwlock_unlock(&ilock);
            continue;
        }
        pthread_rwlock_unlock(&ilock);
        ret = video_isp_proc(0);
        if( ret  ) {
            log_goke(DEBUG_WARNING, "-----------error happened in isp extra proc, exit!!!!!!!!!!!!!!!!!");
            break;
        }
        usleep( 500 * 1000); //500ms
    }
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, VIDEO_THREAD_ISP_EXTRA);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread exit: server_isp_extra-----------");
}

static int test = 0;
static int *video_stream_func(void *arg) {
    video_stream_info_t stream_info;
    int ret;
    int max_fd = 0;
    struct timeval timeout;
    fd_set read_fds;
    int fd;
    VENC_CHN_STATUS_S stat;
    VENC_CHN_ATTR_S channel_attr;
    VENC_STREAM_S stream;
    av_data_info_t avinfo;
    char *data;
    int i;
    unsigned int packet_start, packet_size, total_size;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    misc_set_thread_name("server_video_stream");
    stream_info.channel = (int) arg;
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit(&info.thread_start, VIDEO_THREAD_ENCODER + stream_info.channel);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread start: server_stream id=%d-----------", stream_info.channel);
    //data init
    ret = HI_MPI_VENC_GetChnAttr(stream_info.channel, &channel_attr);
    if (ret != HI_SUCCESS) {
        log_goke(DEBUG_WARNING, "HI_MPI_VENC_GetChnAttr chn[%d] failed with %#x!", \
                   stream_info.channel, ret);
        goto EXIT;
    }
    stream_info.width = channel_attr.stVencAttr.u32PicWidth;
    stream_info.height = channel_attr.stVencAttr.u32PicHeight;
    stream_info.rc_mode = channel_attr.stRcAttr.enRcMode;
    switch (channel_attr.stRcAttr.enRcMode) {
        case VENC_RC_MODE_H264CBR:
            stream_info.fps = channel_attr.stRcAttr.stH264Cbr.fr32DstFrameRate;
            break;
        case VENC_RC_MODE_H264VBR:
            stream_info.fps = channel_attr.stRcAttr.stH264Vbr.fr32DstFrameRate;
            break;
        case VENC_RC_MODE_H265CBR:
            stream_info.fps = channel_attr.stRcAttr.stH265Cbr.fr32DstFrameRate;
            break;
    }
    /* Set Venc Fd. */
    fd = HI_MPI_VENC_GetFd(stream_info.channel);
    if (fd < 0) {
        log_goke(DEBUG_SERIOUS, "HI_MPI_VENC_GetFd failed with %#x!", fd);
        return NULL;
    }
    if (max_fd <= fd) {
        max_fd = fd;
    }
    while (1) {
        //status check
        pthread_rwlock_rdlock(&ilock);
        if (info.exit ||
            misc_get_bit(info.thread_exit, VIDEO_THREAD_ENCODER + stream_info.channel) ) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        if (info.status != STATUS_RUN) {
            pthread_rwlock_unlock(&ilock);
            continue;
        }
        pthread_rwlock_unlock(&ilock);
        //selector
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000;    //10ms
        ret = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ret < 0) {
            log_goke(DEBUG_WARNING, "video stream select select failed");
            break;
        } else if (ret == 0) {
            continue;
        } else {
            if (FD_ISSET(fd, &read_fds)) {
                //query frames
                memset(&stream, 0, sizeof(stream));
                ret = HI_MPI_VENC_QueryStatus(stream_info.channel, &stat);
                if (HI_SUCCESS != ret) {
                    log_goke(DEBUG_WARNING, "HI_MPI_VENC_QueryStatus chn[%d] failed with %#x!", stream_info.channel,
                             ret);
                    break;
                }
                if (0 == stat.u32CurPacks) {
                    log_goke(DEBUG_WARNING, "NOTE: Current  frame is NULL");
                    continue;
                }
                //allocate stream node
                stream.pstPack = (VENC_PACK_S *) malloc(sizeof(VENC_PACK_S) * stat.u32CurPacks);
                if (NULL == stream.pstPack) {
                    log_goke(DEBUG_SERIOUS, "malloc stream pack failed");
                    break;
                }
                //get stream data
                stream.u32PackCount = stat.u32CurPacks;
                ret = HI_MPI_VENC_GetStream(stream_info.channel, &stream, HI_TRUE);
                if (HI_SUCCESS != ret) {
                    free(stream.pstPack);
                    stream.pstPack = NULL;
                    log_goke(DEBUG_WARNING, "HI_MPI_VENC_GetStream failed with %#x!", ret);
                    break;
                }
                total_size = 0;
                for (i = 0; i < stream.u32PackCount; i++) {
                    HI_U32 packet_start = stream.pstPack[i].pu8Addr + stream.pstPack[i].u32Offset;
                    HI_U32 packet_size = stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset;
                    total_size += packet_size;
                }
                if (total_size > MAX_VIDEO_FRAME_SIZE) {
                    log_goke(DEBUG_WARNING, "+++++++++++++++++++++++++++++video frame size=%d!!!!!!", total_size);
                    goto CLEAN;
                }
                data = malloc(total_size);
                if (data == NULL) {
                    log_goke(DEBUG_SERIOUS, "allocate memory failed in video buffer, size=%d", total_size);
                    goto CLEAN;
                }
                total_size = 0;
                for (i = 0; i < stream.u32PackCount; i++) {
                    packet_start = stream.pstPack[i].pu8Addr + stream.pstPack[i].u32Offset;
                    packet_size = stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset;
                    memcpy(&data[total_size], packet_start, packet_size);
                    total_size += packet_size;
                }
                if ((stream_info.goke_stamp == 0) && (stream_info.unix_stamp == 0)) {
                    stream_info.goke_stamp = stream.pstPack[0].u64PTS;  //first pts
                    stream_info.unix_stamp = time_get_now_stamp();
                }
                stream_info.frame = stream.u32Seq;
                //***write video info
                avinfo.flag = 0;
                avinfo.frame_index = stream_info.frame;
                avinfo.timestamp = (unsigned int) ((time_get_now_ms() - _universal_tag_));
                avinfo.fps = stream_info.fps;
                avinfo.width = stream_info.width;
                avinfo.height = stream_info.height;
                avinfo.size = total_size;
                avinfo.source = STREAM_SOURCE_LIVE;
                avinfo.channel = stream_info.channel ? VIDEO_STREAM_LOW : VIDEO_STREAM_HIGH;
                avinfo.type = MEDIA_TYPE_VIDEO;
                avinfo.key_frame = h264_is_iframe(data[4]);
                //***iterate for different stream destination
                for (i = 0; i < MAX_AV_CHANNEL; i++) {
                    if ((channel[i].status == CHANNEL_VALID) &&
                        (channel[i].stream_type == stream_info.channel)) {
                        if (channel[i].channel_type == SERVER_RECORDER) {
                            ret = write_video_buffer(&avinfo, data, MSG_RECORDER_VIDEO_DATA, SERVER_RECORDER,
                                                     channel[i].service_id);
                            if (ret) {
                                log_goke(DEBUG_VERBOSE, "recorder video send failed! ret = %d", ret);
                            }
#if 0
                            if( test > 200 ) {
                                player_init_t player;
                                //player init
                                memset(&player, 0, sizeof(player_init_t));
                                player.start = 1676995200;//1675991766;  //UTC
                                player.stop = 1677031165;//1676004965;
                                player.offset = 0;
                                player.speed = 1;
                                player.channel_merge = 0;
                                player.switch_to_live = 1;
                                player.audio = 1;
                                player.type = LV_STORAGE_RECORD_PLAN;
                                player.channel = channel[i].service_id;
                                //strcpy( player.filename, "/mnt/sdcard/media/normal/20230210050605_20230210051605.mp4");
                                //send for player server
                                message_t msg;
                                msg_init(&msg);
                                msg.sender = msg.receiver = SERVER_ALIYUN;
                                msg.message = MSG_PLAYER_REQUEST;
                                msg.arg_in.duck = msg.arg_pass.duck = STREAM_SOURCE_PLAYER;
                                msg.arg_in.wolf = msg.arg_pass.wolf = channel[i].service_id;
                                msg.arg = &player;
                                msg.arg_size = sizeof(player_init_t);
                                global_common_send_message( SERVER_PLAYER, &msg);
                                test = -500000000;
                            }
                            test++;
#endif
                        } else if (channel[i].channel_type == SERVER_ALIYUN) {
                            if ((channel[i].require_key && avinfo.key_frame) ||
                                !channel[i].require_key) {
                                if (channel[i].require_key) {
                                    init_visual_stream_config(i);
                                }
                                lv_stream_send_media_param_s param = {{0}};
                                param.common.type = LV_STREAM_MEDIA_VIDEO;
                                param.common.p = (char *) data;
                                param.common.len = avinfo.size;
                                param.common.timestamp_ms = avinfo.timestamp;
                                param.video.format = LV_VIDEO_FORMAT_H264;
                                param.video.key_frame = avinfo.key_frame;
                                ret = lv_stream_send_media(channel[i].service_id, &param);
                                if (ret) {
                                    log_goke(DEBUG_VERBOSE, "aliyun video send failed, ret=%x, service_id = %d", ret,
                                             channel[i].service_id);
                                    if (ret == LV_ERROR_DEFAULT || ret == LV_ERROR_ILLEGAL_INPUT) {
                                        channel[i].status = CHANNEL_EMPTY;
                                        log_goke(DEBUG_WARNING, "aliyun channel closed due to error service_id = %d",
                                                 channel[i].service_id);
                                    }
                                    if( channel[i].qos_sucess!=0) {
                                        int err = avinfo.timestamp - channel[i].qos_sucess;
                                        if (err > QOS_MAX_LATENCY) {//
                                            log_goke(DEBUG_WARNING,
                                                     " aliyun audio channel sending failed for %d seconds, "
                                                     "channel closed!", err / 1000);
                                            channel[i].status = CHANNEL_EMPTY;
                                        }
                                    }
                                } else {
                                    channel[i].qos_sucess = avinfo.timestamp;
                                }
                                if (channel[i].require_key && !ret) {
                                    channel[i].require_key = 0;
                                }
                            }
                        }
                    }
                }
                if (data) {
                    free(data);
                    data = 0;
                }
                CLEAN:
                //release stream
                ret = HI_MPI_VENC_ReleaseStream(stream_info.channel, &stream);
                if (HI_SUCCESS != ret) {
                    log_goke(DEBUG_SERIOUS, "HI_MPI_VENC_ReleaseStream failed, %x", ret);
                    free(stream.pstPack);
                    stream.pstPack = NULL;
                    break;
                }
                //free nodes
                free(stream.pstPack);
                stream.pstPack = NULL;
            }
        }
    }
    EXIT:
    //release
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, VIDEO_THREAD_ENCODER + stream_info.channel);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread exit: server_stream id=%d-----------", stream_info.channel);
    pthread_exit(0);
}

int video_md_trigger_message(void) {
    int ret = 0;
    recorder_init_t init;
    unsigned long long int now;
    if (_config_.alarm_switch) {
        now = time_get_now_stamp();
        if ((now - last_report) >= video_config.md.alarm_interval[_config_.alarm_freqency] * 60) {
            last_report = now;
            message_t msg;
            /********motion notification********/
//            msg_init(&msg);
//            msg.message = MSG_ALIYUN_EVENT;
//            msg.sender = msg.receiver = SERVER_VIDEO;
//            msg.arg_in.cat = ALIYUN_EVENT_OBJECT_DETECTED;
//            msg.extra = &now;
//            msg.extra_size = sizeof(now);
//            ret = global_common_send_message(SERVER_ALIYUN, &msg);
            /********recorder********/
            msg_init(&msg);
            msg.message = MSG_RECORDER_ADD;
            msg.sender = msg.receiver = SERVER_VIDEO;
            memset(&init, 0, sizeof(init));
            init.video_channel = 0;
            init.mode = RECORDER_MODE_BY_TIME;
            init.type = LV_STORAGE_RECORD_ALARM;
            init.audio = 1;
            memset(init.start, 0, sizeof(init.start));
            time_get_now_str(init.start);
            now += recorder_config.alarm_length;
            memset(init.stop, 0, sizeof(init.stop));
            time_stamp_to_date(now, init.stop);
            init.repeat = 0;
            init.repeat_interval = 0;
            init.quality = 0;
            msg.arg = &init;
            msg.arg_size = sizeof(recorder_init_t);
            msg.extra = &now;
            msg.extra_size = sizeof(now);
            ret = global_common_send_message(SERVER_RECORDER, &msg);
        }
    }
    return ret;
}

static void *video_osd_func(void *pArgs) {
    HI_S32 ret;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    misc_set_thread_name("server_osd_func");
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit(&info.thread_start, VIDEO_THREAD_OSD);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread start: server_osd_func -----------");
    while (1) {
        //status check
        pthread_rwlock_rdlock(&ilock);
        if (info.exit ||
            misc_get_bit(info.thread_exit, VIDEO_THREAD_OSD) ) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        if (info.status != STATUS_RUN ) {
            pthread_rwlock_unlock(&ilock);
            continue;
        }
        pthread_rwlock_unlock(&ilock);
        ret = osd_proc();
        if( ret == 0 ) {
            log_goke(DEBUG_WARNING, "-----------error happened in OSD proc, exit!!!!!!!!!!!!!!!!!");
            break;
        }
        usleep( 500 * 1000); //500ms
    }
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, VIDEO_THREAD_OSD);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread exit: server_osd_func-----------");
}

static void *video_md_func(void) {
    HI_S32 ret;
    VIDEO_FRAME_INFO_S stExtFrmInfo;
    HI_S32 s32MilliSec = 500000;     //200ms
    HI_BOOL bInstant = HI_TRUE;
    HI_S32 s32CurIdx = 0;
    HI_BOOL bFirstFrm = HI_TRUE;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    pthread_detach(pthread_self());
    misc_set_thread_name("server_md_stream");
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit(&info.thread_start, VIDEO_THREAD_MD);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread start: server_md_stream -----------");
    //Create chn
    ret = HI_IVS_MD_CreateChn(video_config.md.md_channel, &video_config.md.md_attr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_IVS_MD_CreateChn fail,Error(%#x)", ret);
        goto EXIT;
    }
    while (1) {
        //status check
        pthread_rwlock_rdlock(&ilock);
        if (info.exit ||
            misc_get_bit(info.thread_exit, VIDEO_THREAD_MD) ) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        if (info.status != STATUS_RUN ||
            !running_info.md_run) {
            s32CurIdx = 0;
            bFirstFrm = HI_TRUE;
            pthread_rwlock_unlock(&ilock);
            continue;
        }
        pthread_rwlock_unlock(&ilock);

        ret = HI_MPI_VPSS_GetChnFrame(video_config.vpss.group,
                                      video_config.profile.stream[ID_MD].vpss_chn,
                                      &stExtFrmInfo, s32MilliSec);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "Error(%#x),HI_MPI_VPSS_GetChnFrame failed, VPSS_GRP(%d), 2)",
                     ret, video_config.vpss.group);
            continue;
        }
        if (HI_TRUE != bFirstFrm) {
            ret = hisi_md_dma_image(&stExtFrmInfo, &video_config.md.ast_img[s32CurIdx], bInstant);
            if (HI_SUCCESS != ret) {
                log_goke(DEBUG_WARNING, "SAMPLE_COMM_IVE_DmaImage fail,Error(%#x)", ret);
                goto EXT_RELEASE;
            }
        } else {
            ret = hisi_md_dma_image(&stExtFrmInfo, &video_config.md.ast_img[1 - s32CurIdx], bInstant);
            if (HI_SUCCESS != ret) {
                log_goke(DEBUG_WARNING, "SAMPLE_COMM_IVE_DmaImage fail,Error(%#x)", ret);
                goto EXT_RELEASE;
            }
            bFirstFrm = HI_FALSE;
            goto CHANGE_IDX;//first frame just init reference frame
        }
        ret = HI_IVS_MD_Process(video_config.md.md_channel, &video_config.md.ast_img[s32CurIdx],
                                &video_config.md.ast_img[1 - s32CurIdx],
                                NULL, &video_config.md.blob);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "HI_IVS_MD_Process fail,Error(%#x)", ret);
            goto EXT_RELEASE;
        }
        ive_blob_to_rect(IVE_CONVERT_64BIT_ADDR(IVE_CCBLOB_S, video_config.md.blob.u64VirAddr),
                         &(video_config.md.region), IVE_RECT_NUM, 8,
                         video_config.md.md_attr.u32Width, video_config.md.md_attr.u32Height,
                         video_config.vi.sensor_info.width,
                         video_config.vi.sensor_info.height);

        if (video_config.md.region.u16Num > 3) {//report and recording
            log_goke(DEBUG_WARNING, "-----motion detected, movement section = %d-----", video_config.md.region.u16Num);
            ret = video_md_trigger_message();
        }
        CHANGE_IDX:
        //Change reference and current frame index
        s32CurIdx = 1 - s32CurIdx;

        EXT_RELEASE:
        ret = HI_MPI_VPSS_ReleaseChnFrame(video_config.vpss.group,
                                          video_config.profile.stream[ID_MD].vpss_chn,
                                          &stExtFrmInfo);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "Error(%#x),HI_MPI_VPSS_ReleaseChnFrame failed,Grp(%d) chn(2)!",
                     ret, video_config.vpss.group);
        }
    }

    //destroy
    ret = HI_IVS_MD_DestroyChn(video_config.md.md_channel);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_IVS_MD_DestroyChn fail,Error(%#x)", ret);
    }
    EXIT:
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, VIDEO_THREAD_MD);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread exit: server_md_stream-----------");
}

static void video_snap_func(void *arg) {
    struct timeval timeout;
    int repeat;
    fd_set read_fds;
    HI_S32 fd;
    VENC_CHN_STATUS_S stStat;
    VENC_STREAM_S stream;
    HI_S32 ret;
    VENC_RECV_PIC_PARAM_S stRecvParam;
    int i;
    stream_profile_info_t profile;
    static char filename[4 * MAX_SYSTEM_STRING_SIZE];
    long long int now;
    char timestr[MAX_SYSTEM_STRING_SIZE];
    FILE *pFile;
    message_t msg, send_msg;
    int channel;
    MPP_CHN_S stDestChn;
    MPP_CHN_S stSrcChn;
    VPSS_CHN_ATTR_S stVpssChnAttr;
    bool qrcode_success = 0;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    misc_set_thread_name("server_video_snap");
    pthread_detach(pthread_self());
    channel = (int) arg;
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit(&info.thread_start, VIDEO_THREAD_ENCODER + channel);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread start: server_video_snap -----------");
    //data init
    msg_buffer_init2(&snap_buff, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_small, &snap_mutex);
    profile = video_config.profile.stream[ID_PIC];
    stDestChn.enModId = HI_ID_VENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = profile.venc_chn;
    ret = HI_MPI_SYS_GetBindbyDest(&stDestChn, &stSrcChn);
    if (ret == HI_SUCCESS && stSrcChn.enModId == HI_ID_VPSS) {
        ret = HI_MPI_VPSS_GetChnAttr(stSrcChn.s32DevId, stSrcChn.s32ChnId, &stVpssChnAttr);
        if (ret != HI_SUCCESS || stVpssChnAttr.enCompressMode == COMPRESS_MODE_NONE) {
            goto EXIT;
        }
    }
    while (1) {
        pthread_rwlock_rdlock(&ilock);
        if (info.exit || (misc_get_bit(info.thread_exit, VIDEO_THREAD_ENCODER + channel))) {
            pthread_rwlock_unlock(&ilock);
            break;
        }
        pthread_rwlock_unlock(&ilock);
        //condition
        pthread_mutex_lock(&snap_mutex);
        if (snap_buff.head == snap_buff.tail) {
            pthread_cond_wait(&snap_cond, &snap_mutex);
        }
        msg_init(&msg);
        ret = msg_buffer_pop(&snap_buff, &msg);
        pthread_mutex_unlock(&snap_mutex);
        if (ret)
            continue;
        if( msg.message == MSG_MANAGER_DUMMY)
            continue;
        //***processing
        if (msg.arg_in.cat == SNAP_TYPE_BARCODE) {
            qrcode_success = 0;
            stRecvParam.s32RecvPicNum = 1;
        } else {
            stRecvParam.s32RecvPicNum = 1;
        }
        ret = HI_MPI_VENC_StartRecvFrame(profile.venc_chn, &stRecvParam);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "HI_MPI_VENC_StartRecvPic faild with%#x!", ret);
            goto CLEAN;
        }
        ret = HI_MPI_VPSS_TriggerSnapFrame(video_config.vi.device_info.device, profile.vpss_chn,
                                           stRecvParam.s32RecvPicNum);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING,
                     "call HI_MPI_VPSS_TriggerSnapFrame Grp = %d, ChanId = %d, SnapCnt = %d return failed(0x%x)!",
                     video_config.vi.device_info.device, ID_PIC, 1, ret);
            goto CLEAN;
        }
        fd = HI_MPI_VENC_GetFd(profile.venc_chn);
        if (fd < 0) {
            log_goke(DEBUG_WARNING, "HI_MPI_VENC_GetFd faild with%#x!", fd);
            goto CLEAN;
        }
        for (i = 0; i < stRecvParam.s32RecvPicNum; i++) {
            //select
            FD_ZERO(&read_fds);
            FD_SET(fd, &read_fds);
            timeout.tv_sec = 1;         //1s
            timeout.tv_usec = 0;
            ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
            if (ret < 0) {
                log_goke(DEBUG_WARNING, "snap select failed");
                goto CLEAN;
            } else if (0 == ret) {
                goto CLEAN;
            } else {
                if (FD_ISSET(fd, &read_fds)) {
                    //video_osd_write_time(profile.venc_chn );
                    ret = HI_MPI_VENC_QueryStatus(profile.venc_chn, &stStat);
                    if (ret != HI_SUCCESS) {
                        log_goke(DEBUG_WARNING, "HI_MPI_VENC_QueryStatus failed with %#x!", ret);
                        goto CLEAN;
                    }
                    if (0 == stStat.u32CurPacks) {
                        log_goke(DEBUG_WARNING, "NOTE: Current  frame is NULL");
                        goto CLEAN;
                    }
                    stream.pstPack = (VENC_PACK_S *) malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
                    if (NULL == stream.pstPack) {
                        log_goke(DEBUG_WARNING, "malloc memory failed");
                        goto CLEAN;
                    }
                    stream.u32PackCount = stStat.u32CurPacks;
                    ret = HI_MPI_VENC_GetStream(profile.venc_chn, &stream, -1);
                    if (HI_SUCCESS != ret) {
                        log_goke(DEBUG_WARNING, "HI_MPI_VENC_GetStream failed with %#x!", ret);
                        free(stream.pstPack);
                        stream.pstPack = NULL;
                        goto CLEAN;
                    }
                    if (msg.arg_in.cat == SNAP_TYPE_BARCODE ) {
                        ret = zbar_process(&stream, profile.width, profile.height);
                        if (ret == 0) {
                            HI_MPI_VENC_ReleaseStream(profile.venc_chn, &stream);
                            free(stream.pstPack);
                            stream.pstPack = NULL;
                            qrcode_success = 1;
                            goto CLEAN;
                        }
                    } else {
                        memset(filename, 0, sizeof(filename));
                        strcpy(filename, (char *) msg.arg);
                        pFile = fopen(filename, "wb");
                        if (pFile == NULL) {
                            log_goke(DEBUG_WARNING, "open file err!");
                            free(stream.pstPack);
                            stream.pstPack = NULL;
                            goto CLEAN;
                        } else { //save
                            int size = 0;
                            for (i = 0; i < stream.u32PackCount; i++) {
                                fwrite(stream.pstPack[i].pu8Addr + stream.pstPack[i].u32Offset,
                                       stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset, 1, pFile);
                                size += stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset;
                                fflush(pFile);
                            }
                            //post alarm message to cloud
                            if( ((msg.arg_in.cat == SNAP_TYPE_NORMAL) && (msg.arg_in.dog == LV_STORAGE_RECORD_ALARM) )
                                || (msg.arg_in.cat == SNAP_TYPE_CLOUD) ) {
                                lv_intelligent_alarm_param_s param;
                                int service_id;
                                lv_device_auth_s auth;
                                char *image = 0;
                                char title[]="test";
                                //copy image data
                                if( size>0 && size<= 1024*1024 ) {   //1M limits from aliyun
                                    image = malloc(size);
                                    if( image ) {
                                        int pos = 0;
                                        for (i = 0; i < stream.u32PackCount; i++) {
                                            memcpy( &image[pos],stream.pstPack[i].pu8Addr + stream.pstPack[i].u32Offset,
                                                   stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset);
                                            pos += stream.pstPack[i].u32Len - stream.pstPack[i].u32Offset;
                                        }
                                        linkit_get_auth(0, &auth);//这个回调只有主设备才会进入
                                        memset(&param, 0, sizeof(lv_intelligent_alarm_param_s));
                                        param.type = LV_INTELLIGENT_EVENT_MOVING_CHECK;
                                        param.media.p = (char *) image;
                                        param.media.len = size;
                                        param.addition_string.p = title;
                                        param.addition_string.len = strlen(title);
                                        param.format = LV_MEDIA_JPEG;//该字段目前无效
                                        lv_post_intelligent_alarm(&auth, &param, &service_id);
                                        log_goke(DEBUG_INFO, "lv_post_intelligent_alarm, service id = %d\n",
                                                 service_id);
                                        if( image ) {
                                            free(image);
                                        }
                                    } else {
                                        log_goke( DEBUG_SERIOUS, " memory allocation failed in alarm report, size = %d", size);
                                    }
                                } else {
                                    log_goke(DEBUG_WARNING, " alarm image size illegal, size = %d", size);
                                }
                            }
                            fclose(pFile);
                            log_goke(DEBUG_WARNING, "write jpeg success!");
                        }
                    }
                    ret = HI_MPI_VENC_ReleaseStream(profile.venc_chn, &stream);
                    if (HI_SUCCESS != ret) {
                        log_goke(DEBUG_WARNING, "HI_MPI_VENC_ReleaseStream failed with %#x!", ret);
                        free(stream.pstPack);
                        stream.pstPack = NULL;
                        goto CLEAN;
                    }
                    free(stream.pstPack);
                    stream.pstPack = NULL;
                }
            }
        }
        CLEAN:
        ret = HI_MPI_VENC_StopRecvFrame(profile.venc_chn);
        if (ret != HI_SUCCESS) {
            log_goke(DEBUG_WARNING, "HI_MPI_VENC_StopRecvPic failed with %#x!", ret);
        }
        if (msg.arg_in.cat == SNAP_TYPE_BARCODE) {
            if (qrcode_success == 0) {
                //resent message
                message_t send_msg;
                msg_init(&send_msg);
                send_msg.sender = SERVER_ALIYUN;
                send_msg.arg_in.cat = SNAP_TYPE_BARCODE;
                server_video_snap_message(&send_msg);
                if (((int) repeat % 60) == 0) {
                    //play sound
                    /****************************/
                    msg_init(&send_msg);
                    send_msg.message = MSG_AUDIO_PLAY_SOUND;
                    send_msg.arg_in.cat = AUDIO_RESOURCE_SCAN_START;
                    global_common_send_message(SERVER_AUDIO, &send_msg);
                    /****************************/
                }
            }
            repeat++;
        }
        //finish processing
        msg_free(&msg);
    };
    EXIT:
    //exit thread
    msg_buffer_release2(&snap_buff, &snap_mutex);
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit(&info.thread_start, VIDEO_THREAD_ENCODER + channel);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_VIDEO);
    log_goke(DEBUG_INFO, "-----------thread exit: server_video_snap-----------");
    return;
}

/*
 *
 *
 * helper
 *
 *
 */
static int video_set_property(message_t *msg) {
    int ret = 0, mode = -1;
    int temp;
    //
    if (msg->arg_in.cat == VIDEO_PROPERTY_QUALITY) {
        temp = *((int *) (msg->arg));
        if (_config_.video_quality != temp) {
            int val = (temp > 0) ? VIDEO_STREAM_HIGH : VIDEO_STREAM_LOW;
            pthread_rwlock_wrlock(&ilock);
            for (int i = 0; i < MAX_AV_CHANNEL; i++) {
                if ( (channel[i].status == CHANNEL_VALID) && (channel[i].channel_type == SERVER_ALIYUN) ) {
                    if (val != channel[i].stream_type) {
                        channel[i].stream_type = val;
                        channel[i].require_key = 1;
                        log_goke(DEBUG_INFO, "changed the video quality = %d", channel[i].stream_type);
                    }
                }
            }
            pthread_rwlock_unlock(&ilock);
            _config_.video_quality = temp;
            config_set(&_config_);  //save
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.video_quality);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_SUBQUALITY) {
        temp = *((int *) (msg->arg));
        if (_config_.subvideo_quality != temp) {
            _config_.subvideo_quality = temp;
            config_set(&_config_);  //save
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.subvideo_quality);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_MOTION_SWITCH) {
        temp = *((int *) (msg->arg));
        if (temp != _config_.alarm_switch) {
            _config_.alarm_switch = temp;
            log_goke(DEBUG_INFO, "changed the alarm switch = %d", _config_.alarm_switch);
            config_set(&_config_); //save
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.alarm_switch);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_MOTION_SENSITIVITY) {
        temp = *((int *) (msg->arg));
        if (temp != _config_.motion_detect_sensitivity) {
            if( info.status == STATUS_RUN ) {
                MD_ATTR_S attr;
                ret = HI_IVS_MD_GetChnAttr(video_config.md.md_channel, &attr);
                if( ret == HI_SUCCESS ) {
                    attr.u16SadThr = video_config.md.sad_thresh[temp] * (1 << 1);
                    ret = HI_IVS_MD_SetChnAttr(video_config.md.md_channel, &attr);
                    if( ret == HI_SUCCESS ) {
                        log_goke(DEBUG_INFO, "changed the motion detection sensitivity = %d",
                                 _config_.motion_detect_sensitivity);
                        _config_.motion_detect_sensitivity = temp;
                        config_set(&_config_);
                    }
                    else {
                        log_goke( DEBUG_WARNING, " set md attr error with ret = 0x%x", ret);
                    }
                } else {
                    log_goke( DEBUG_WARNING, " get md attr error with ret = 0x%x", ret);
                }
            }
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.motion_detect_sensitivity);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_MOTION_ALARM_INTERVAL) {
        temp = *((int *) (msg->arg));
        if (temp != _config_.alarm_freqency) {
            log_goke(DEBUG_INFO, "changed the motion detection alarm interval = %d", _config_.alarm_freqency);
            _config_.alarm_freqency = temp;
            config_set(&_config_);
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.alarm_freqency);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_IMAGE_FLIP) {
        temp = *((int *) (msg->arg));
        if (_config_.video_mirror != temp) {
            ret = 0;
            for(int i=0;i < VPSS_MAX_PHY_CHN_NUM; i++) {
                if( video_config.vpss.enabled[i]) {
                    video_config.vpss.channel_info[i].bMirror = temp;
                    if( info.status == STATUS_RUN ) {   //only apply change when the video is running
                        ret = HI_MPI_VPSS_SetChnAttr(video_config.vpss.group, i, &video_config.vpss.channel_info[i]);
                        if (ret) { //failed
                            log_goke(DEBUG_WARNING, "flip the video failed! original flip status = %d ",
                                     _config_.video_mirror);
                            video_config.vpss.channel_info[i].bMirror = _config_.video_mirror;
                            break;
                        }
                    }
                }
            }
            if(!ret) {
                _config_.video_mirror = temp;
                log_goke(DEBUG_INFO, "changed the flip property, flip = %d ", _config_.video_mirror);
                config_set(&_config_);  //save
            }
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.video_mirror);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_NIGHT_SHOT) {
        temp = *((int *) (msg->arg));
        if (temp != _config_.day_night_mode) {
            if(temp == 0) {//0:白天
                gk_ircut_daymode();
            }
            else if(temp == 1) {//1:夜市
                gk_ircut_nightmode();
            }
            else {//2:自动

            }
            message_t msg_ircut;
            msg_init(&msg_ircut);
            msg_ircut.message = MSG_MANAGER_TIMER_ADD;
            msg_ircut.sender = SERVER_VIDEO;
            msg_ircut.arg_in.cat = 200;   //200ms
            msg_ircut.arg_in.dog = 0;
            msg_ircut.arg_in.duck = 1;
            msg_ircut.arg_in.handler = &ircut_opt_release;
            global_common_send_message(SERVER_MANAGER, &msg_ircut);
            log_goke(DEBUG_INFO, "changed the night shot mode = %d", _config_.day_night_mode);
            _config_.day_night_mode = temp;
            config_set(&_config_);
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.day_night_mode);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_MOTION_PLAN) {
        if (msg->arg_in.dog != -1) {
            _config_.motion_scheduler[msg->arg_in.dog].begin = msg->arg_in.chick;
            _config_.motion_scheduler[msg->arg_in.dog].end = msg->arg_in.duck;
            _config_.motion_scheduler_enable = 1;
            log_goke(DEBUG_INFO, " add day %d begin=%d end=%d ", msg->arg_in.dog, msg->arg_in.chick, msg->arg_in.duck);
            config_set(&_config_);
        } else {
            _config_.motion_scheduler_enable = 0;
            for (int i = 0; i < 7; i++) {
                _config_.motion_scheduler[i].begin = 0;
                _config_.motion_scheduler[i].end = 0;
            }
            config_set(&_config_);
        }
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_TIME_WATERMARK) {
        temp = *((int *) (msg->arg));
        if (temp != _config_.osd_enable) {
            log_goke(DEBUG_INFO, "changed the time osd mode = %d", _config_.osd_enable);
            _config_.osd_enable = temp;
            config_set(&_config_);
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.osd_enable);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_WDR) {
        temp = *((int *) (msg->arg));
        if (temp != _config_.wdr_enable) {
            /*
             * todo: add code
             */
            log_goke(DEBUG_INFO, "changed the wdr mode = %d", _config_.wdr_enable);
            _config_.wdr_enable = temp;
            config_set(&_config_);
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.wdr_enable);
    } else if (msg->arg_in.cat == VIDEO_PROPERTY_IMAGE_CORRECTION) {
        temp = *((int *) (msg->arg));
        if (temp != _config_.image_correct) {
            if( info.status == STATUS_RUN ) {   //only apply change when the video is running
                VPSS_LDCV3_ATTR_S stLDCV3Attr;
                ret = 0;
                if (temp) {
                    stLDCV3Attr.bEnable = 1;
                    stLDCV3Attr.stAttr.enViewType = LDC_VIEW_TYPE_ALL;   //LDC_VIEW_TYPE_CROP;
                    stLDCV3Attr.stAttr.s32CenterXOffset = 0;
                    stLDCV3Attr.stAttr.s32CenterYOffset = 0;
                    stLDCV3Attr.stAttr.s32DistortionRatio = 200;
                    stLDCV3Attr.stAttr.s32MinRatio = 0;
                } else {
                    stLDCV3Attr.bEnable = 0;
                }
                for (int j = 0; j < VPSS_MAX_PHY_CHN_NUM; j++) {
                    if (HI_TRUE == video_config.vpss.enabled[j]) {
 //                       ret = HI_MPI_VPSS_SetChnLDCV3Attr(video_config.vpss.group, j, &stLDCV3Attr);
                        if (HI_SUCCESS != ret) {
                            log_goke(DEBUG_WARNING, "HI_MPI_VPSS_SetChnLDCV3Attr failed witfh %x\n", ret);
                            break;
                        }
                    }
                }
            }
            if( !ret ) {
                log_goke(DEBUG_INFO, "changed the ldc mode = %d", _config_.image_correct);
                video_config.vpss.ldc = temp;
                _config_.image_correct = temp;
                config_set(&_config_);
            }
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.image_correct);
    }else {
        log_goke(DEBUG_WARNING, " Not support video property: =%d", msg->arg_in.cat);
    }
    return ret;
}

static int write_video_buffer(av_data_info_t *avinfo, char *data, int id, int target, int channel) {
    int ret = 0;
    message_t msg;
    /********message body********/
    msg_init(&msg);
    msg.sender = msg.receiver = SERVER_VIDEO;
    msg.arg_in.wolf = channel;
    msg.message = id;
    msg.arg = data;
    msg.arg_size = avinfo->size;
    msg.extra = avinfo;
    msg.extra_size = sizeof(av_data_info_t);
    if (target == SERVER_ALIYUN)
        ret = global_common_send_message(target, &msg);
    else if (target == SERVER_RECORDER)
        ret = server_recorder_video_message(&msg);
    return ret;
}

static int video_adjust_parameters(video_config_t *config) {
    int i, max;
    SIZE_S pstSize;
    int vpss;
    config->vpss.group_info.u32MaxW = 0;
    config->vpss.group_info.u32MaxH = 0;
    max = 0;
    for (i = MAX_STREAM_NUM - 1; i >= 0 ; i--) {
        if( config->profile.stream[i].enable) {
            if (!hisi_get_pic_size(config->profile.stream[i].size, &pstSize)) {
                config->profile.stream[i].width = pstSize.u32Width;
                config->profile.stream[i].height = pstSize.u32Height;
                if( pstSize.u32Width > config->vpss.group_info.u32MaxW ) {
                    config->vpss.group_info.u32MaxW = pstSize.u32Width;
                    config->vpss.group_info.u32MaxH = pstSize.u32Height;
                }
            }
            vpss = config->profile.stream[i].vpss_chn;
            if( config->profile.stream[i].frame_rate > max ) {
                max = config->profile.stream[i].frame_rate;
            }
            config->vpss.channel_info[vpss].u32Width = config->profile.stream[i].width;
            config->vpss.channel_info[vpss].u32Height = config->profile.stream[i].height;
            config->vpss.enabled[vpss] = 1;
            config->vpss.channel_info[vpss].bMirror = _config_.video_mirror;

            if( _config_.image_correct ) {
                config->vpss.ldc = 1;
            } else {
                config->vpss.ldc = 0;
            }
        }
    }
    ISP_PUB_ATTR_GC2063.f32FrameRate = max;
    if( config->vpss.group_info.u32MaxW == 0 || config->vpss.group_info.u32MaxW == 0 ) {
        return -1;
    }
    return 0;
}

/*
 *
 *
 * video state machine function
 *
 *
 */
static int video_md_check_scheduler_time(void) {
    struct tm *ptm;
    long ts;
    int ret, begin, end;
    struct tm tt = {0};

    if (!_config_.motion_scheduler_enable) {
        return 0;
    }
    ts = time(NULL);
    ptm = localtime_r(&ts, &tt);
    if( _config_.motion_scheduler[ ptm->tm_wday].end != 0 ) {
        ts = ptm->tm_hour * 3600 + ptm->tm_min * 60 + ptm->tm_sec;
        begin = _config_.motion_scheduler[ptm->tm_wday].begin;
        end = _config_.motion_scheduler[ptm->tm_wday].end;

        if (ts >= begin && ts <= end) {
            ret = 0;
        } else {
            ret = 1;
        }
    } else {
        ret = 1;
    }
    return ret;
}

static int video_md_check_scheduler(void) {
    int ret = 0;

    ret = video_md_check_scheduler_time();
    if (!ret) {
        running_info.md_run = 1;
    } else {
        running_info.md_run = 0;
    }

    return ret;
}

static int video_init(void) {
    HI_S32 ret = 0;
    int info = 0;
    HI_S32 chip;
    int i;
    message_t msg;
    //reset running info
    memset(&running_info, 0, sizeof(video_running_info_t));
    //adjust parameters
    ret = video_adjust_parameters(&video_config);
    if (ret) {
        log_goke(DEBUG_SERIOUS, "parameter error!");
        return -1;
    }
    //check chip
    ret = HI_MPI_SYS_GetChipId(&chip);
    if (ret != HI_SUCCESS || chip != CHIP_NAME_GK7205V200) {
        log_goke(DEBUG_SERIOUS, "not support this chip");
        return -1;
    }
    //vi
    ret = hisi_init_vi(&video_config.vi, &info);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_SERIOUS, "start vi failed.ret:0x%x !", ret);
        return ret;
    }
    running_info.vi_init = info;
    //vpss
    ret = hisi_init_vpss(&video_config);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_SERIOUS, "start vpss group failed. ret: 0x%x !", ret);
        return ret;
    }
    running_info.vpss_init = 1;
    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (video_config.profile.stream[i].enable) {
            if (video_config.profile.stream[i].type != PT_JPEG) {
                ret = hisi_init_video_encoder(&video_config, i);
            } else {
                ret = hisi_init_snap_encoder(&video_config, i, HI_FALSE);
            }
            if (HI_SUCCESS != ret) {
                log_goke(DEBUG_SERIOUS, "Venc Start failed for %#x!", ret);
                return ret;
            }
            running_info.venc_init[i] = 1;
        }
    }
    //region
    if (1) {
		#if 0
        ret = video_init_osd_layer(video_config.vi.device_info.device,
                                   &video_config.region.overlay[ID_HIGH]);
        ret = video_init_osd_layer(video_config.vi.device_info.device,
                                   &video_config.region.overlay[ID_LOW]);
        ret = video_init_osd_layer(video_config.vi.device_info.device,
                                   &video_config.region.overlay[ID_PIC]);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "init osd failed. ret: 0x%x !", ret);
            return ret;
        }
		#else
		goke_osd_init();
		#endif
        running_info.region_init = 1;
    }
    //md
    if ( video_config.profile.stream[ID_MD].enable ) {
        int sad_thresh = video_config.md.sad_thresh[ _config_.motion_detect_sensitivity ] * (1 << 1); //100 * (1 << 2);
        ret = hisi_init_md(&video_config.md, sad_thresh,
                           video_config.profile.stream[ID_MD].width,
                           video_config.profile.stream[ID_MD].height);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_SERIOUS, "init md failed. ret: 0x%x !", ret);
            return ret;
        }
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_MANAGER_TIMER_ADD;
        msg.sender = SERVER_VIDEO;
        msg.arg_in.cat = 1000;
        msg.arg_in.dog = 0;
        msg.arg_in.duck = 0;
        msg.arg_in.handler = &video_md_check_scheduler;
        global_common_send_message(SERVER_MANAGER, &msg);
        /****************************/
        running_info.md_init = 1;
        running_info.md_run = 0;
    }
    return ret;
}

static int video_start(void) {
    HI_S32 ret = 0;
    int info = 0;
    int i;

    //vi
    if (!misc_full_bit(running_info.vi_start, VI_INIT_START_BUTT)) {
        ret = hisi_start_vi(&video_config.vi, &info);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_SERIOUS, "start vi failed.ret:0x%x !", ret);
            return ret;
        }
        running_info.vi_start = info;
        //start isp thread
        ret = pthread_create(&running_info.thread_id[VIDEO_THREAD_ISP],
                             NULL, video_isp_func, (void *) &video_config);
        if (ret != 0) {
            log_goke(DEBUG_SERIOUS, "isp thread create error! ret = %d", ret);
            return ret;
        } else {
            log_goke(DEBUG_INFO, "isp thread create successful!");
            misc_set_bit(&running_info.vi_start, VI_INIT_START_ISP);
        }
        //start isp extra thread
        ret = pthread_create(&running_info.thread_id[VIDEO_THREAD_ISP_EXTRA],
                             NULL, video_isp_extra_func, (void *) &video_config);
        if (ret != 0) {
            log_goke(DEBUG_SERIOUS, "isp thread extra create error! ret = %d", ret);
            return ret;
        } else {
            log_goke(DEBUG_INFO, "isp thread extra create successful!");
            misc_set_bit(&running_info.vi_start, VI_INIT_START_ISP);
        }
    }
    //vpss
    if (!running_info.vpss_start) {
        ret = hisi_start_vpss(&video_config);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_SERIOUS, "start vpss failed. ret: 0x%x !", ret);
            return ret;
        }
        running_info.vpss_start = 1;
    }
    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (video_config.profile.stream[i].enable) {
            if (video_config.profile.stream[i].type != PT_JPEG) {
                if (!running_info.venc_start[i]) {
                    ret = hisi_start_video_encoder(&video_config, i);
                    if (HI_SUCCESS != ret) {
                        log_goke(DEBUG_SERIOUS, "Venc Start failed for %#x!", ret);
                        return ret;
                    }
                    //start stream thread
                    ret = pthread_create(&running_info.thread_id[i],
                                         NULL, video_stream_func, (void *) i);
                    if (ret != 0) {
                        hisi_stop_video_encoder(&video_config, i);
                        log_goke(DEBUG_SERIOUS, "encoder thread create error! ret = %d", ret);
                        return ret;
                    } else {
                        log_goke(DEBUG_INFO, "encoder thread create successful!");
                        running_info.venc_start[i] = 1;
                    }
                }
            } else {
                if (!running_info.venc_start[i]) {
                    ret = hisi_start_snap_encoder(&video_config, i);
                    if (HI_SUCCESS != ret) {
                        log_goke(DEBUG_SERIOUS, "Venc Start failed for %#x!", ret);
                        return ret;
                    }
                    //start snap thread
                    ret = pthread_create(&running_info.thread_id[i],
                                         NULL, video_snap_func, (void *) i);
                    if (ret != 0) {
                        hisi_stop_snap_encoder(&video_config, i);
                        log_goke(DEBUG_SERIOUS, "snap thread create error! ret = %d", ret);
                        return ret;
                    } else {
                        log_goke(DEBUG_INFO, "snap thread create successful!");
                        running_info.venc_start[i] = 1;
                    }
                }
            }
        }
    }
    //region
    if (1) {
        if (!running_info.region_start) {
            //start md thread
            ret = pthread_create(&running_info.thread_id[VIDEO_THREAD_OSD],
                                 NULL, video_osd_func, 0);
            if (ret != 0) {
                log_goke(DEBUG_SERIOUS, "osd thread create error! ret = %d", ret);
                return ret;
            } else {
                log_goke(DEBUG_INFO, "osd thread create successful!");
                running_info.region_start = 1;
            }
        }
    }
    if ( video_config.profile.stream[ID_MD].enable) {
        if (!running_info.md_start) {
            //start md thread
            ret = pthread_create(&running_info.thread_id[VIDEO_THREAD_MD],
                                 NULL, video_md_func, 0);
            if (ret != 0) {
                log_goke(DEBUG_SERIOUS, "md thread create error! ret = %d", ret);
                return ret;
            } else {
                log_goke(DEBUG_INFO, "md thread create successful!");
                running_info.md_start = 1;
            }
        }
    }
    return ret;
}

static int video_stop(void) {
    int i, ret, temp;
    //md
    if (running_info.md_start) {
        misc_set_bit(&info.thread_exit, VIDEO_THREAD_MD);
        running_info.md_run = 0;
        running_info.md_start = 0;
    }
    //osd
    if (running_info.region_start) {
        misc_set_bit(&info.thread_exit, VIDEO_THREAD_OSD);
        running_info.region_start = 0;
    }
    //encoder
    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (video_config.profile.stream[i].type != PT_JPEG) {
            if (running_info.venc_start[i]) {
                misc_set_bit(&info.thread_exit, VIDEO_THREAD_ENCODER + i);
                ret = hisi_stop_video_encoder(&video_config, i);
                running_info.venc_start[i] = 0;
            }
        } else {
            misc_set_bit(&info.thread_exit, VIDEO_THREAD_ENCODER + i);
            ret = hisi_stop_snap_encoder(&video_config, i);
            running_info.venc_start[i] = 0;
        }
    }
    //vpss
    if (running_info.vpss_start) {
        hisi_stop_vpss(&video_config);
        running_info.vpss_start = 0;
    }
    //vi
    if (running_info.vi_start) {
        misc_set_bit(&info.thread_exit, VIDEO_THREAD_ISP_EXTRA);
        misc_set_bit(&info.thread_exit, VIDEO_THREAD_ISP);
        hisi_stop_vi(&video_config.vi, &temp);
        running_info.vi_start = temp;
    }
}

static int video_uninit(void) {
    int ret = 0;
    int i;
    int temp = 0;
    message_t msg;
    //md
    if (running_info.md_init) {
        hisi_uninit_md(&video_config.md);
        running_info.md_init = 0;
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_MANAGER_TIMER_REMOVE;
        msg.sender = SERVER_VIDEO;
        msg.arg_in.handler = &video_md_check_scheduler;
        global_common_send_message(SERVER_MANAGER, &msg);
        /****************************/
    }
    //osd
    if (running_info.region_init) {
		#if 0
        ret = video_uninit_osd_layer();
		#else
		goke_osd_exit();
		#endif
        running_info.region_init = 0;
    }
    //encoder
    for (i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        if (video_config.profile.stream[i].type != PT_JPEG) {
            if (running_info.venc_init[i]) {
                ret = hisi_uninit_video_encoder(&video_config, i);
                running_info.venc_init[i] = 0;
            }
        } else {
            ret = hisi_uninit_snap_encoder(&video_config, i);
            running_info.venc_init[i] = 0;
        }
    }
    //vpss
    if (running_info.vpss_init) {
        hisi_uninit_vpss(&video_config);
        running_info.vpss_init = 0;
    }
    //vi
    if (running_info.vi_init) {
        hisi_uninit_vi(&video_config.vi, &temp);
        running_info.vi_init = temp;
    }
    //
    return ret;
}

static void server_thread_termination(int sign) {
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_VIDEO_SIGINT;
    msg.sender = msg.receiver = SERVER_VIDEO;
    global_common_send_message(SERVER_MANAGER, &msg);
    /****************************/
}

static void video_broadcast_thread_exit(void) {
    message_t msg;
    msg.message = MSG_MANAGER_DUMMY;
    server_video_snap_message(&msg);
}

static void server_release_1(void) {
    video_stop();
    video_uninit();
    usleep(1000 * 10);
}

static void server_release_2(void) {
    memset(&channel, 0, sizeof(channel));
    msg_buffer_release2(&message, &mutex);
    memset(&video_config, 0, sizeof(video_config_t));
}

static void server_release_3(void) {
    msg_free(&info.task.msg);
    memset(&info, 0, sizeof(server_info_t));
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_MANAGER_EXIT_ACK;
    msg.sender = SERVER_VIDEO;
    global_common_send_message(SERVER_MANAGER, &msg);
    /***************************/
}

static int server_init(void) {
    int ret = 0;
    message_t msg;
    if (!misc_get_bit(info.init_status, VIDEO_INIT_CONDITION_GOKE)) {
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_GOKE_PROPERTY_GET;
        msg.sender = msg.receiver = SERVER_VIDEO;
        msg.arg_in.cat = GOKE_PROPERTY_STATUS;
        global_common_send_message(SERVER_GOKE, &msg);
        /****************************/
        usleep(MESSAGE_RESENT_SLEEP);
    }
    if (misc_full_bit(info.init_status, VIDEO_INIT_CONDITION_NUM)) {
        info.status = STATUS_WAIT;
    }
    return ret;
}

static int server_setup(void) {
    message_t msg;
    int ret = 0;
    if (video_init() == 0) {
        info.status = STATUS_IDLE;
    } else {
        info.status = STATUS_ERROR;
    }
    return ret;
}

static int server_start(void) {
    int ret = 0;
    int i;
    if (video_start() == 0) {
        if (video_test_full_thread()) {
            info.status = STATUS_RUN;
        }
    } else {
        info.status = STATUS_ERROR;
    }
    return ret;
}

static int server_stop(void) {
    int ret = 0;
    if (video_stop() == 0) {
        if (info.thread_start == 0) {
            info.status = STATUS_IDLE;
        }
    } else {
        info.status = STATUS_ERROR;
    }
    return ret;
}

static int server_restart(void) {
    int ret = 0;
    if (video_stop() == 0) {
        if (info.thread_start == 0) {    //all threads are clean
            if (video_uninit() == 0) {
                info.status = STATUS_IDLE;
            }
        } else {
            info.status = STATUS_ERROR;
        }
    } else {
        info.status = STATUS_ERROR;
    }
    return ret;
}

/*
 *
 *
 * video server function
 *
 *
 */
static int video_message_block(void) {
    int ret = 0;
    int id = -1, id1, index = 0;
    message_t msg;
    //search for unblocked message and swap if necessory
    if (!info.msg_lock) {
        log_goke(DEBUG_MAX, "===video message block, return 0 when first message is msg_lock=0");
        return 0;
    }
    index = 0;
    msg_init(&msg);
    ret = msg_buffer_probe_item(&message, index, &msg);
    if (ret) {
        log_goke(DEBUG_MAX, "===video message block, return 0 when first message is empty");
        return 0;
    }
    if (msg_is_system(msg.message) || msg_is_response(msg.message)) {
        log_goke(DEBUG_MAX, "===video message block, return 0 when first message is system or response message %s",
                 global_common_message_to_string(msg.message));
        return 0;
    }
    id = msg.message;
    do {
        index++;
        msg_init(&msg);
        ret = msg_buffer_probe_item(&message, index, &msg);
        if (ret) {
            log_goke(DEBUG_MAX, "===video message block, return 1 when message index = %d is not found!", index);
            return 1;
        }
        if (msg_is_system(msg.message) ||
            msg_is_response(msg.message)) {    //find one behind system or response message
            msg_buffer_swap(&message, 0, index);
            id1 = msg.message;
            log_goke(DEBUG_INFO, "===video: swapped message happend, message %s was swapped with message %s",
                     global_common_message_to_string(id), global_common_message_to_string(id1));
            return 0;
        }
    } while (!ret);
    return ret;
}

static int video_message_filter(message_t *msg) {
    int ret = 0;
    if (info.status >= STATUS_ERROR) { //only system message
        if (!msg_is_system(msg->message) && !msg_is_response(msg->message))
            return 1;
    }
    return ret;
}


/*
 *
 * message proc
 *
 *
 */
static int server_message_proc(void) {
    int ret = 0;
    message_t msg;
    int id = -1;
    //condition
    pthread_mutex_lock(&mutex);
    if (message.head == message.tail) {
        if (info.status == info.old_status) {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    if (video_message_block()) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    msg_init(&msg);
    ret = msg_buffer_pop(&message, &msg);
    pthread_mutex_unlock(&mutex);
    if (ret == 1)
        return 0;
    if (video_message_filter(&msg)) {
        log_goke(DEBUG_MAX, "VIDEO message filtered: sender=%s, message=%s, head=%d, tail=%d was screened",
                 global_common_get_server_name(msg.sender),
                 global_common_message_to_string(msg.message), message.head, message.tail);
        msg_free(&msg);
        return -1;
    }
    log_goke(DEBUG_MAX, "VIDEO message popped: sender=%s, message=%s, head=%d, tail=%d",
             global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
    switch (msg.message) {
        case MSG_VIDEO_KEY_FRAME_REQUIRE:
            pthread_rwlock_wrlock(&ilock);
            id = get_old_channel(msg.arg_in.wolf, msg.sender);
            if (id != -1) {
                channel[id].require_key = 1;
            }
            pthread_rwlock_unlock(&ilock);
            break;
        case MSG_VIDEO_START:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.task.type = TASK_TYPE_START;
            info.msg_lock = 1;
            break;
        case MSG_VIDEO_STOP:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.task.type = TASK_TYPE_STOP;
            info.msg_lock = 1;
            break;
        case MSG_VIDEO_PROPERTY_SET_DIRECT:
            video_set_property(&msg);
            break;
        case MSG_VIDEO_PROPERTY_GET:
//            video_get_property(&msg);
            break;
        case MSG_VIDEO_PROPERTY_SET_EXT:
            msg_init(&info.task.msg);
            msg_deep_copy(&info.task.msg, &msg);
            info.task.type = TASK_TYPE_RESTART;
            info.msg_lock = 1;
            info.task.start = info.status;
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
        case MSG_GOKE_PROPERTY_NOTIFY:
        case MSG_GOKE_PROPERTY_GET_ACK:
            if (msg.arg_in.cat == GOKE_PROPERTY_STATUS) {
                if (msg.arg_in.dog == STATUS_RUN) {
                    misc_set_bit(&info.init_status, VIDEO_INIT_CONDITION_GOKE);
                }
            }
            break;
        case MSG_MANAGER_EXIT_ACK:
            misc_clear_bit(&info.quit_status, msg.sender);
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "VIDEO: server %s quit and quit lable now = %s",
                     global_common_get_server_name(msg.sender),
                     fp);
            if (fp) {
                free(fp);
            }
            break;
        case MSG_PLAYER_REQUEST_SYNC: {
            int id = -1;
            pthread_rwlock_wrlock(&ilock);
            id = get_old_channel( msg.arg_in.cat, SERVER_ALIYUN );
            if (id != -1) {
                channel[id].status = CHANNEL_EMPTY;
            }
            message_t send_msg;
            msg_init(&send_msg);
            send_msg.message = MSG_PLAYER_REQUEST_SYNC_ACK;
            send_msg.sender = send_msg.receiver = SERVER_VIDEO;
            send_msg.arg_in.cat = msg.arg_in.cat;
            send_msg.arg_in.dog = msg.arg_in.dog;
            global_common_send_message( SERVER_PLAYER, &send_msg);
            pthread_rwlock_unlock(&ilock);
            break;
        }
        default:
            log_goke(DEBUG_SERIOUS, "VIDEO not support message %s",
                     global_common_message_to_string(msg.message));
            break;
    }
    msg_free(&msg);
    return ret;
}

/*
 * task proc
 */
static void task_proc(void) {
    task_finish_enum finished = TASK_FINISH_NO;
    static int para_set = 0;
    int temp;
    int changed = 0;
    //start task
    if (info.task.type == TASK_TYPE_NONE) {
        if (info.status == STATUS_NONE) {
            info.status = STATUS_INIT;
        } else if (info.status == STATUS_WAIT) {
            info.status = STATUS_SETUP;
        } else if (info.status == STATUS_IDLE) {
            info.status = STATUS_START;
        }
    } else if (info.task.type == TASK_TYPE_START) {
        if (info.status == STATUS_NONE) {
            info.status = STATUS_INIT;
        } else if (info.status == STATUS_WAIT) {
            info.status = STATUS_SETUP;
        } else if (info.status == STATUS_IDLE) {
            info.status = STATUS_START;
        } else if (info.status == STATUS_RUN) {
            finished = TASK_FINISH_SUCCESS;
        } else if (info.status >= STATUS_ERROR) {
            finished = TASK_FINISH_FAIL;
        }
        if (finished) {
            message_t msg;
            /**************************/
            msg_init(&msg);
            memcpy(&(msg.arg_pass), &(info.task.msg.arg_pass), sizeof(message_arg_t));
            msg.message = info.task.msg.message | 0x1000;
            msg.sender = msg.receiver = SERVER_VIDEO;
            msg.result = (finished == TASK_FINISH_SUCCESS) ? 0 : -1;
            /***************************/
            if (msg.result == 0) {
                int id = -1;
                pthread_rwlock_wrlock(&ilock);
                id = get_new_channel(info.task.msg.arg_in.wolf, info.task.msg.sender);
                if (channel != -1) {
                    channel[id].stream_type = info.task.msg.arg_in.cat;
                    channel[id].require_key = 1;
                    channel[id].qos_sucess = 0;
                }
                pthread_rwlock_unlock(&ilock);
            }
            global_common_send_message(info.task.msg.receiver, &msg);
            msg_free(&info.task.msg);
            info.task.type = TASK_TYPE_NONE;
            info.msg_lock = 0;
        }
    }
        //stop task
    else if (info.task.type == TASK_TYPE_STOP) {
        if (info.status == STATUS_RUN) {
            int id = -1;
            pthread_rwlock_wrlock(&ilock);
            id = get_old_channel(info.task.msg.arg_in.wolf, info.task.msg.sender);
            if (id != -1) {
                channel[id].status = CHANNEL_EMPTY;
            }
            pthread_rwlock_unlock(&ilock);
            finished = TASK_FINISH_SUCCESS;
        } else if (info.status >= STATUS_ERROR) {
            finished = TASK_FINISH_FAIL;
        } else {
            finished = TASK_FINISH_SUCCESS;
        }
        if (finished) {
            message_t msg;
            /**************************/
            msg_init(&msg);
            memcpy(&(msg.arg_pass), &(info.task.msg.arg_pass), sizeof(message_arg_t));
            msg.message = info.task.msg.message | 0x1000;
            msg.sender = msg.receiver = SERVER_VIDEO;
            msg.result = (finished == TASK_FINISH_SUCCESS) ? 0 : -1;
            global_common_send_message(info.task.msg.receiver, &msg);
            msg_free(&info.task.msg);
            info.task.type = TASK_TYPE_NONE;
            info.msg_lock = 0;
        }
    } else if (info.task.type == TASK_TYPE_RESTART) {
        if (info.status == STATUS_WAIT) {
            if (info.task.msg.arg_in.cat == VIDEO_PROPERTY_TIME_WATERMARK) {
                temp = *((int *) (info.task.msg.arg));
            }
            para_set = 1;
            if (info.task.start == STATUS_WAIT)
                finished = TASK_FINISH_SUCCESS;
            else
                info.status = STATUS_SETUP;
        } else if (info.status == STATUS_IDLE) {
            if (!para_set)
                info.status = STATUS_RESTART;
            else {
                if (info.task.start == STATUS_IDLE)
                    finished = TASK_FINISH_SUCCESS;
                else
                    info.status = STATUS_START;
            }
        } else if (info.status == STATUS_RUN) {
            if (!para_set) {
                if (info.task.msg.arg_in.cat == VIDEO_PROPERTY_TIME_WATERMARK) {
                    temp = *((int *) (info.task.msg.arg));
                    //no need to change
                    finished = TASK_FINISH_SUCCESS;
                } else {
                    info.status = STATUS_RESTART;
                }
            } else {
                finished = TASK_FINISH_SUCCESS;
            }
        } else {
            finished = TASK_FINISH_SUCCESS;
        }
    }
    if (finished) {
        message_t msg;
        /**************************/
        msg_init(&msg);
        memcpy(&(msg.arg_pass), &(info.task.msg.arg_pass), sizeof(message_arg_t));
        msg.message = info.task.msg.message | 0x1000;
        msg.sender = msg.receiver = SERVER_VIDEO;
        msg.arg_in.wolf = changed;
        msg.result = (finished == TASK_FINISH_SUCCESS) ? 0 : -1;
        msg.arg = &temp;
        msg.arg_size = sizeof(temp);
        /***************************/
        global_common_send_message(info.task.msg.receiver, &msg);
        msg_free(&info.task.msg);
        info.task.type = TASK_TYPE_NONE;
        info.msg_lock = 0;
        para_set = 0;
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
        case STATUS_ERROR:
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case EXIT_INIT:
            log_goke(DEBUG_INFO, "VIDEO: switch to exit task!");
            if (info.task.msg.sender == SERVER_MANAGER) {
                info.quit_status &= (info.task.msg.arg_in.cat);
            } else {
                info.quit_status = 0;
            }
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "VIDEO: Exit precondition =%s", fp);
            if (fp) {
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
            video_broadcast_thread_exit();
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
            log_goke(DEBUG_SERIOUS, "!!!!!!!unprocessed server status in state_machine = %d", info.status);
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
    misc_set_thread_name("server_video");
    pthread_detach(pthread_self());
    msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_normal, &mutex);
    info.quit_status = VIDEO_EXIT_CONDITION;
    info.init = 1;
    while (!info.exit) {
        info.old_status = info.status;
        state_machine();
        task_proc();
        server_message_proc();
    }
    server_release_3();
    log_goke(DEBUG_INFO, "-----------thread exit: server_video-----------");
    pthread_exit(0);
}

/*
 * internal interface
 */

/*
 * external interface
 */
int server_video_start(void) {
    int ret = -1;
    ret = pthread_create(&info.id, NULL, server_func, NULL);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "video server create error! ret = %d", ret);
        return ret;
    } else {
        log_goke(DEBUG_INFO, "video server create successful!");
        return 0;
    }
}

int server_video_message(message_t *msg) {
    int ret = 0;
    pthread_mutex_lock(&mutex);
    if (!message.init) {
        log_goke(DEBUG_MAX, "VIDEO server is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message));
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    ret = msg_buffer_push(&message, msg);
    log_goke(DEBUG_MAX, "VIDEO message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret, message.head, message.tail);
    if (ret != 0)
        log_goke(DEBUG_WARNING, "VIDEO message push in error =%d", ret);
    else {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}

int server_video_snap_message(message_t *msg) {
    int ret = 0;
    pthread_mutex_lock(&snap_mutex);
    if ((!snap_buff.init)) {
        log_goke(DEBUG_MAX, "VIDEO-SNAP ENGINE is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message));
        pthread_mutex_unlock(&snap_mutex);
        return -1;
    }
    ret = msg_buffer_push(&snap_buff, msg);
    log_goke(DEBUG_MAX, "VIDEO-SNAP message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret, message.head, message.tail);
    if (ret != 0)
        log_goke(DEBUG_WARNING, "VIDEO-SNAP message push in error =%d", ret);
    else {
        pthread_cond_signal(&snap_cond);
    }
    pthread_mutex_unlock(&snap_mutex);
    return ret;
}