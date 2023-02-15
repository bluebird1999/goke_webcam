/*
 * header
 */
//system header
#include <pthread.h>
#include <stdio.h>
#include <signal.h>
#include <malloc.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <link_visual_api.h>
#include <iot_export_linkkit.h>
#include <iot_export.h>
#include <iot_import.h>
//program header
#include "../../common/tools_interface.h"
#include "../../server/aliyun/aliyun_interface.h"
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"
#include "../../server/recorder/recorder_interface.h"
#include "../../server/goke/goke_interface.h"
#include "../../server/video/video_interface.h"
#include "../../server/device/gk_gpio.h"
//server header
#include "audio.h"
#include "config.h"
#include "audio_interface.h"

/*
 * static
 */

//variable
static 	message_buffer_t	message;
static 	server_info_t 		info;
static  audio_running_info_t running_info;
static  pthread_rwlock_t	ilock = PTHREAD_RWLOCK_INITIALIZER;
static	pthread_mutex_t		mutex = PTHREAD_MUTEX_INITIALIZER;
static	pthread_cond_t		cond = PTHREAD_COND_INITIALIZER;
static  audio_channel_t     channel[MAX_AV_CHANNEL];

//function
//common
static void *server_func(void);
static int server_message_proc(void);
static void server_release_1(void);
static void server_release_2(void);
static void server_release_3(void);
static void server_thread_termination(void);
static void task_default(void);
static void task_start(void);
static void task_stop(void);
//specific
static int stream_init(void);
static int stream_destroy(void);
static int stream_start(void);
static int stream_stop(void);
static int audio_init(void);
static int write_audio_buffer(av_data_info_t *avinfo, char *data, int id, int target, int type);
static void write_audio_info(struct rts_av_buffer *data, av_data_info_t	*info);

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */
static void init_intercom_stream_config(int service_id) {
    lv_video_param_s video_param;
    lv_audio_param_s audio_param;
    lv_stream_send_config_param_s send_config;

    memset(&video_param, 0, sizeof(lv_video_param_s));

    memset(&audio_param, 0, sizeof(lv_audio_param_s));
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
    send_config.bitrate_kbps = 128;
    send_config.video_param = &video_param;
    send_config.audio_param = &audio_param;
    lv_stream_send_config(service_id, &send_config);
}

static int audio_set_property(message_t *msg) {
    int ret = 0, mode = -1;
    int temp;
    //
    if (msg->arg_in.cat == AUDIO_PROPERTY_SPEAKER) {
        temp = *((int *) (msg->arg));
        if (_config_.speaker_enable != temp) {
            HI_MPI_AO_SetMute( audio_config.ao_channel, !temp, 0);
            if( temp ) {
                gk_enable_speaker();
            } else {
                gk_disable_speaker();
            }
            _config_.speaker_enable = temp;
            config_set(&_config_);  //save
        }
        linkkit_sync_property_int(msg->arg_in.wolf, msg->extra, _config_.speaker_enable);
    } else {
        log_goke(DEBUG_WARNING, " Not support audio property: =%d", msg->arg_in.cat);
    }
    return ret;
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
    for( i=0; i<MAX_AV_CHANNEL; i++ ) {
        if( channel[i].status != CHANNEL_VALID ) {
            channel[i].service_id = id;
            channel[i].channel_type = type;
            channel[i].status = CHANNEL_VALID;
            return i;
        }
    }
    return -1;
}
/*
 *
 *
 * thread func
 *
 *
 */
