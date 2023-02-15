﻿
#ifndef __GOKE_OVERLAY_H__
#define __GOKE_OVERLAY_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define	RGB_VALUE_BLACK	    0x8000
#define	RGB_VALUE_WHITE		0xffff
#define	RGB_VALUE_RED		0xfc00

typedef struct
{
    int stream_id;                      /* stream id, attached encode channel */
    int handle;                         /* region handle */
    int posx;                           /* osd location x */
    int posy;                           /* osd location y */
    int width;                          /* pixel size of width */
    int height;                         /* pixel size of height */
    int factor;                         /* font size */
    int font_color;                     /*字体颜色*/
    int edge_color;                     /*边框颜色*/
	int iUsing;							/*=0,未使用 =1正在使用*/
    void* bitmap_data;                  /*address of bitmap's data*/
}goke_overlay_s;

#define INVALID_REGION_HANDLE       0xffff


/*****************************************************************************
 函 数 名: goke_overlay_create
 功能描述  : 创建视频叠加区域

 输入参数  : overlay的位置，大小等
 输出参数  :
 返 回 值: 设置成功返回0, 失败返回小于0
*****************************************************************************/
int goke_overlay_create(goke_overlay_s *param);


/*****************************************************************************
 函 数 名: goke_overlay_destory
 功能描述  : 销毁视频叠加区域

 输入参数  : overlay的位置，大小等
 输出参数  :
 返 回 值: 成功返回0, 失败返回小于0
*****************************************************************************/
int goke_overlay_destory(goke_overlay_s *param);


/*****************************************************************************
 函 数 名: goke_overlay_refresh_bitmap
 功能描述  : 刷新视频叠加区域位图

 输入参数  : overlay位图大小和数据
 输出参数  :
 返 回 值: 成功返回0, 失败返回小于0
*****************************************************************************/
int goke_overlay_refresh_bitmap(goke_overlay_s *param);





#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif

