#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include "../../../common/tools_interface.h"
#include "../../../server/manager/manager_interface.h"
#include "../../audio/config.h"
#include "hisi_common.h"
#include "adp/audio_dl_adp.h"
#include "adp/audio_aac_adp.h"

int hisi_add_audio_header(unsigned char *input, int size) {
    input[0] = 0x00;
    input[1] = 0x01;
    input[2] = size / 2;
    input[3] = 0x00;
    return 4;
}

HI_S32 hisi_bind_audio(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn) {
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

HI_S32 hisi_unbind_audio(AUDIO_DEV AiDev, AI_CHN AiChn, AENC_CHN AeChn) {
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = AiDev;
    stSrcChn.s32ChnId = AiChn;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = AeChn;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

HI_S32 hisi_inner_audio_codec(audio_config_t *config) {
    HI_S32 fdAcodec = -1;
    HI_S32 ret = HI_SUCCESS;
    ACODEC_FS_E i2s_fs_sel = 0;
    int iAcodecInputVol = 0;
    ACODEC_MIXER_E input_mode = 0;

    fdAcodec = open(config->codec_file, O_RDWR);
    if (fdAcodec < 0) {
        log_goke(DEBUG_WARNING, "can't open Acodec,%s", config->codec_file);
        return HI_FAILURE;
    }
    if (ioctl(fdAcodec, ACODEC_SOFT_RESET_CTRL)) {
        log_goke(DEBUG_WARNING, "Reset audio codec error");
    }

    switch (config->aio_attr.enSamplerate) {
        case AUDIO_SAMPLE_RATE_8000:
            i2s_fs_sel = ACODEC_FS_8000;
            break;
        case AUDIO_SAMPLE_RATE_16000:
            i2s_fs_sel = ACODEC_FS_16000;
            break;
        case AUDIO_SAMPLE_RATE_32000:
            i2s_fs_sel = ACODEC_FS_32000;
            break;
        case AUDIO_SAMPLE_RATE_11025:
            i2s_fs_sel = ACODEC_FS_11025;
            break;
        case AUDIO_SAMPLE_RATE_22050:
            i2s_fs_sel = ACODEC_FS_22050;
            break;
        case AUDIO_SAMPLE_RATE_44100:
            i2s_fs_sel = ACODEC_FS_44100;
            break;
        case AUDIO_SAMPLE_RATE_12000:
            i2s_fs_sel = ACODEC_FS_12000;
            break;
        case AUDIO_SAMPLE_RATE_24000:
            i2s_fs_sel = ACODEC_FS_24000;
            break;
        case AUDIO_SAMPLE_RATE_48000:
            i2s_fs_sel = ACODEC_FS_48000;
            break;
        case AUDIO_SAMPLE_RATE_64000:
            i2s_fs_sel = ACODEC_FS_64000;
            break;
        case AUDIO_SAMPLE_RATE_96000:
            i2s_fs_sel = ACODEC_FS_96000;
            break;
        default:
            log_goke(DEBUG_WARNING, "not support enSample:%d", config->aio_attr.enSamplerate);
            ret = HI_FAILURE;
            break;
    }

    if (ioctl(fdAcodec, ACODEC_SET_I2S1_FS, &i2s_fs_sel)) {
        log_goke(DEBUG_WARNING, "set acodec sample rate failed");
        ret = HI_FAILURE;
    }
    input_mode = ACODEC_MIXER_IN1;
    if (ioctl(fdAcodec, ACODEC_SET_MIXER_MIC, &input_mode)) {
        log_goke(DEBUG_WARNING, ": select acodec input_mode failed");
        ret = HI_FAILURE;
    }

    /******************************************************************************************
    The input volume range is [-78, +80]. Both the analog gain and digital gain are adjusted.
    A larger value indicates higher volume.
    For example, the value 80 indicates the maximum volume of 80 dB,
    and the value -78 indicates the minimum volume (muted status).
    The volume adjustment takes effect simultaneously in the audio-left and audio-right channels.
    The recommended volume range is [+19, +50].
    Within this range, the noises are lowest because only the analog gain is adjusted,
    and the voice quality can be guaranteed.
    *******************************************************************************************/
    iAcodecInputVol = config->capture_volume;
    if (ioctl(fdAcodec, ACODEC_SET_INPUT_VOL, &iAcodecInputVol)) {
        log_goke(DEBUG_WARNING, ": set acodec micin volume failed");
        return HI_FAILURE;
    }

    close(fdAcodec);
    return ret;
}

HI_S32 hisi_init_audio_codec(audio_config_t *config) {
    HI_S32 ret = HI_SUCCESS;

    ret = hisi_inner_audio_codec(config);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ":SAMPLE_INNER_CODEC_CfgAudio failed");
        return ret;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_init_ai(audio_config_t *config) {
    HI_S32 i;
    HI_S32 ret;

    ret = HI_MPI_AI_SetPubAttr(config->ai_dev, &config->aio_attr);
    if (ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AI_SetPubAttr(%d) failed with %#x", config->ai_dev, ret);
        return ret;
    }
    ret = HI_MPI_AI_Enable(config->ai_dev);
    if (ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AI_Enable(%d) failed with %#x", config->ai_dev, ret);
        return ret;
    }

    ret = HI_MPI_AI_EnableChn(config->ai_dev, config->ai_channel);
    if (ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AI_EnableChn(%d,%d) failed with %#x", config->ai_dev, i, ret);
        return ret;
    }
/*
    ret = HI_MPI_AI_SetTalkVqeAttr(config->ai_dev, config->ai_channel, config->ao_dev, config->ao_channel,
                             (AI_TALKVQE_CONFIG_S *)&config->vqe_attr);
    if (ret) {
        log_goke(DEBUG_WARNING,": HI_MPI_AI_SetTalkVqeAttr(%d,%d) failed with %#x", config->ai_dev, i, ret);
        return ret;
    }
    ret = HI_MPI_AI_EnableVqe(config->ai_dev, config->ai_channel);
    if (ret) {
        log_goke(DEBUG_WARNING,": HI_MPI_AI_EnableVqe(%d,%d) failed with %#x", config->ai_dev, config->ai_channel, ret);
        return ret;
    }
    */
    return HI_SUCCESS;
}

HI_S32 hisi_start_ai(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_ai(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_ai(audio_config_t *config) {
    HI_S32 ret;

    ret = HI_MPI_AI_DisableVqe(config->ai_dev, config->ai_channel);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "[Func]:%s [Line]:%d [Info]:%s", __LINE__, "failed");
        return ret;
    }

    ret = HI_MPI_AI_DisableChn(config->ai_dev, config->ai_channel);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "[Func]:%s [Line]:%d [Info]:%s", __LINE__, "failed");
        return ret;
    }

    ret = HI_MPI_AI_Disable(config->ai_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "[Func]:%s [Line]:%d [Info]:%s", __LINE__, "failed");
        return ret;
    }

    return HI_SUCCESS;
}