void* aduio_stream_func(void* arg)
{
    int i, ret, status;
    audio_stream_info_t stream_info;
    AUDIO_STREAM_S stream;
	AI_CHN_PARAM_S stAiChnParam;
    struct timeval timeout;
    fd_set  read_fds;
    int     fd;
    av_data_info_t avinfo;
    char *data;
    int     number = 0;
    //thread preparation
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    misc_set_thread_name("server_audio_stream");
    stream_info.channel = 0;
    log_goke(DEBUG_INFO, "-----------thread start: audio_stream id=%d-----------", stream_info.channel);
    pthread_detach(pthread_self());
    pthread_rwlock_wrlock(&ilock);
    misc_set_bit( &info.thread_start, AUDIO_THREAD_ENCODER);
    pthread_rwlock_unlock(&ilock);
    //init
    FD_ZERO(&read_fds);
	stAiChnParam.u32UsrFrmDepth = 30;
	HI_MPI_AI_SetChnParam(0,0,&stAiChnParam);
    fd = HI_MPI_AENC_GetFd(stream_info.channel);
    if (fd < 0) {
        log_goke(DEBUG_WARNING,"HI_MPI_AENC_GetFd failed with %#x!", fd);
        return NULL;
    }
    FD_SET(fd, &read_fds);
    //
    global_common_send_dummy(SERVER_AUDIO);
    while ( 1 ) {
        //status check
        if( info.exit ) break;
        if( misc_get_bit(info.thread_exit, AUDIO_THREAD_ENCODER) )
            break;
        status = info.status;
        if( status != STATUS_RUN )
            continue;
        //selector
        FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
        if (ret < 0) {
            break;
        }
        else if (0 == ret) {
            log_goke(DEBUG_WARNING,"%s: get aenc stream select time out", __FUNCTION__);
            break;
        }
        if (FD_ISSET(fd, &read_fds)) {
            /* get stream from aenc chn */
            ret = HI_MPI_AENC_GetStream( stream_info.channel, &stream, HI_FALSE);
            if (HI_SUCCESS != ret ) {
                log_goke(DEBUG_WARNING,"%s: HI_MPI_AENC_GetStream(%d), failed with %#x!", \
                       __FUNCTION__,  stream_info.channel, ret);
                continue;
            }

            if( stream.u32Len > MAX_AUDIO_FRAME_SIZE ) {
                log_goke(DEBUG_WARNING, "++++++++++++++++++++realtek audio frame size=%d!!!!!!", stream.u32Len);
                goto RELEASE;
            }
            data = malloc( stream.u32Len );
            if( data == NULL) {
                log_goke(DEBUG_WARNING, "allocate memory failed in audio buffer, size=%d", stream.u32Len);
                ret = HI_MPI_AENC_ReleaseStream( stream_info.channel, &stream);
                break;
            }
            //remove 4 bytes hisi G711A header
            memcpy(data, stream.pStream + 4, stream.u32Len - 4);
            //
            if( (stream_info.goke_stamp == 0) && (stream_info.unix_stamp == 0) ) {
                stream_info.goke_stamp = stream.u64TimeStamp;
                stream_info.unix_stamp = time_get_now_stamp();
            }
            //***write audio info
            avinfo.flag = 0;
            avinfo.frame_index = stream.u32Seq;
            avinfo.timestamp = (unsigned int)((time_get_now_ms() - _universal_tag_) );
            avinfo.size = stream.u32Len - 4;
            avinfo.source = STREAM_SOURCE_LIVE;
            avinfo.channel = 0;
            avinfo.type = MEDIA_TYPE_AUDIO;
            //***iterate for different stream destination
            for (i = 0; i < MAX_AV_CHANNEL; i++) {
                if ( channel[i].status == CHANNEL_VALID ) {
                    if( channel[i].channel_type == SERVER_RECORDER ) {
                        ret = write_audio_buffer(&avinfo, data, MSG_RECORDER_AUDIO_DATA, SERVER_RECORDER,
                                                 channel[i].service_id);
                        if (ret) {
                            log_goke(DEBUG_WARNING, "recorder audio send failed! ret = %d", ret);
                        }
                    }
                    else if( channel[i].channel_type == SERVER_ALIYUN ) {
                        ret = audio_video_synchronize( channel[i].service_id );
                        if( !ret ) {
                            lv_stream_send_media_param_s param = {{0}};
                            param.common.type = LV_STREAM_MEDIA_AUDIO;
                            param.common.p = (char *) data;
                            param.common.len = avinfo.size;
                            param.common.timestamp_ms = avinfo.timestamp;
                            param.audio.format = LV_AUDIO_FORMAT_G711A;
                            ret = lv_stream_send_media(channel[i].service_id, &param);
                            if (ret) {
                                log_goke(DEBUG_WARNING, "aliyun audio send failed, ret=%x, service_id = %d", ret,
                                         channel[i].service_id);
                            }
                        } else {
                            log_goke(DEBUG_INFO, "aliyun audio channel waiting for video stream init, service_id = %d", channel[i].service_id);
                        }
                    }
                    else if( channel[i].channel_type == SERVER_AUDIO ) {
                        if( channel[i].require_key ) {
                            init_intercom_stream_config( channel[i].service_id );
                            channel[i].require_key = 0;
                        }
                        lv_stream_send_media_param_s param = {{0}};
                        param.common.type = LV_STREAM_MEDIA_AUDIO;
                        param.common.p = (char *) data;
                        param.common.len = avinfo.size;
                        param.common.timestamp_ms = avinfo.timestamp;
                        param.audio.format = LV_AUDIO_FORMAT_G711A;
                        ret = lv_stream_send_media(channel[i].service_id, &param);
                        if (ret) {
                            log_goke(DEBUG_WARNING, "aliyun audio send failed, ret=%x, service_id = %d", ret,
                                     channel[i].service_id);
                        }
                    }
                }
            }
            if( data) {
                free(data);
                data = 0;
            }
            RELEASE:
            /* finally you must release the stream */
            ret = HI_MPI_AENC_ReleaseStream( stream_info.channel, &stream);
            if (HI_SUCCESS != ret ) {
                log_goke(DEBUG_WARNING,"%s: HI_MPI_AENC_ReleaseStream(%d), failed with %#x!", \
                       __FUNCTION__,  stream_info.channel, ret);
                break;
            }
        }
    }
    //release
EXIT:
    pthread_rwlock_wrlock(&ilock);
    misc_clear_bit( &info.thread_start, AUDIO_THREAD_ENCODER);
    pthread_rwlock_unlock(&ilock);
    global_common_send_dummy(SERVER_AUDIO);
    log_goke(DEBUG_INFO, "-----------thread exit: audio_stream-----------");
    pthread_exit(0);
}

