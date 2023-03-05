#include "od.h"
#include "../../server/goke/hisi/hisi.h"

ivp_param_t g_ivp_param;
ivp_effect_param_t g_ivp_effect_param;

static hi_s32 ivp_set_roi(hi_s32 ivp_handle)
{
    hi_s32 ret;
    hi_ivp_roi_attr roi_attr;
    hi_ivp_roi_map roi_map;
    hi_u32 mb_size = 4;
    hi_s32 mb_range_x, mb_range_y;
    hi_s32 i, j, idx;

    roi_attr.enable = HI_TRUE;
    roi_attr.threshold = 256;
    ret = hi_ivp_set_roi_attr(ivp_handle, &roi_attr);
    if (ret != HI_SUCCESS) {
        printf("hi_ivp_set_roi_attr fail 0x%x\n",ret);
        return HI_FAILURE;
    }

    roi_map.roi_mb_mode = HI_IVP_ROI_MB_MODE_16X16;
    roi_map.img_width = 640;
    roi_map.img_height = 360;
    roi_map.mb_map = NULL;

    switch(roi_map.roi_mb_mode) {
        case HI_IVP_ROI_MB_MODE_4X4:
            mb_size = 4;
            break;
        case HI_IVP_ROI_MB_MODE_8X8:
            mb_size = 8;
            break;
        case HI_IVP_ROI_MB_MODE_16X16:
            mb_size = 16;
            break;
        default:
            return HI_FAILURE;
    }

    mb_range_x = DIV_UP(roi_map.img_width, mb_size);
    mb_range_y = DIV_UP(roi_map.img_height, mb_size);
    roi_map.mb_map = (hi_u8 *)malloc(mb_range_x * mb_range_y);
    if(roi_map.mb_map == NULL) {
        return HI_FAILURE;
    }

    /*****************************************************************************
    1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0 0 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 0
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 1
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1 1 1
    0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 1 1 1 1 1 1
    ********************************************************************************/
    for (i = 0; i < mb_range_y; i++) {
        idx = i * mb_range_x;
        for (j = 0; j < mb_range_x; j++) {
            if ((j > mb_range_x * i / mb_range_y - mb_range_x / 5) &&
                (j < mb_range_x * i / mb_range_y + mb_range_x / 5)) {
                roi_map.mb_map[idx] = 1;
            } else {
                roi_map.mb_map[idx] = 1;
            }
            idx++;
        }
    }

    if (roi_attr.enable == HI_TRUE) {
        ret = hi_ivp_set_roi_map(ivp_handle, &roi_map);
        if (ret != HI_SUCCESS) {
            printf("hi_ivp_set_roi_map fail 0x%x\n",ret);
            free(roi_map.mb_map);
            return HI_FAILURE;
        }
    }

    free(roi_map.mb_map);

    return HI_SUCCESS;
}


HI_VOID od_set_effect_param(void)
{
    /*lowbitrate para*/
    g_ivp_effect_param.low_bitrate_en = HI_TRUE;
    /*advance isp para*/
    g_ivp_effect_param.advance_isp_en = HI_TRUE;
}

HI_VOID od_set_param(ivp_param_t *ivp_param)
{
    g_ivp_param.ivp_mem_info.physical_addr = ivp_param->ivp_mem_info.physical_addr;
    g_ivp_param.ivp_mem_info.virtual_addr = ivp_param->ivp_mem_info.virtual_addr;
    g_ivp_param.ivp_mem_info.memory_size = ivp_param->ivp_mem_info.memory_size;

    g_ivp_param.ivp_handle = ivp_param->ivp_handle;
}

HI_VOID od_get_param(ivp_param_t *ivp_param)
{
    ivp_param->ivp_mem_info.physical_addr = g_ivp_param.ivp_mem_info.physical_addr;
    ivp_param->ivp_mem_info.virtual_addr = g_ivp_param.ivp_mem_info.virtual_addr;
    ivp_param->ivp_mem_info.memory_size = g_ivp_param.ivp_mem_info.memory_size;

    ivp_param->ivp_handle = g_ivp_param.ivp_handle;
}

static hi_s32 od_read_file(hi_char *file_name, hi_u8 *buf, hi_u32 size)
{
    hi_s32 ret;
    FILE *fp = NULL;

    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        log_goke( DEBUG_WARNING, "Error: open model file failed\n");
        return HI_FAILURE;
    }

    ret = fread(buf, size, 1, fp);
    if (ret == -1) {
        log_goke( DEBUG_WARNING, "Error: fread failed\n");
        fclose(fp);
        return HI_FAILURE;
    }

    fclose(fp);

    return HI_SUCCESS;
}

