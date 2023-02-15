#ifndef __GOKE_OSD_H__
#define __GOKE_OSD_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif
#include <sys/time.h>
#include <time.h>
#include "goke_overlay.h"

#define MEDIA_OSD_RGN_ENABLE		1
#define MEDIA_OSD_RGN_DISABLE		1
#define MEDIA_OSD_BUFFER_MAXSIZE	256

#define RGB_VALUE_BLACK     0x8000  //黑
#define RGB_VALUE_WHITE     0xffff  // 白色
#define RGB_VALUE_BG        0x7fff //透明


#define OSD_GB2312_FILE_PATH    "/app/bin/font/gb2312_16"
#define OSD_ASC_FILE_PATH       "/app/bin/font/asc16"

	
typedef enum E_MEDIA_OSD_TYPE
{
    E_MEDIA_OSD_TYPE_TIME = 0,
    E_MEDIA_OSD_TYPE_TEXT,
    E_MEDIA_OSD_TYPE_MAX,
}E_MEDIA_OSD_TYPE;


typedef enum E_MEDIA_OSDLANGUAGE_TYPE
{
    E_MEDIA_OSDLANGUAGE_EN = 0,
    E_MEDIA_OSDLANGUAGE_CHN,
    E_MEDIA_OSDLANGUAGE_MAX,
}E_MEDIA_OSDLANGUAGE_TYPE;


typedef enum E_MEDIA_OSDTIME_TYPE
{
    E_MEDIA_OSDTIME_TYPE0 = 0,
    E_MEDIA_OSDTIME_TYPE1,
    E_MEDIA_OSDTIME_TYPE2,
    E_MEDIA_OSDTIME_TYPE3,
    E_MEDIA_OSDTIME_TYPE4,
    E_MEDIA_OSDTIME_TYPE5,
    E_MEDIA_OSDTIME_TYPE6,
    E_MEDIA_OSDTIME_TYPE7,
    E_MEDIA_OSDTIME_TYPE_MAX,
}E_MEDIA_OSDTIME_TYPE;


typedef enum E_MEDIA_OSD_RGNNUM
{
    E_MEDIA_OSD_RGNNUM0 = 0,
    E_MEDIA_OSD_RGNNUM1,
    E_MEDIA_OSD_RGNNUM2,
    E_MEDIA_OSD_RGNNUM3,
    E_MEDIA_OSD_RGNNUM4,
    E_MEDIA_OSD_RGNNUM5,
    E_MEDIA_OSD_RGNNUM6,
    E_MEDIA_OSD_RGNNUM7,
    E_MEDIA_OSD_RGNNUM_MAX,
}E_MEDIA_OSD_RGNNUM;

typedef enum E_MEDIA_STREAM_CHN
{
    E_MEDIA_STREAM_CHN_MAIN = 0,
    E_MEDIA_STREAM_CHN_SUB = 1,
    E_MEDIA_STREAM_CHN_JPEG = 2,
    E_MEDIA_STREAM_CHN_MAX,
}E_MEDIA_STREAM_CHN;


typedef enum OSD_TITLE_TYPE_E
{
    TITLE_ADD_NOTHING = 0,
    TITLE_ADD_RESOLUTION = 1,
    TITLE_ADD_BITRATE = 2,
    TITLE_ADD_ALL = 3,
    TITLE_ADD_BUTT,
}OSD_TITLE_TYPE_E;

typedef enum E_MEDIA_OSD_OPERATE_TYPE
{
    E_MEDIA_OSD_OPERATE_TYPE_NULL = 0,
    E_MEDIA_OSD_OPERATE_TYPE_CREATE = 1,
    E_MEDIA_OSD_OPERATE_TYPE_DESTROY = 2,
    E_MEDIA_OSD_OPERATE_TYPE_RESET = 3,
    E_MEDIA_OSD_OPERATE_TYPE_MAX,
}E_MEDIA_OSD_OPERATE_TYPE;


typedef enum E_MEDIA_OSD_LOCATION
{
    E_MEDIA_OSD_LOCATION_LEFT_TOP = 0,
    E_MEDIA_OSD_LOCATION_LEFT_BOTTOM = 1,
    E_MEDIA_OSD_LOCATION_RIGHT_TOP = 2,
    E_MEDIA_OSD_LOCATION_RIGHT_BOTTOM = 3,
    E_MEDIA_OSD_LOCATION_HIDE = 4,
}E_MEDIA_OSD_LOCATION;

typedef struct T_Media_Osd_Item
{
	unsigned int iHandleId;//OSD handle,相当于OSD ID
	unsigned int iEnable;//该通道是否使能
	unsigned int iOsdType;//OSD 类型:时间/文本
	unsigned int iOsdWidth;//OSD分辨率宽度
	unsigned int iOsdHeight;//OSD分辨率高度	
	unsigned int iBindEncChn;//绑定的编码通道
	unsigned int iFontSize;//字体大小	
	E_MEDIA_OSD_LOCATION eLocation;//位置
	char acBuffer[MEDIA_OSD_BUFFER_MAXSIZE];//每个OSD最大的buffer大小
}T_Media_Osd_Item;

typedef struct 
{
	unsigned char main_stream_content[64];
	unsigned char sub_stream_content[64];
}goke_osd_content_s;


typedef struct T_MediaOsdCfg
{
    struct timeval current;

    E_MEDIA_OSDLANGUAGE_TYPE eOsdLanguage;//语言
    E_MEDIA_OSDTIME_TYPE eOsdTimeType;//OSD时间类型
    const char* path_hzk;
    const char* path_asc;
    int gap;
    int running;
    pthread_t pid;
	T_Media_Osd_Item tMediaOsdItem[E_MEDIA_STREAM_CHN_MAX][E_MEDIA_OSD_RGNNUM_MAX];
    goke_overlay_s param[E_MEDIA_STREAM_CHN_MAX][E_MEDIA_OSD_RGNNUM_MAX];
}T_MediaOsdCfg;




/*****************************************************************************
 函 数 名: goke_osd_init
 功能描述  : 初始化OSD模块，默认显示时间和标题
 输入参数  : 无
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int goke_osd_init();

/*****************************************************************************
 函 数 名: goke_osd_exit
 功能描述  : 去初始化OSD模块
 输入参数  : 无
 输出参数  : 无
 返 回 值: 成功返回0,失败返回小于0
*****************************************************************************/
int goke_osd_exit();
int osd_proc(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