HI_S32 hisi_init_ae(audio_config_t *config) {
    HI_S32 ret;
    AENC_CHN_ATTR_S stAencAttr;
    AENC_ATTR_AAC_S stAencAac;
    AENC_ATTR_G711_S stAencG711;
    AENC_ATTR_LPCM_S stAencLpcm;

    stAencAttr.enType = config->payload_type;
    stAencAttr.u32BufSize = 30;
    stAencAttr.u32PtNumPerFrm = config->aio_attr.u32PtNumPerFrm;

    if (PT_AAC == stAencAttr.enType) {
        stAencAttr.pValue = &stAencAac;
        stAencAac.enAACType = AAC_TYPE_AACLC;
        stAencAac.enBitRate = AAC_BPS_128K;
        stAencAac.enBitWidth = AUDIO_BIT_WIDTH_16;
        stAencAac.enSmpRate = config->aio_attr.enSamplerate;
        stAencAac.enSoundMode = config->aio_attr.enSoundmode;
        stAencAac.enTransType = AAC_TRANS_TYPE_ADTS;
        stAencAac.s16BandWidth = 0;
    } else if (PT_G711A == stAencAttr.enType || PT_G711U == stAencAttr.enType) {
        stAencAttr.pValue = &stAencG711;
    } else if (PT_LPCM == stAencAttr.enType) {
        stAencAttr.pValue = &stAencLpcm;
    }else {
        log_goke(DEBUG_WARNING, "invalid aenc payload type:%d", stAencAttr.enType);
        return HI_FAILURE;
    }
    /* create aenc chn*/
    ret = HI_MPI_AENC_CreateChn(config->ae_channel, &stAencAttr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MPI_AENC_CreateChn(%d) failed with %#x!",
                 config->ae_channel, ret);
        return ret;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_start_ae(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_ae(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_ae(audio_config_t *config) {
    HI_S32 ret;

    ret = HI_MPI_AENC_DestroyChn(config->ae_channel);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MPI_AENC_DestroyChn(%d) failed with %#x!",
                 config->ae_channel, ret);
        return ret;
    }

    return HI_SUCCESS;
}

HI_S32 hisi_init_ao(audio_config_t *config) {
    HI_S32 i;
    HI_S32 ret;
	HI_S32 iVolume;
    ret = HI_MPI_AO_SetPubAttr(config->ao_dev, &config->aio_attr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "HI_MPI_AO_SetPubAttr(%d) failed with %#x!", \
               config->ao_dev, ret);
        return HI_FAILURE;
    }

    ret = HI_MPI_AO_Enable(config->ao_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AO_Enable(%d) failed with %#x!", config->ao_dev, ret);
        return HI_FAILURE;
    }

    ret = HI_MPI_AO_EnableChn(config->ao_dev, config->ao_channel);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AO_EnableChn(%d) failed with %#x!", i, ret);
        return HI_FAILURE;
    }

    ret = HI_MPI_AO_EnableChn(config->ao_dev, AO_SYSCHN_CHNID);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AO_EnableChn(%d) failed with %#x!", i, ret);
        return HI_FAILURE;
    }
	ret = HI_MPI_AO_GetVolume(config->ao_channel,&iVolume);

	iVolume = 10;
	ret = HI_MPI_AO_SetVolume(config->ao_channel,iVolume);

    return HI_SUCCESS;
}