static hi_s32 od_load_resource(hi_s32 *ivp_handle)
{
    hi_s32 ret;
    hi_char resource_name[256] = IVP_RESOURCE_ALLDAY;
    hi_u32 file_size;
    hi_ivp_mem_info ivp_mem_info;
    ivp_param_t ivp_param;

    file_size = misc_get_file_size(resource_name);
    if (file_size == -1) {
        log_goke( DEBUG_WARNING, "SAMPLE_IVP_GetFileSize failed with %#x!\n", ret);
        return ret;
    }

    ret = HI_MPI_SYS_MmzAlloc(&ivp_mem_info.physical_addr, (HI_VOID**)&ivp_mem_info.virtual_addr,
                              "RESOURCE_FILE_MEM", NULL, file_size);
    if(HI_SUCCESS != ret) {
        log_goke( DEBUG_WARNING, "HI_MPI_SYS_MmzAlloc failed with %#x!\n", ret);
        return ret;
    }
    ivp_mem_info.memory_size = file_size;

    ret = od_read_file(resource_name, (HI_U8 *)(HI_SL)ivp_mem_info.virtual_addr, file_size);
    if (HI_SUCCESS != ret) {
        log_goke( DEBUG_WARNING, "SAMPLE_IVP_ReadFile failed with %#x!\n", ret);
        goto MEM_FREE_EXIT;
    }

    ret = hi_ivp_load_resource_from_memory(&ivp_mem_info, ivp_handle);
    if (HI_SUCCESS != ret) {
        log_goke( DEBUG_WARNING, "hi_ivp_load_resource_from_memory failed with %#x!\n", ret);
        goto MEM_FREE_EXIT;
    }

    ivp_param.ivp_handle = *ivp_handle;
    ivp_param.ivp_mem_info.physical_addr = ivp_mem_info.physical_addr;
    ivp_param.ivp_mem_info.virtual_addr = ivp_mem_info.virtual_addr;
    ivp_param.ivp_mem_info.memory_size = ivp_mem_info.memory_size;
    od_set_param(&ivp_param);

    return HI_SUCCESS;

    MEM_FREE_EXIT:
    HI_MPI_SYS_MmzFree(ivp_mem_info.physical_addr, (HI_VOID*)(HI_SL)ivp_mem_info.virtual_addr);
    return ret;
}

static hi_s32 od_set_optional_attr(hi_s32 ivp_handle)
{
    hi_s32 ret;
    hi_ivp_ctrl_attr ivp_pd_ctrl_attr;
    hi_ivp_ctrl_attr ivp_fd_ctrl_attr;
    hi_u32 chip_id = 0;

    /* set pd threshold */
    ivp_pd_ctrl_attr.threshold = video_config.od.thresh_value;
    ret = hi_ivp_set_ctrl_attr(ivp_handle, &ivp_pd_ctrl_attr);
    if (ret != HI_SUCCESS) {
        log_goke( DEBUG_WARNING, "hi_ivp_set_ctrl_attr failed with %#x!\n", ret);
        return ret;
    }

//    ret = ivp_set_venc_low_bitrate(ivp_handle);
//    if (ret != HI_SUCCESS) {
//        log_goke( DEBUG_WARNING, "ivp_set_venc_low_bitrate failed with %#x!\n", ret);
//        return ret;
//    }

//    ret = ivp_set_advance_isp(ivp_handle, video_config.vi.pipe_info.pipe[0], 1);
//    if (ret != HI_SUCCESS) {
//        log_goke( DEBUG_WARNING, "ivp_set_advance_isp failed with %#x!\n", ret);
//        return ret;
//    }

    ret = ivp_set_roi(ivp_handle);
    if (ret != HI_SUCCESS) {
        log_goke( DEBUG_WARNING, "ivp_set_roi failed!\n");
        return ret;
    }

    return HI_SUCCESS;
}

HI_S32 od_init(void)
{
    hi_s32 ret;
    hi_s32 ivp_handle = -1;

    od_set_effect_param();

    ret = hi_ivp_init();
    if (HI_SUCCESS != ret) {
        log_goke( DEBUG_WARNING, "hi_ivp_init failed with %#x!\n", ret);
        return HI_FAILURE;
    }

    ret = od_load_resource(&ivp_handle);
    if (ret != HI_SUCCESS) {
        log_goke( DEBUG_WARNING, "ivp_load_resource failed with %#x!\n", ret);
        goto IVP_DEINIT_EXIT;
    }

    ret = od_set_optional_attr(ivp_handle);
    if (ret != HI_SUCCESS) {
        log_goke( DEBUG_WARNING, "ivp_set_optional_attr failed with %#x!\n", ret);
        goto UNLOAD_RES;
    }
    return HI_SUCCESS;

    UNLOAD_RES:
    hi_ivp_unload_resource(ivp_handle);
    IVP_DEINIT_EXIT:
    hi_ivp_deinit();
    return ret;
}

HI_S32 od_uninit(void)
{
    ivp_param_t ivp_param;
    od_get_param(&ivp_param);
    HI_MPI_SYS_MmzFree(ivp_param.ivp_mem_info.physical_addr, (HI_VOID*)(HI_SL)ivp_param.ivp_mem_info.virtual_addr);
    hi_ivp_unload_resource(ivp_param.ivp_handle);
    hi_ivp_deinit();
    return HI_SUCCESS;
}