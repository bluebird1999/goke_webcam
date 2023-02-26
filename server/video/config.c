/*
 * header
 */
//system header
//program header
//server header
#include "config.h"
/*
 * static
 */
video_config_t video_config = {
        //0 high, 1 low, 2 pic
        .profile.stream[ID_HIGH].enable = 1,
        .profile.stream[ID_HIGH].type = PT_H264,
        .profile.stream[ID_HIGH].size = PIC_1080P,
        .profile.stream[ID_HIGH].frame_rate = 20,
        .profile.stream[ID_HIGH].profile = 0,
        .profile.stream[ID_HIGH].rc_mode = RC_MODE_VBR,
        .profile.stream[ID_HIGH].gop_mode = VENC_GOPMODE_NORMALP,
        .profile.stream[ID_HIGH].gop = 20,
        .profile.stream[ID_HIGH].vpss_chn = 0,    //fixed
        .profile.stream[ID_HIGH].venc_chn = 0,    //fixed
        .profile.stream[ID_LOW].enable = 1,
        .profile.stream[ID_LOW].type = PT_H264,
        .profile.stream[ID_LOW].size = PIC_VGA,
        .profile.stream[ID_LOW].frame_rate = 20,
        .profile.stream[ID_LOW].profile = 0,
        .profile.stream[ID_LOW].rc_mode = RC_MODE_VBR,
        .profile.stream[ID_LOW].gop_mode = VENC_GOPMODE_NORMALP,
        .profile.stream[ID_LOW].gop = 20,
        .profile.stream[ID_LOW].vpss_chn = 1,    //fixed
        .profile.stream[ID_LOW].venc_chn = 1,    //fixed
        .profile.stream[ID_PIC].enable = 1,
        .profile.stream[ID_PIC].type = PT_JPEG,
        .profile.stream[ID_PIC].size = PIC_VGA,
        .profile.stream[ID_PIC].frame_rate = 5,
        .profile.stream[ID_PIC].profile = 0,
        .profile.stream[ID_PIC].rc_mode = RC_MODE_VBR,
        .profile.stream[ID_PIC].gop_mode = VENC_GOPMODE_NORMALP,
        .profile.stream[ID_PIC].gop = 5,
        .profile.stream[ID_PIC].vpss_chn = 0,    //dynamic
        .profile.stream[ID_PIC].venc_chn = 2,    //fixed
        .profile.stream[ID_MD].enable = 1,
        .profile.stream[ID_MD].type = PT_JPEG,
        .profile.stream[ID_MD].size = PIC_360P,
        .profile.stream[ID_MD].frame_rate = 5,
        .profile.stream[ID_MD].profile = 0,
        .profile.stream[ID_MD].rc_mode = RC_MODE_CBR,
        .profile.stream[ID_MD].gop_mode = VENC_GOPMODE_NORMALP,
        .profile.stream[ID_MD].gop = 5,
        .profile.stream[ID_MD].vpss_chn = 2,    //dynamic
        .profile.stream[ID_MD].venc_chn = -1,    //fixed
        .profile.enc_shared_buff = 1,

        .vi.sensor_info.sensor_type = GALAXYCORE_GC2053_MIPI_2M_30FPS_10BIT,
        .vi.sensor_info.sensor_id = 0,
        .vi.sensor_info.bus_id = 0,
        .vi.sensor_info.mipi_device = 0,
        .vi.sensor_info.device = "/dev/mipi",
        .vi.sensor_info.width = 1920,
        .vi.sensor_info.height = 1080,
        .vi.sensor_info.lane_divide_mode = LANE_DIVIDE_MODE_0,
        .vi.sensor_info.framerate = 20,
        .vi.sensor_info.full_line_height = 1125,
        .vi.device_info.device = 0,
        .vi.device_info.wdr_mode = WDR_MODE_NONE,

        .vi.pipe_info.pipe[0] = 0,
        .vi.pipe_info.pipe[1] = -1,
        .vi.pipe_info.mast_pipe_mode = VI_ONLINE_VPSS_ONLINE,
        .vi.pipe_info.isp_bypass = 0,
        .vi.pipe_info.multi_pipe = 0,
        .vi.pipe_info.cnumber_configured = 0,
        .vi.pipe_info.pixel_format = 0,
        .vi.pipe_info.vc_number[0] = 0,
        .vi.pipe_info.vc_number[1] = 0,

        .vi.channel_info.channel = 0,
        .vi.channel_info.pixel_format = PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        .vi.channel_info.dynamic_range = DYNAMIC_RANGE_SDR8,
        .vi.channel_info.video_format = VIDEO_FORMAT_LINEAR,
        .vi.channel_info.compress_mode = COMPRESS_MODE_NONE,

        .vi.snap_info.snap = 0,
        .vi.snap_info.double_pipe = 0,
        .vi.snap_info.video_pipe = 0,
        .vi.snap_info.snap_pipe = 0,
        .vi.snap_info.snap_pipe_mode = VI_OFFLINE_VPSS_OFFLINE,
        .vi.snap_info.video_pipe_mode = VI_OFFLINE_VPSS_OFFLINE,

        .vb.u32MaxPoolCnt = 4,

        .vpss.group  = 0,
        .vpss.buff_wrap = 0,
        .vpss.ldc = 0,
        .vpss.vi_vpss_mode = VI_ONLINE_VPSS_ONLINE,
        .vpss.group_info.enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        .vpss.group_info.enDynamicRange = DYNAMIC_RANGE_SDR8,
        .vpss.group_info.stFrameRate.s32SrcFrameRate = -1,
        .vpss.group_info.stFrameRate.s32DstFrameRate = -1,
        .vpss.group_info.bNrEn = 1,
        .vpss.group_info.stNrAttr.enCompressMode = COMPRESS_MODE_FRAME,
        .vpss.group_info.stNrAttr.enNrMotionMode = NR_MOTION_MODE_NORMAL,
        .vpss.group_info.stNrAttr.enNrType = VPSS_NR_TYPE_VIDEO,

        .vpss.channel_info[ID_HIGH].enChnMode = VPSS_CHN_MODE_USER,
        .vpss.channel_info[ID_HIGH].enVideoFormat = VIDEO_FORMAT_LINEAR,
        .vpss.channel_info[ID_HIGH].enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        .vpss.channel_info[ID_HIGH].enDynamicRange = DYNAMIC_RANGE_SDR8,
        .vpss.channel_info[ID_HIGH].enCompressMode = COMPRESS_MODE_SEG,   //different
        .vpss.channel_info[ID_HIGH].stFrameRate.s32SrcFrameRate = -1,
        .vpss.channel_info[ID_HIGH].stFrameRate.s32DstFrameRate = -1,
        .vpss.channel_info[ID_HIGH].bMirror = 0,
        .vpss.channel_info[ID_HIGH].bFlip = 0,
        .vpss.channel_info[ID_HIGH].u32Depth = 0,
        .vpss.channel_info[ID_HIGH].stAspectRatio.enMode = ASPECT_RATIO_NONE,

        .vpss.channel_info[ID_LOW].enChnMode = VPSS_CHN_MODE_USER,
        .vpss.channel_info[ID_LOW].enVideoFormat = VIDEO_FORMAT_LINEAR,
        .vpss.channel_info[ID_LOW].enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        .vpss.channel_info[ID_LOW].enDynamicRange = DYNAMIC_RANGE_SDR8,
        .vpss.channel_info[ID_LOW].enCompressMode = COMPRESS_MODE_NONE,
        .vpss.channel_info[ID_LOW].stFrameRate.s32SrcFrameRate = -1,
        .vpss.channel_info[ID_LOW].stFrameRate.s32DstFrameRate = -1,
        .vpss.channel_info[ID_LOW].bMirror = 0,
        .vpss.channel_info[ID_LOW].bFlip = 0,
        .vpss.channel_info[ID_LOW].u32Depth = 0,
        .vpss.channel_info[ID_LOW].stAspectRatio.enMode = ASPECT_RATIO_NONE,

        .vpss.channel_info[ID_PIC].enChnMode = VPSS_CHN_MODE_USER,
        .vpss.channel_info[ID_PIC].enVideoFormat = VIDEO_FORMAT_LINEAR,
        .vpss.channel_info[ID_PIC].enPixelFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        .vpss.channel_info[ID_PIC].enDynamicRange = DYNAMIC_RANGE_SDR8,
        .vpss.channel_info[ID_PIC].enCompressMode = COMPRESS_MODE_NONE,
        .vpss.channel_info[ID_PIC].stFrameRate.s32SrcFrameRate = 5,
        .vpss.channel_info[ID_PIC].stFrameRate.s32DstFrameRate = 5,
        .vpss.channel_info[ID_PIC].bMirror = 0,
        .vpss.channel_info[ID_PIC].bFlip = 0,
        .vpss.channel_info[ID_PIC].u32Depth = 1,
        .vpss.channel_info[ID_PIC].stAspectRatio.enMode = ASPECT_RATIO_NONE,

        .region.overlay[ID_HIGH].id = 0,
        .region.overlay[ID_HIGH].layer = 0,
        .region.overlay[ID_HIGH].pixel_format = PIXEL_FORMAT_ARGB_4444,
        .region.overlay[ID_HIGH].color = 0,
        .region.overlay[ID_HIGH].offset_left = 16,
        .region.overlay[ID_HIGH].offset_top = 16,
        .region.overlay[ID_HIGH].size = 32,
        .region.overlay[ID_HIGH].font_face = "FreeMono",
        .region.overlay[ID_HIGH].venc_chn = 0,

        .region.overlay[ID_LOW].id = 1,
        .region.overlay[ID_LOW].layer = 1,
        .region.overlay[ID_LOW].pixel_format = PIXEL_FORMAT_ARGB_4444,
        .region.overlay[ID_LOW].color = 0,
        .region.overlay[ID_LOW].offset_left = 16,
        .region.overlay[ID_LOW].offset_top = 16,
        .region.overlay[ID_LOW].size = 16,
        .region.overlay[ID_LOW].font_face = "FreeMono",
        .region.overlay[ID_LOW].venc_chn = 1,

        .region.overlay[ID_PIC].id = 2,
        .region.overlay[ID_PIC].layer = 2,
        .region.overlay[ID_PIC].pixel_format = PIXEL_FORMAT_ARGB_4444,
        .region.overlay[ID_PIC].color = 0,
        .region.overlay[ID_PIC].offset_left = 16,
        .region.overlay[ID_PIC].offset_top = 16,
        .region.overlay[ID_PIC].size = 16,
        .region.overlay[ID_PIC].font_face = "FreeMono",
        .region.overlay[ID_PIC].venc_chn = 2,

        .md.md_channel = 0,
        .md.sad_thresh[0] = 65535,
        .md.sad_thresh[1] = 200,
        .md.sad_thresh[2] = 150,
        .md.sad_thresh[3] = 100,
        .md.sad_thresh[4] = 70,
        .md.sad_thresh[5] = 20,

        .md.alarm_interval[0] = 10,     //low freqency in minutes
        .md.alarm_interval[1] = 3,
        .md.alarm_interval[2] = 1,

        .isp.last_mode = HISI_ISP_MODE_DAY,
        .isp.delay_time = 2000,
        .isp.last_fps = 30,
        .isp.max_fps[0] = 30,
        .isp.max_fps[1] = 30,
        .isp.bSlowShutter = 1,
        .isp.max_fps_cfg[0] = 30,
        .isp.max_fps_cfg[1] = 30,
};