#include "config.h"

audio_config_t audio_config = {
        .ai_dev = 0,
        .ao_dev = 0,
        .ai_channel = 0,
        .ao_channel = 0,
        .ae_channel = 0,
        .ad_channel = 0,
        //ai
        .aio_attr.enSamplerate = AUDIO_SAMPLE_RATE_8000,
        .aio_attr.enBitwidth = AUDIO_BIT_WIDTH_16,
        .aio_attr.enWorkmode = AIO_MODE_I2S_MASTER,
        .aio_attr.enSoundmode = AUDIO_SOUND_MODE_MONO,
        .aio_attr.u32EXFlag = 1,
        .aio_attr.u32FrmNum = 32,           //buffer
        .aio_attr.u32PtNumPerFrm = 480,
        .aio_attr.u32ChnCnt = 1,
        .aio_attr.u32ClkSel = 0,
        .aio_attr.enI2sType = AIO_I2STYPE_INNERCODEC,
        //
        .vqe_attr.u32OpenMask = AI_TALKVQE_MASK_HPF | AI_TALKVQE_MASK_ANR,
        .vqe_attr.s32WorkSampleRate    = AUDIO_SAMPLE_RATE_8000,
        .vqe_attr.s32FrameSample       = 480,
        .vqe_attr.enWorkstate          = VQE_WORKSTATE_COMMON,
        .vqe_attr.stAgcCfg.bUsrMode    = HI_FALSE,
        .vqe_attr.stAecCfg.bUsrMode    = HI_FALSE,
        .vqe_attr.stAnrCfg.bUsrMode    = HI_FALSE,
        .vqe_attr.stHpfCfg.bUsrMode    = HI_TRUE,
        .vqe_attr.stHpfCfg.enHpfFreq   = AUDIO_HPF_FREQ_150,
        //other
        .capture_volume = 80,
        .payload_type = PT_G711A,
        .codec_file = "/dev/acodec",
};