/*
 * helper
 */
static int audio_speaker(char *buffer, unsigned int buffer_size, bool end) {
    HI_S32 ret;
    AUDIO_STREAM_S stAudioStream;
    if( buffer_size > 0 ) {
        stAudioStream.pStream = buffer;
        stAudioStream.u32Len = buffer_size;

        ret = HI_MPI_ADEC_SendStream(audio_config.ad_channel, &stAudioStream, HI_TRUE);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "%s: HI_MPI_ADEC_SendStream(%d) failed with %#x!", \
                   __FUNCTION__, audio_config.ad_channel, ret);
            return 1;
        }
    }
    if( end ) {
        ret = HI_MPI_ADEC_SendEndOfStream(audio_config.ad_channel, HI_FALSE);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "%s: HI_MPI_ADEC_SendEndOfStream failed!", __FUNCTION__);
            return 1;
        }
    }
    return 0;
}

static int play_audio_file(char *path)
{
    int ret;
    FILE *fp = NULL;
    char *buffer;
    if(access(path, R_OK)) {
        log_goke(DEBUG_SERIOUS, "can not find %s", path);
        return -1;
    }
    fp = fopen(path, "rb");
    if (!fp) {
        log_goke(DEBUG_SERIOUS, "fail to open %s: %s", path, strerror(errno));
        ret = -1;
        goto EXIT;
    }
    buffer = malloc( audio_config.aio_attr.u32PtNumPerFrm + 4 );
    while (1) {
        memset(buffer, 0, audio_config.aio_attr.u32PtNumPerFrm + 4);
        ret = fread(buffer + 4, 1,  audio_config.aio_attr.u32PtNumPerFrm, fp);
        if( ret>=0 && feof(fp)){
            hisi_add_audio_header(buffer, ret);
            audio_speaker(buffer, ret + 4, 1);
            log_goke(DEBUG_INFO, "finish");
            ret = 0;
            break;
        }else if(ferror(fp)) {
            log_goke(DEBUG_SERIOUS, "fread error");
            ret = -1;
            break;
        }else {
            hisi_add_audio_header(buffer, ret);
            audio_speaker(buffer, ret + 4, 0);
        }
    }
    free(buffer);
    fclose(fp);
    EXIT:
    return ret;
}

