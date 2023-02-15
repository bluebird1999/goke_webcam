#ifndef SERVER_GOKE_HISI_HISI_H_
#define SERVER_GOKE_HISI_HISI_H_

#include "hisi_common.h"
#include "../../video/config.h"
#include "../../audio/config.h"

//sys
HI_S32 hisi_get_pic_size(picture_size_t enPicSize, SIZE_S* pstSize);
HI_S32 hisi_init_sys(video_config_t *config);
HI_VOID hisi_uninit_sys(void);
HI_U32 hisi_get_vpss_wrap_buf_line(video_config_t *config);
HI_S32 hisi_set_vi_vpss_mode(vi_info_t *config);
HI_S32 hisi_bind_vi_vpss(VI_PIPE ViPipe, VI_CHN ViChn, VPSS_GRP VpssGrp);
HI_S32 hisi_unbind_vi_vpss(VI_PIPE ViPipe, VI_CHN ViChn, VPSS_GRP VpssGrp);
HI_S32 hisi_bind_vpss_venc(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VENC_CHN VencChn);
HI_S32 hisi_unbind_vpss_venc(VPSS_GRP VpssGrp, VPSS_CHN VpssChn, VENC_CHN VencChn);
//vi
HI_S32 hisi_init_vi(vi_info_t *config, int *info);
HI_S32 hisi_start_vi(vi_info_t *config, int *info);
HI_S32 hisi_stop_vi(vi_info_t *config, int *info);
HI_S32 hisi_uninit_vi(vi_info_t *config, int *info);
//vpss
HI_S32 hisi_init_vpss(video_config_t *config);
HI_S32 hisi_start_vpss(video_config_t *config);
HI_S32 hisi_stop_vpss(video_config_t *config);
HI_S32 hisi_uninit_vpss(video_config_t *config);
//ve
HI_S32 hisi_init_video_encoder(video_config_t *config, int stream);
HI_S32 hisi_start_video_encoder(video_config_t *config, int stream);
HI_S32 hisi_stop_video_encoder(video_config_t *config, int stream);
HI_S32 hisi_uninit_video_encoder(video_config_t *config, int stream );
HI_S32 hisi_init_snap_encoder(video_config_t *config, int stream, HI_BOOL bSupportDCF);
HI_S32 hisi_start_snap_encoder(video_config_t *config, int stream);
HI_S32 hisi_stop_snap_encoder(video_config_t *config, int stream);
HI_S32 hisi_uninit_snap_encoder(video_config_t *config, int stream );
//audio
HI_S32 hisi_init_ai(audio_config_t *config);
HI_S32 hisi_start_ai(audio_config_t *config);
HI_S32 hisi_stop_ai(audio_config_t *config);
HI_S32 hisi_uninit_ai(audio_config_t *config);
HI_S32 hisi_init_audio_codec(audio_config_t *config);
HI_S32 hisi_init_ae(audio_config_t *config);
HI_S32 hisi_start_ae(audio_config_t *config);
HI_S32 hisi_stop_ae(audio_config_t *config);
HI_S32 hisi_uninit_ae(audio_config_t *config);
HI_S32 hisi_init_ao(audio_config_t *config);
HI_S32 hisi_start_ao(audio_config_t *config);
HI_S32 hisi_stop_ao(audio_config_t *config);
HI_S32 hisi_uninit_ao(audio_config_t *config);
HI_S32 hisi_init_ad(audio_config_t *config);
HI_S32 hisi_start_ad(audio_config_t *config);
HI_S32 hisi_stop_ad(audio_config_t *config);
HI_S32 hisi_uninit_ad(audio_config_t *config);
HI_S32 hisi_bind_ae_ai(audio_config_t *config);
HI_S32 hisi_unbind_ae_ai(audio_config_t *config);
HI_S32 hisi_bind_ao_ad(audio_config_t *config);
HI_S32 hisi_unbind_ao_ad(audio_config_t *config);
int hisi_add_audio_header(unsigned char *input, int size);
//osd
HI_S32 hisi_create_overlay_mix(int layer, int height, int width, PIXEL_FORMAT_E pixel, int color);
HI_S32 hisi_destroy_region(int layer);
HI_S32 hisi_attach_region_to_chn(int layer, int offset_x, int offset_y, MPP_CHN_S *pstMppChn);
HI_S32 hisi_detach_from_chn(HI_S32 HandleNum, MPP_CHN_S *pstMppChn);
HI_S32 hisi_set_region_bitmap(BITMAP_S *data, RGN_HANDLE Handle);
//md
HI_S32 hisi_init_md(md_info_t *pstMd, HI_U32 u32Width, HI_U32 u32Height);
HI_VOID hisi_uninit_md(md_info_t *pstMd);
HI_S32 hisi_md_dma_image(VIDEO_FRAME_INFO_S *pstFrameInfo, IVE_DST_IMAGE_S *pstDst, HI_BOOL bInstant);

#endif
