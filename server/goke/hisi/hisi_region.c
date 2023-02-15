#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/prctl.h>
#include "../../../common/tools_interface.h"
#include "hisi.h"


HI_S32 hisi_attach_region_to_chn(int layer, int offset_x, int offset_y, MPP_CHN_S *pstMppChn)
{
    HI_S32 i;
    HI_S32 s32Ret;
    RGN_CHN_ATTR_S stChnAttr;

    /*set the chn config*/
    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = OVERLAY_RGN;

    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;

    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bQpDisable = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_TRUE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 30;

    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.u16ColorLUT[0] = 0x2abc;
    stChnAttr.unChnAttr.stOverlayChn.u16ColorLUT[1] = 0x7FF0;
    stChnAttr.unChnAttr.stOverlayChn.enAttachDest = ATTACH_JPEG_MAIN;
    /*attach to Chn*/
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = offset_x;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = offset_y;
    stChnAttr.unChnAttr.stOverlayChn.u32Layer = layer;

    s32Ret = HI_MPI_RGN_AttachToChn(layer, pstMppChn, &stChnAttr);
    if(HI_SUCCESS !=s32Ret) {
        log_goke( DEBUG_WARNING, "SAMPLE_REGION_AttachToChn failed");
    }
    return s32Ret;
}

HI_S32 hisi_detach_from_chn(HI_S32 HandleNum, MPP_CHN_S *pstMppChn)
{
    HI_S32 i;
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_MPI_RGN_DetachFromChn(HandleNum, pstMppChn);
    if(HI_SUCCESS != s32Ret){
        log_goke( DEBUG_WARNING, "SAMPLE_REGION_DetachFromChn failed! Handle:%#x",s32Ret);
    }
    return s32Ret;
}


HI_S32 hisi_create_overlay_mix(int layer, int width, int height, PIXEL_FORMAT_E pixel, int color)
{
    HI_S32 s32Ret;
    HI_S32 i;
    RGN_ATTR_S stRegion;

    stRegion.enType = OVERLAY_RGN;
    stRegion.unAttr.stOverlay.stSize.u32Height = height;
    stRegion.unAttr.stOverlay.stSize.u32Width  = width;
    stRegion.unAttr.stOverlay.u32CanvasNum = 1;

    stRegion.unAttr.stOverlay.enPixelFmt = pixel;
    stRegion.unAttr.stOverlay.u32BgColor = color;
    s32Ret = HI_MPI_RGN_Create(layer, &stRegion);
    if(HI_SUCCESS != s32Ret) {
        log_goke( DEBUG_WARNING, "HI_MPI_RGN_Create failed with %#x!", s32Ret);
        return HI_FAILURE;
    }
    return s32Ret;
}

HI_S32 hisi_destroy_region(int layer)
{
    HI_S32 s32Ret = HI_SUCCESS;
    s32Ret = HI_MPI_RGN_Destroy(layer);
    if (HI_SUCCESS != s32Ret) {
        log_goke( DEBUG_WARNING, "SAMPLE_COMM_REGION_Destroy failed");
    }
    return s32Ret;
}

HI_S32 hisi_set_region_bitmap(BITMAP_S *data, RGN_HANDLE Handle)
{
    HI_S32 s32Ret;
    s32Ret = HI_MPI_RGN_SetBitMap(Handle, data);
    if(HI_SUCCESS != s32Ret) {
        log_goke( DEBUG_WARNING, "HI_MPI_RGN_SetBitMap failed with %#x!", s32Ret);
        return HI_FAILURE;
    }
    return s32Ret;
}