static int audio_play(int type)
{
    int ret;
    switch(type) {
        case AUDIO_RESOURCE_SCAN_START:
            play_audio_file( AUDIO_SCAN_START );
            break;
        case AUDIO_RESOURCE_SCAN_SUCCESS:
            play_audio_file( AUDIO_SCAN_SUCCESS );
            break;
        case AUDIO_RESOURCE_BINDING_SUCCESS:
            play_audio_file( AUDIO_BINDING_SUCCESS );
            break;
        case AUDIO_RESOURCE_START:
            play_audio_file( AUDIO_START );
            break;
    }
    return ret;
}

static int audio_init(void)
{
	int ret;
    //init ai
    ret = hisi_init_ai(&audio_config);
    if( ret ) {
        log_goke(DEBUG_WARNING,"%s: get chip id failed with %d!", __FUNCTION__, ret);
        return -1;
    }
    running_info.ai_init = 1;
    //init audio codec
    ret = hisi_init_audio_codec(&audio_config);
    if( ret ) {
        log_goke(DEBUG_WARNING,"%s: get chip id failed with %d!", __FUNCTION__, ret);
        return -1;
    }
    running_info.codec_init = 1;
    //init ae
    ret = hisi_init_ae(&audio_config);
    if( ret ) {
        log_goke(DEBUG_WARNING,"%s: get chip id failed with %d!", __FUNCTION__, ret);
        return -1;
    }
    running_info.ae_init = 1;
    //init ao
    ret = hisi_init_ao(&audio_config);
    if( ret ) {
        log_goke(DEBUG_WARNING,"%s: get chip id failed with %d!", __FUNCTION__, ret);
        return -1;
    }
    running_info.ao_init = 1;
    //init ad
    ret = hisi_init_ad(&audio_config);
    if( ret ) {
        log_goke(DEBUG_WARNING,"%s: get chip id failed with %d!", __FUNCTION__, ret);
        return -1;
    }
    running_info.ad_init = 1;
	return 0;
}