HI_S32 hisi_start_ao(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_ao(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_ao(audio_config_t *config) {
    HI_S32 i;
    HI_S32 ret;

    ret = HI_MPI_AO_DisableChn(config->ao_dev, config->ao_channel);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AO_DisableChn failed with %#x!", ret);
        return ret;
    }


    ret = HI_MPI_AO_Disable(config->ao_dev);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_AO_Disable failed with %#x!", ret);
        return ret;
    }

    return HI_SUCCESS;
}

HI_S32 hisi_init_ad(audio_config_t *config) {
    HI_S32 ret;
    ADEC_CHN_ATTR_S stAdecAttr;
    ADEC_ATTR_G711_S stAdecG711;
    ADEC_ATTR_AAC_S stAdecAac;
    ADEC_ATTR_LPCM_S stAdecLpcm;

    stAdecAttr.enType = config->payload_type;
    stAdecAttr.u32BufSize = 20;
    stAdecAttr.enMode = ADEC_MODE_STREAM;/* propose use pack mode in your app */
    if (PT_AAC == stAdecAttr.enType) {
        stAdecAttr.pValue = &stAdecAac;
        stAdecAttr.enMode = ADEC_MODE_STREAM;   /* aac should be stream mode */
        stAdecAac.enTransType = AAC_TRANS_TYPE_ADTS;
    } else if (PT_G711A == stAdecAttr.enType || PT_G711U == stAdecAttr.enType) {
        stAdecAttr.pValue = &stAdecG711;
    } else if (PT_LPCM == stAdecAttr.enType) {
        stAdecAttr.pValue = &stAdecLpcm;
    }else {
        log_goke(DEBUG_WARNING, ": invalid aenc payload type:%d", stAdecAttr.enType);
        return HI_FAILURE;
    }
    /* create adec chn*/
    ret = HI_MPI_ADEC_CreateChn(config->ad_channel, &stAdecAttr);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_ADEC_CreateChn(%d) failed with %#x!", \
               config->ad_channel, ret);
        return ret;
    }
    return 0;
}

HI_S32 hisi_start_ad(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_stop_ad(audio_config_t *config) {
    return HI_SUCCESS;
}

HI_S32 hisi_uninit_ad(audio_config_t *config) {
    HI_S32 ret;

    ret = HI_MPI_ADEC_DestroyChn(config->ad_channel);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, ": HI_MPI_ADEC_DestroyChn(%d) failed with %#x!",
                 config->ad_channel, ret);
        return ret;
    }
    return HI_SUCCESS;
}

HI_S32 hisi_bind_ae_ai(audio_config_t *config) {
    int i, j, ret;
    int channel_number;
    int AeChn, AiChn;
    channel_number = config->aio_attr.u32ChnCnt >> config->aio_attr.enSoundmode;
    for (i = 0; i < channel_number; i++) {
        AeChn = i;
        AiChn = i;
        ret = hisi_bind_audio(config->ai_dev, i, i);
        if (ret != HI_SUCCESS) {
            for (j = 0; j < i; j++) {
                hisi_unbind_audio(config->ai_dev, j, j);
            }
            return HI_FAILURE;
        }
        log_goke(DEBUG_WARNING, "Ai(%d,%d) bind to AencChn:%d ok!", config->ai_dev, AiChn, AeChn);
    }

    return HI_SUCCESS;
}

HI_S32 hisi_unbind_ae_ai(audio_config_t *config) {
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_AI;
    stSrcChn.s32DevId = config->ai_dev;
    stSrcChn.s32ChnId = config->ai_channel;
    stDestChn.enModId = HI_ID_AENC;
    stDestChn.s32DevId = 0;
    stDestChn.s32ChnId = config->ae_channel;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}

HI_S32 hisi_bind_ao_ad(audio_config_t *config) {
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = config->ad_channel;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = config->ao_dev;
    stDestChn.s32ChnId = config->ao_channel;

    return HI_MPI_SYS_Bind(&stSrcChn, &stDestChn);
}

HI_S32 hisi_unbind_ao_ad(audio_config_t *config) {
    MPP_CHN_S stSrcChn, stDestChn;

    stSrcChn.enModId = HI_ID_ADEC;
    stSrcChn.s32ChnId = config->ad_channel;
    stSrcChn.s32DevId = 0;
    stDestChn.enModId = HI_ID_AO;
    stDestChn.s32DevId = config->ao_dev;
    stDestChn.s32ChnId = config->ao_channel;

    return HI_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
}