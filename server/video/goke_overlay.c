#include "hi_common.h"
#include "hi_comm_region.h"
#include "hi_comm_video.h"
#include "goke_overlay.h"
#define OVERLAY_CHECK(exp, ret, fmt...) do\
{\
    if (!(exp))\
    {\
        printf(fmt);\
        return ret;\
    }\
}while(0)
	
int goke_overlay_create(goke_overlay_s *param)
{
    OVERLAY_CHECK(param, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, param);
    OVERLAY_CHECK(param->handle != INVALID_REGION_HANDLE, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, param->handle);

    int ret;
    RGN_ATTR_S stRgnAttr;
    stRgnAttr.enType = OVERLAY_RGN;
    stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_ARGB_1555;//PIXEL_FORMAT_ARGB_1555;//PIXEL_FORMAT_RGB_1555;
    stRgnAttr.unAttr.stOverlay.u32BgColor = 0x7c00;
    stRgnAttr.unAttr.stOverlay.stSize.u32Width  = param->width;
    stRgnAttr.unAttr.stOverlay.stSize.u32Height = param->height;
	stRgnAttr.unAttr.stOverlay.u32CanvasNum = 5;
	
    ret = HI_MPI_RGN_Create(param->handle, &stRgnAttr);
    OVERLAY_CHECK(ret == HI_SUCCESS, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, ret);

    MPP_CHN_S stChn;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = param->stream_id;
	param->iUsing = 1;
    RGN_CHN_ATTR_S stChnAttr;
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.bShow = HI_TRUE;
    stChnAttr.enType = OVERLAY_RGN;
    stChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;//值越小越透明
    stChnAttr.unChnAttr.stOverlayChn.u32Layer = 0;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp  = 0;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.bInvColEn = HI_FALSE;
    stChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 0;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.enChgMod = LESSTHAN_LUM_THRESH;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.u32LumThresh = 128;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = param->posx;
    stChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = param->posy;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Width = 16 * param->factor;
    stChnAttr.unChnAttr.stOverlayChn.stInvertColor.stInvColArea.u32Height = 16 * param->factor;
    ret = HI_MPI_RGN_AttachToChn(param->handle, &stChn, &stChnAttr);
    OVERLAY_CHECK(ret == HI_SUCCESS, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, ret);
    param->bitmap_data = malloc(param->width * param->height * 2);
	OVERLAY_CHECK(param->bitmap_data, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, param->bitmap_data);

    return HI_SUCCESS;
}

int goke_overlay_destory(goke_overlay_s *param)
{
    OVERLAY_CHECK(param, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, param);
    OVERLAY_CHECK(INVALID_REGION_HANDLE != param->handle, -1,"%d:invalid handle %#x.\n",__LINE__, param->handle);

    int ret = -1;
    MPP_CHN_S stChn;
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = param->stream_id;
    ret = HI_MPI_RGN_DetachFromChn(param->handle, &stChn);
    OVERLAY_CHECK(ret == HI_SUCCESS, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, ret);

    ret = HI_MPI_RGN_Destroy(param->handle);
    OVERLAY_CHECK(ret == HI_SUCCESS, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, ret);

    param->handle = INVALID_REGION_HANDLE;
    if (param->bitmap_data)
    {
        free(param->bitmap_data);
        param->bitmap_data = NULL;
    }
	param->iUsing = 0;
    return 0;
}

int goke_overlay_refresh_bitmap(goke_overlay_s *param)
{
    OVERLAY_CHECK(param, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, param);
    OVERLAY_CHECK(INVALID_REGION_HANDLE != param->handle, -1,"%d:invalid handle %#x.\n",__LINE__, param->handle);
    int ret = -1;
    BITMAP_S bitmap;
    bitmap.enPixelFormat = PIXEL_FORMAT_ARGB_1555;//PIXEL_FORMAT_RGB_1555;
    bitmap.u32Width = param->width;
    bitmap.u32Height = param->height;
    bitmap.pData = param->bitmap_data;
    
#if 0
	printf("\nhandle = %d\n",param->handle);
	printf("channid = %d\n",param->stream_id);
	printf("posx	= %d\n",param->posx);
	printf("posy	= %d\n",param->posy);
	printf("width	 = %d\n",param->width);
	printf("height	  = %d\n",param->height);
	printf("factor	  = %d\n",param->factor);
	printf("font_color= %d\n",param->font_color);
	printf("edge_color= %d\n",param->edge_color);
	printf("pbitmap_data=%p\n\n",param->bitmap_data);
		
#endif
    ret = HI_MPI_RGN_SetBitMap(param->handle, &bitmap);
    OVERLAY_CHECK(ret == HI_SUCCESS, HI_FAILURE, "%d:Error with %#x.\n",__LINE__, ret);
    return ret;
}