static int audio_start(void)
{
    HI_S32      ret = 0;
    int         info=0;
    int         i;
    //ai
    if( !running_info.ai_start ) {
        ret = hisi_start_ai(&audio_config);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "start ai failed.ret:0x%x !", ret);
            return ret;
        }
        running_info.ai_start = 1;
    }
    //ae
    if( !running_info.ae_start ) {
        ret = hisi_start_ae(&audio_config);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "start ae failed. ret: 0x%x !", ret);
            return ret;
        }
        //start stream thread
        ret = pthread_create(&running_info.thread_id,
                             NULL, aduio_stream_func, 0);
        if (ret != 0) {
            hisi_stop_ae(&audio_config);
            log_goke(DEBUG_SERIOUS, "isp thread create error! ret = %d", ret);
            return ret;
        } else {
            log_goke(DEBUG_INFO, "isp thread create successful!");
            running_info.ae_start = 1;
        }
    }
    //ad
    if( !running_info.ad_start ) {
        ret = hisi_start_ad(&audio_config);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "start ai failed.ret:0x%x !", ret);
            return ret;
        }
        running_info.ad_start = 1;
    }
    //ao
    if( !running_info.ao_start ) {
        ret = hisi_start_ao(&audio_config);
        if (HI_SUCCESS != ret) {
            log_goke(DEBUG_WARNING, "start ai failed.ret:0x%x !", ret);
            return ret;
        }
        running_info.ao_start = 1;
    }
    //binding
    if( !running_info.ae_ai_bind ) {
        ret = hisi_bind_ae_ai(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ae_ai_bind = 1;
    }
    if( !running_info.ao_ad_bind ) {
        ret = hisi_bind_ao_ad(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ao_ad_bind = 1;
    }
    return ret;
}

static int audio_stop(void)
{
    int i,ret;
    int info = 0;
    //unbind
    if( running_info.ao_ad_bind ) {
        ret = hisi_unbind_ao_ad(&audio_config);
        if( ret ) {
            log_goke(DEBUG_WARNING,"%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ao_ad_bind = 0;
    }
    if( running_info.ae_ai_bind ) {
        ret = hisi_unbind_ae_ai(&audio_config);
        if( ret ) {
            log_goke(DEBUG_WARNING,"%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ae_ai_bind = 0;
    }
    //ao
    if( running_info.ao_start ) {
        ret = hisi_stop_ao(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ao_start = 0;
    }
    //ad
    if( running_info.ad_start ) {
        ret = hisi_stop_ad(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ad_start = 0;
    }
    //ae
    if( running_info.ae_start ) {
        ret = hisi_stop_ae(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ae_start = 0;
    }
    //ai
    if( running_info.ai_start ) {
        ret = hisi_stop_ai(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ai_start = 0;
    }
    return 0;
}

static int audio_uninit(void)
{
    int ret;
    //ao
    if( running_info.ao_init ) {
        ret = hisi_uninit_ao(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ao_init = 0;
    }
    //ad
    if( running_info.ad_init ) {
        ret = hisi_uninit_ad(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ad_init = 0;
    }
    //ae
    if( running_info.ae_init ) {
        ret = hisi_uninit_ae(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ae_init = 0;
    }
    //ai
    if( running_info.ai_init ) {
        ret = hisi_uninit_ai(&audio_config);
        if (ret) {
            log_goke(DEBUG_WARNING, "%s: get chip id failed with %d!", __FUNCTION__, ret);
            return -1;
        }
        running_info.ai_init = 0;
    }
//    HI_MPI_AENC_AacDeInit();
//    HI_MPI_ADEC_AacDeInit();
    return 0;
}

static int write_audio_buffer(av_data_info_t *avinfo, char *data, int id, int target, int channel)
{
	int ret=0;
	message_t msg;
    /********message body********/
	msg_init(&msg);
	msg.arg_in.wolf = channel;
	msg.sender = msg.receiver = SERVER_AUDIO;
	msg.message = id;
    msg.arg = data;
    msg.arg_size = avinfo->size;	//make sure this is 0 for non-deep-copy
    msg.extra = avinfo;
    msg.extra_size = sizeof(av_data_info_t);
	if( target == SERVER_ALIYUN )
		ret = server_aliyun_message(&msg);
    else if( target == SERVER_RECORDER )
		ret = server_recorder_audio_message(&msg);
	return ret;
	/****************************/
}


static void server_thread_termination(void)
{
	message_t msg;
    /********message body********/
	msg_init(&msg);
	msg.message = MSG_AUDIO_SIGINT;
	msg.sender = msg.receiver = SERVER_AUDIO;
	/****************************/
	global_common_send_message(SERVER_MANAGER, &msg);
}

static void audio_broadcast_thread_exit(void)
{
}

static void server_release_1(void)
{
    audio_stop();
    audio_uninit();
	usleep(1000*10);
}

static void server_release_2(void)
{
	msg_buffer_release2(&message, &mutex);
}

static void server_release_3(void)
{
	msg_free(&info.task.msg);
	memset(&info, 0, sizeof(server_info_t));
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_MANAGER_EXIT_ACK;
    msg.sender = SERVER_AUDIO;
    global_common_send_message(SERVER_MANAGER, &msg);
    /***************************/
}
/*
 * State Machine
 */
static int audio_message_block(void)
{
	int ret = 0;
	int id = -1, id1, index = 0;
	message_t msg;
	//search for unblocked message and swap if necessory
	if( !info.msg_lock ) {
		log_goke(DEBUG_VERBOSE, "===audio message block, return 0 when first message is msg_lock=0");
		return 0;
	}
	index = 0;
	msg_init(&msg);
	ret = msg_buffer_probe_item(&message, index, &msg);
	if( ret ) {
		log_goke(DEBUG_VERBOSE, "===audio message block, return 0 when first message is empty");
		return 0;
	}
	if( msg_is_system(msg.message) || msg_is_response(msg.message) ) {
		log_goke(DEBUG_VERBOSE, "===audio message block, return 0 when first message is system or response message %s",
                 global_common_message_to_string(msg.message) );
		return 0;
	}
	id = msg.message;
	do {
		index++;
		msg_init(&msg);
		ret = msg_buffer_probe_item(&message, index, &msg);
		if(ret) {
			log_goke(DEBUG_VERBOSE, "===audio message block, return 1 when message index = %d is not found!", index);
			return 1;
		}
		if( msg_is_system(msg.message) ||
				msg_is_response(msg.message) ) {	//find one behind system or response message
			msg_buffer_swap(&message, 0, index);
			id1 = msg.message;
			log_goke(DEBUG_INFO, "===AUDIO: swapped message happend, message %s was swapped with message %s",
                     global_common_message_to_string(id), global_common_message_to_string(id1) );
			return 0;
		}
	}
	while(!ret);
	return ret;
}

static int audio_message_filter(message_t  *msg)
{
	int ret = 0;
	if( info.status >= STATUS_ERROR) { //only system message
		if( !msg_is_system(msg->message) && !msg_is_response(msg->message) )
			return 1;
	}
	return ret;
}

static int server_message_proc(void)
{
	int ret = 0;
	message_t msg;
	//condition
	pthread_mutex_lock(&mutex);
	if( message.head == message.tail ) {
		if( (info.status == info.old_status ) ) {
			pthread_cond_wait(&cond,&mutex);
		}
	}
	if( audio_message_block() ) {
		pthread_mutex_unlock(&mutex);
		return 0;
	}
	msg_init(&msg);
	ret = msg_buffer_pop(&message, &msg);
	pthread_mutex_unlock(&mutex);
	if( ret == 1)
		return 0;
	if( audio_message_filter(&msg) ) {
		log_goke(DEBUG_VERBOSE, "AUDIO message filtered: sender=%s, message=%s, head=%d, tail=%d was screened",
                 global_common_get_server_name(msg.sender),
                 global_common_message_to_string(msg.message), message.head, message.tail);
        msg_free(&msg);
		return -1;
	}
	log_goke(DEBUG_VERBOSE, "AUDIO message popped: sender=%s, message=%s, head=%d, tail=%d",
             global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
	/**************************/
	switch(msg.message) {
		case MSG_AUDIO_START:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.task.type = TASK_TYPE_START;
            info.msg_lock = 1;
			break;
		case MSG_AUDIO_STOP:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.task.type = TASK_TYPE_STOP;
            info.msg_lock = 1;
			break;
		case MSG_MANAGER_EXIT:
			msg_init(&info.task.msg);
			msg_copy(&info.task.msg, &msg);
			info.status = EXIT_INIT;
			info.msg_lock = 0;
			break;
		case MSG_MANAGER_TIMER_ON:
			((HANDLER)msg.arg_in.handler)();
			break;
		case MSG_GOKE_PROPERTY_NOTIFY:
		case MSG_GOKE_PROPERTY_GET_ACK:
            if (msg.arg_in.cat == GOKE_PROPERTY_STATUS) {
                if (msg.arg_in.dog == STATUS_RUN) {
                    misc_set_bit(&info.init_status, AUDIO_INIT_CONDITION_GOKE);
                }
            }
            break;
		case MSG_MANAGER_EXIT_ACK:
            misc_clear_bit(&info.quit_status, msg.sender);
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "AUDIO: server %s quit and quit lable now = %s",
                     global_common_get_server_name(msg.sender),
                     fp);
            if(fp) {
                free(fp);
            }
            break;
		case MSG_MANAGER_DUMMY:
			break;
        case MSG_AUDIO_PLAY_SOUND:
            if( info.status == STATUS_RUN ) {
                audio_play(msg.arg_in.cat);
            } else {
                log_goke( DEBUG_WARNING, "Audio server is not ready to play sound!");
            }
            break;
        case MSG_AUDIO_SPEAKER_START: {
            break;
        }
        case MSG_AUDIO_SPEAKER_STOP: {
            break;
        }
        case MSG_AUDIO_PROPERTY_SET:
            audio_set_property(&msg);
            break;
        case MSG_AUDIO_PROPERTY_GET:
//          audio_get_property(&msg);
            break;
        case MSG_AUDIO_SPEAKER_DATA:
        	if( msg.arg_in.cat == SPEAKER_CTL_INTERCOM_DATA ) {
        		if(msg.arg) {
					audio_speaker(msg.arg, msg.arg_size, 0);
				}
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
            send_msg.sender = send_msg.receiver = SERVER_AUDIO;
            send_msg.message = MSG_PLAYER_REQUEST_SYNC_ACK;
            send_msg.arg_in.cat = msg.arg_in.cat;
            send_msg.arg_in.dog = msg.arg_in.dog;
            global_common_send_message( SERVER_PLAYER, &send_msg);
            pthread_rwlock_unlock(&ilock);
            break;
        }
		default:
            log_goke(DEBUG_SERIOUS, "AUDIO not support message %s",
                     global_common_message_to_string(msg.message));
			break;
	}
	msg_free(&msg);
	return ret;
}


/*
 *
 */
static int server_init(void)
{
    int ret = 0;
    message_t msg;
    if( !misc_get_bit( info.init_status, AUDIO_INIT_CONDITION_GOKE ) ) {
        /********message body********/
        msg_init(&msg);
        msg.message = MSG_GOKE_PROPERTY_GET;
        msg.sender = msg.receiver = SERVER_AUDIO;
        global_common_send_message(SERVER_GOKE, &msg);
        /****************************/
        usleep(MESSAGE_RESENT_SLEEP);
    }
    if( misc_full_bit( info.init_status, AUDIO_INIT_CONDITION_NUM ) ) {
        info.status = STATUS_WAIT;
    }
    return ret;
}

static int server_setup(void)
{
    int ret = 0;
    if( audio_init() == 0)
        info.status = STATUS_IDLE;
    else
        info.status = STATUS_ERROR;
    return ret;
}

static int server_start(void)
{
    int ret = 0;
    if( audio_start()==0 ) {
        if( misc_get_bit(info.thread_start, AUDIO_THREAD_ENCODER) ) {
            info.status = STATUS_RUN;
            audio_play( AUDIO_RESOURCE_START );
        }
    }
    else {
        info.status = STATUS_ERROR;
    }
    return ret;
}

static int server_stop(void)
{
    int ret = 0;
    if( audio_stop()==0 ) {
        if (info.thread_start == 0) {    //all threads are clean
            info.status = STATUS_IDLE;
        }
    }
    else {
        info.status = STATUS_ERROR;
    }
    return ret;
}

static int server_restart(void)
{
    int ret = 0;
    if( audio_stop() == 0 ) {
        if( audio_uninit() == 0 ) {
            if (info.status2 == 0) {    //all threads are clean
                info.status = STATUS_IDLE;
            }
        }
        else {
            info.status = STATUS_ERROR;
        }
    }
    else {
        info.status = STATUS_ERROR;
    }
    return ret;
}

/*
 *
 *
 *
 * task proc
 *
 *
 *
 *
 */
static void task_proc(void)
{
    task_finish_enum finished = TASK_FINISH_NO;
    int temp;
    //start task
    if (info.task.type == TASK_TYPE_NONE) {
        if (info.status == STATUS_NONE) {
            info.status = STATUS_INIT;
        } else if (info.status == STATUS_WAIT) {
            info.status = STATUS_SETUP;
        } else if (info.status == STATUS_IDLE) {
            info.status = STATUS_START;
        }
    } else if( info.task.type == TASK_TYPE_START ) {
        if( info.status == STATUS_RUN ) {
            finished = TASK_FINISH_SUCCESS;
        }
        else if( info.status == STATUS_NONE ) {
            info.status = STATUS_INIT;
        }
        else if( info.status == STATUS_WAIT ) {
            info.status = STATUS_SETUP;
        }
        else if( info.status == STATUS_IDLE ) {
            info.status = STATUS_START;
        }
        else if( info.status >= STATUS_ERROR ) {
            finished = TASK_FINISH_FAIL;
        }
        if( finished ) {
            message_t msg;
            /**************************/
            msg_init(&msg);
            memcpy(&(msg.arg_pass), &(info.task.msg.arg_pass), sizeof(message_arg_t));
            msg.message = info.task.msg.message | 0x1000;
            msg.sender = msg.receiver = SERVER_VIDEO;
            msg.result = (finished == TASK_FINISH_SUCCESS) ? 0: -1;
            /***************************/
            if (msg.result == 0) {
                int id = -1;
                pthread_rwlock_wrlock(&ilock);
                id = get_new_channel( info.task.msg.arg_in.wolf, info.task.msg.sender);
                if (info.task.msg.arg_in.cat == 2) { //intercom
                    channel[id].require_key = 1;
                    channel[id].channel_type = SERVER_AUDIO;
                }
                pthread_rwlock_unlock(&ilock);
            }
            global_common_send_message(info.task.msg.receiver, &msg);
            msg_free(&info.task.msg);
            info.task.type = TASK_TYPE_NONE;
            info.msg_lock = 0;
        }
    }
    else if( info.task.type == TASK_TYPE_STOP ) {
        if( info.status == STATUS_RUN ) {
            int id = -1;
            pthread_rwlock_wrlock(&ilock);
            id = get_old_channel( info.task.msg.arg_in.wolf, info.task.msg.sender);
            if( id != -1 ) {
                channel[id].status = CHANNEL_EMPTY;
            }
            pthread_rwlock_unlock(&ilock);
            finished = TASK_FINISH_SUCCESS;
        }
        else if( info.status >= STATUS_ERROR ) {
            finished = TASK_FINISH_FAIL;
        }
        else {
            finished = TASK_FINISH_SUCCESS;
        }
        if( finished ) {
            message_t msg;
            /**************************/
            msg_init(&msg);
            memcpy(&(msg.arg_pass), &(info.task.msg.arg_pass),sizeof(message_arg_t));
            msg.message = info.task.msg.message | 0x1000;
            msg.sender = msg.receiver = SERVER_VIDEO;
            msg.result = (finished == TASK_FINISH_SUCCESS) ? 0: -1;
            global_common_send_message(info.task.msg.receiver, &msg);
            msg_free(&info.task.msg);
            info.task.type = TASK_TYPE_NONE;
            info.msg_lock = 0;
        }
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
static void state_machine(void)
{
	switch( info.status ){
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
            log_goke(DEBUG_INFO,"AUDIO: switch to exit task!");
            if( info.task.msg.sender == SERVER_MANAGER) {
                info.quit_status &= (info.task.msg.arg_in.cat);
            }
            else {
                info.quit_status = 0;
            }
            char *fp = global_common_get_server_names( info.quit_status );
            log_goke(DEBUG_INFO, "AUDIO: Exit precondition =%s", fp);
            if( fp ) {
                free(fp);
            }
            info.status = EXIT_SERVER;
            break;
        case EXIT_SERVER:
            if( !info.quit_status )
                info.status = EXIT_STAGE1;
            break;
        case EXIT_STAGE1:
            server_release_1();
            info.status = EXIT_THREAD;
            break;
        case EXIT_THREAD:
            info.thread_exit = info.thread_start;
            audio_broadcast_thread_exit();
            if( !info.thread_start )
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
			log_goke(DEBUG_SERIOUS, "AUDIO !!!!!!!unprocessed server status in state machine = %d", info.status);
			break;
	}
	return;
}

static void *server_func(void)
{
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
	pthread_detach(pthread_self());
	misc_set_thread_name("server_audio");
	msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_normal, &mutex);
    info.quit_status = AUDIO_EXIT_CONDITION;
	info.init = 1;
	while( !info.exit ) {
		info.old_status = info.status;
		state_machine();
        task_proc();
		server_message_proc();
	}
	server_release_3();
	log_goke(DEBUG_INFO, "-----------thread exit: server_audio-----------");
	pthread_exit(0);
}

/*
 * internal interface
 *
 */


/*
 * external interface
 */
int server_audio_start(void)
{
	int ret=-1;
	ret = pthread_create(&info.id, NULL, server_func, NULL);
	if(ret != 0) {
		log_goke(DEBUG_SERIOUS, "audio server create error! ret = %d",ret);
		 return ret;
	 }
	else {
		log_goke(DEBUG_INFO, "audio server create successful!");
		return 0;
	}
}

int server_audio_message(message_t *msg)
{
	int ret=0;
	pthread_mutex_lock(&mutex);
	if( !message.init ) {
        log_goke(DEBUG_VERBOSE, "AUDIO server is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message) );
		pthread_mutex_unlock(&mutex);
		return -1;
	}
	ret = msg_buffer_push(&message, msg);
    log_goke(DEBUG_VERBOSE, "AUDIO message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret,message.head, message.tail);
	if( ret!=0 )
		log_goke(DEBUG_WARNING, "AUDIO push in error =%d", ret);
	else {
		pthread_cond_signal(&cond);
	}
	pthread_mutex_unlock(&mutex);
	return ret;
}
