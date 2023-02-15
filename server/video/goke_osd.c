#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/prctl.h>
#include "goke_osd.h"
#include <osd_bitmap.h>
#include "hi_comm_video.h"
#include "cJSON.h"
#include <utf8_to_gbk.h>
#include "../../global/global_interface.h"

#define OSD_CHECK(exp, ret, fmt...) do\
{\
    if (!(exp))\
    {\
        printf(fmt);\
        return ret;\
    }\
}while(0)


goke_osd_content_s g_osd_content;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

T_MediaOsdCfg* g_tMediaOsdCfg = NULL;
bitmap_s bitmap;
static const char *week_day_ch[8]=
{
    "\xD0\xC7\xC6\xDA\xC8\xD5",
    "\xD0\xC7\xC6\xDA\xD2\xBB",
    "\xD0\xC7\xC6\xDA\xB6\xFE",
    "\xD0\xC7\xC6\xDA\xC8\xFD",
    "\xD0\xC7\xC6\xDA\xCB\xC4",
    "\xD0\xC7\xC6\xDA\xCE\xE5",
    "\xD0\xC7\xC6\xDA\xC1\xF9",
    "",
};

static const char *week_day_en[8]=
{
    "   Sunday",
    "   Monday",
    "  Tuesday",
    "Wednesday",
    " Thursday",
    "   Friday",
    " Saturday",
    "",
};

/*
static const char *fmt[]=
{
    "yyyy-mm-dd hh:mm:ss",
    "yyyy/mm/dd hh:mm:ss",
    "yy-mm-dd hh:mm:ss",
    "yy/mm/dd hh:mm:ss",
    "hh:mm:ss dd/mm/yyyy",
    "hh:mm:ss dd-mm-yyyy",
    "hh:mm:ss mm/dd/yyyy",
    "hh:mm:ss mm-dd-yyyy",
    NULL
};
*/

static int util_time_clock(clockid_t clk_id, struct timeval* pTime)
{
    struct timespec stTime;
    memset(&stTime, 0, sizeof(struct timespec));
    memset(pTime, 0, sizeof(struct timeval));
    if (!clock_gettime(clk_id, &stTime))
    {
        pTime->tv_sec = stTime.tv_sec;
        pTime->tv_usec = stTime.tv_nsec/1000;
        return 0;
    }

    return -1;
}

int util_time_abs(struct timeval* pTime)
{
    OSD_CHECK(NULL != pTime, -1, "%d:invalid parameter.\n",__LINE__);

    return util_time_clock(CLOCK_MONOTONIC, pTime);
}

int util_time_local(struct timeval* pTime)
{
    OSD_CHECK(NULL != pTime, -1, "%d:invalid parameter.\n",__LINE__);

    return util_time_clock(CLOCK_REALTIME, pTime);
}


static const char *osd_get_weekday(int index)
{
    const char* ret = NULL;
    if (g_tMediaOsdCfg && g_tMediaOsdCfg->eOsdLanguage)
    {
        ret = week_day_en[index];
    }
    else
    {
        ret = week_day_ch[index];
    }

    return ret;
}

static int osd_time_string(E_MEDIA_OSDTIME_TYPE type, char* output)
{
    struct timeval tv;
    struct tm *ptm;

    util_time_local(&tv);
    ptm = localtime(&tv.tv_sec);

    switch(type)
    {
        case E_MEDIA_OSDTIME_TYPE0:
            sprintf(output, "%04d-%02d-%02d %02d:%02d:%02d",
                (1900 + ptm->tm_year),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec
                /*osd_get_weekday(ptm->tm_wday)*/);
            break;
        case E_MEDIA_OSDTIME_TYPE1:
            sprintf(output, "%04d/%02d/%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
            break;
        case E_MEDIA_OSDTIME_TYPE2:
            sprintf(output, "%02d-%02d-%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year - 2000),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
            break;
        case E_MEDIA_OSDTIME_TYPE3:
            sprintf(output, "%02d/%02d/%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year - 2000),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
            break;
        case E_MEDIA_OSDTIME_TYPE4:
            sprintf(output, "%02d:%02d:%02d %02d/%02d/%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                ptm->tm_mday,
                (1 + ptm->tm_mon),
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;
        case E_MEDIA_OSDTIME_TYPE5:
            sprintf(output, "%02d:%02d:%02d %02d-%02d-%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                ptm->tm_mday,
                (1 + ptm->tm_mon),
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;

        case E_MEDIA_OSDTIME_TYPE6:
            sprintf(output, "%02d:%02d:%02d %02d/%02d/%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;
        case E_MEDIA_OSDTIME_TYPE7:
            sprintf(output, "%02d:%02d:%02d %02d-%02d-%04d %s",
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                (1900 + ptm->tm_year),
                osd_get_weekday(ptm->tm_wday));
            break;
           
        default:
            sprintf(output, "%04d-%02d-%02d %s %02d:%02d:%02d",
                (1900 + ptm->tm_year),
                (1 + ptm->tm_mon),
                ptm->tm_mday,
                osd_get_weekday(ptm->tm_wday),
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec);
    }

    return 0;
}

static int osd_char2short(char* src, int row, int col, unsigned short* dst)
{
    int i = 0;
    int j = 0;

    for (i = 0; i < row; i++)
    {
        for (j = 0; j < col; j++)
        {
            int idx = i*col+j;
            char ch = src[idx];
            if (ch == FORE_GROUND)
            {
                dst[idx] = RGB_VALUE_WHITE;
            }
            else if (ch == BACK_GROUND)
            {
                dst[idx] = RGB_VALUE_BG;
            }
            else if (ch == FONT_EDGE)
            {
                dst[idx] = RGB_VALUE_BLACK;
            }
        }
    }

    return 0;
}

static int osd_get_factor(int width, int height)
{
    int factor = 1;

    if(width*height >= 2048*1536)
        factor = 4;
    else if(width*height >= 1920*1080)
        factor = 3;
    else if(width*height >= 1280*720)
        factor = 2;

    return factor;
}

#if 0
int osd_get_content()
{
	 cJSON *Object = cJSON_ReadFile(ZKU_CFGFILE_VIDEO_OSD);
	 cJSON *Object_next = NULL;
	 cJSON *Object_next_content = NULL;
	 char unicode[64];
	 if(Object)
	 {
		Object_next = cJSON_GetObjectItem(Object,"main");
		if(Object_next)
		{
			Object_next_content = cJSON_GetObjectItem(Object_next,"text");
			if(Object_next_content)
			{
				//printf("main text:%s,len:%d\n",Object_next_content->valuestring,strlen(Object_next_content->valuestring));
				memset(g_osd_content.main_stream_content,0,strlen(g_osd_content.main_stream_content));
				utf8_t_gbk(Object_next_content->valuestring,strlen(Object_next_content->valuestring),g_osd_content.main_stream_content);
			}
		}
		Object_next = cJSON_GetObjectItem(Object,"sub");
		if(Object_next)
		{
			Object_next_content = cJSON_GetObjectItem(Object_next,"text");
			if(Object_next_content)
			{
				//printf("sub text:%s,len:%d\n",Object_next_content->valuestring,strlen(Object_next_content->valuestring));
				memset(g_osd_content.sub_stream_content,0,strlen(g_osd_content.sub_stream_content));
				utf8_t_gbk(Object_next_content->valuestring,strlen(Object_next_content->valuestring),g_osd_content.sub_stream_content);
			}
		}
		cJSON_Delete(Object);
	 }
	 return 0;
}
#endif

static int OsdUpdateContent(int iChn, int iRgnNum,E_MEDIA_OSD_OPERATE_TYPE *eType)
{
    int iRet = 0,iLen = 0;
	//ZKU_CONFIG_OSD_T tOsdCfg;
	//ZkuConfig_GetOsd(&tOsdCfg);
	switch (iRgnNum)
	{
		case E_MEDIA_OSD_TYPE_TIME:
		{
			memset(g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].acBuffer, 0, sizeof(g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].acBuffer));
        	iRet = osd_time_string(g_tMediaOsdCfg->eOsdTimeType, g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].acBuffer);
        	return iRet;
		}
		break;
		case E_MEDIA_OSD_TYPE_TEXT:
		#if 0
		{
			if(g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].iEnable == 0)
			{
				*eType = E_MEDIA_OSD_OPERATE_TYPE_DESTROY;
				return 0;
			}
			static char acTextLast[E_MEDIA_STREAM_CHN_MAX][MEDIA_OSD_BUFFER_MAXSIZE];
			char acText[MEDIA_OSD_BUFFER_MAXSIZE];
			memset(acText,0,sizeof(acText));
	        memset(g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].acBuffer, 0, sizeof(g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].acBuffer));
			g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].iEnable = 0;
			if(iChn == E_MEDIA_STREAM_CHN_MAIN)
			{
				iLen = strlen(tOsdCfg.tMainStream.acText);
				if(iLen > 0)
				{
					utf8_t_gbk(tOsdCfg.tMainStream.acText,iLen,acText);
				}
				else
				{
					*eType = E_MEDIA_OSD_OPERATE_TYPE_DESTROY;
					return 0;
				}
			}
			else if(iChn == E_MEDIA_STREAM_CHN_SUB)
			{
				iLen = strlen(tOsdCfg.tSubStream.acText);
				if(iLen > 0)
				{
					utf8_t_gbk(tOsdCfg.tSubStream.acText,iLen,acText);
				}
				else
				{
					*eType = E_MEDIA_OSD_OPERATE_TYPE_DESTROY;
					return 0;
				}
			}
			else
			{
				return -1;
			}

			if(strcmp(acText,&acTextLast[iChn][0]))
			{
				strcpy(&acTextLast[iChn][0],acText);
				*eType = E_MEDIA_OSD_OPERATE_TYPE_RESET;
				printf("iChn:%d,acText:%s\n",iChn,acText);
			}
			else
			{
				*eType = E_MEDIA_OSD_OPERATE_TYPE_NULL;
			}

			sprintf(g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].acBuffer,"%s",acText);
			iLen = strlen(g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].acBuffer);
			if(iLen > 0)
	    	{
				g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].iEnable = 1;
				//printf("enable :acBuffer = %s\n",g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].acBuffer);
			}
			else
			{
				g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].iEnable = 0;
				//printf("disenable :acBuffer = %s\n",g_tMediaOsdCfg->tMediaOsdItem[iChn][E_MEDIA_OSD_TYPE_TEXT].acBuffer);
			}
		}
		#endif
		break;
		default:
		break;
	}
    return 0;
}


static int OsdUpdateRgnAttr(int iChn, int iRgnNum,E_MEDIA_OSD_OPERATE_TYPE eOsdOptType)
{
    int ret = -1,iLen = 0;
    result_s result;
    memset(&result, 0, sizeof(result));
	iLen = strlen(g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].acBuffer);
	if((eOsdOptType == E_MEDIA_OSD_OPERATE_TYPE_DESTROY)||(iLen <= 0) || !g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iEnable)
	{
		if (g_tMediaOsdCfg->param[iChn][iRgnNum].bitmap_data)
        {
            ret = goke_overlay_destory(&g_tMediaOsdCfg->param[iChn][iRgnNum]);
        }
		return 0;
	}
	
	if (eOsdOptType == E_MEDIA_OSD_OPERATE_TYPE_RESET)
    {
        if (g_tMediaOsdCfg->param[iChn][iRgnNum].bitmap_data)
        {
            ret = goke_overlay_destory(&g_tMediaOsdCfg->param[iChn][iRgnNum]);
            OSD_CHECK(ret == 0, -1, "%d:Error with %#x.\n",__LINE__, ret);
        }
    }
	ret = bitmap_alloc(&bitmap,(const unsigned char*)g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].acBuffer, g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iFontSize, 1, g_tMediaOsdCfg->gap, &result);
	OSD_CHECK(ret == 0, -1, "%d:Error with %#x.\n",__LINE__, ret);
    g_tMediaOsdCfg->param[iChn][iRgnNum].width = result.col;
    g_tMediaOsdCfg->param[iChn][iRgnNum].height = result.row;
    g_tMediaOsdCfg->param[iChn][iRgnNum].factor = g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iFontSize;

    int offset_h = g_tMediaOsdCfg->gap*2;
	int offset_v = g_tMediaOsdCfg->gap*2;
	if (g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].eLocation == E_MEDIA_OSD_LOCATION_LEFT_TOP)
    {
        g_tMediaOsdCfg->param[iChn][iRgnNum].posx = offset_h;
        g_tMediaOsdCfg->param[iChn][iRgnNum].posy = offset_v;
    }
    else if (g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].eLocation == E_MEDIA_OSD_LOCATION_LEFT_BOTTOM)
    {
        g_tMediaOsdCfg->param[iChn][iRgnNum].posx = offset_h;
        g_tMediaOsdCfg->param[iChn][iRgnNum].posy = g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iOsdHeight - g_tMediaOsdCfg->param[iChn][iRgnNum].height - offset_v;
    }
    else if (g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].eLocation == E_MEDIA_OSD_LOCATION_RIGHT_TOP)
    {
        g_tMediaOsdCfg->param[iChn][iRgnNum].posx = g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iOsdWidth - g_tMediaOsdCfg->param[iChn][iRgnNum].width - offset_h;
        g_tMediaOsdCfg->param[iChn][iRgnNum].posy = offset_v;
    }
    else if (g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].eLocation == E_MEDIA_OSD_LOCATION_RIGHT_BOTTOM)
    {
        g_tMediaOsdCfg->param[iChn][iRgnNum].posx = g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iOsdWidth - g_tMediaOsdCfg->param[iChn][iRgnNum].width - offset_h;
        g_tMediaOsdCfg->param[iChn][iRgnNum].posy = g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iOsdHeight - g_tMediaOsdCfg->param[iChn][iRgnNum].height - offset_v;
    }
    
    if (!g_tMediaOsdCfg->param[iChn][iRgnNum].bitmap_data)
    {
    	g_tMediaOsdCfg->param[iChn][iRgnNum].handle = g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iHandleId;
		g_tMediaOsdCfg->param[iChn][iRgnNum].stream_id = g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iBindEncChn;
		ret = goke_overlay_create(&g_tMediaOsdCfg->param[iChn][iRgnNum]);
        OSD_CHECK(ret == 0, -1, "%d:Error with %#x.\n",__LINE__, ret);
    }
    ret = osd_char2short(result.bit_map, result.row, result.col, (unsigned short*)g_tMediaOsdCfg->param[iChn][iRgnNum].bitmap_data);
    OSD_CHECK(ret == 0, -1, "%d:Error with %#x.\n",__LINE__, ret);
    ret = bitmap_free(&bitmap,&result);
    OSD_CHECK(ret == 0, -1, "%d:Error with %#x.\n",__LINE__, ret);
    return 0;
}

int osd_proc(void)
{
    int ret = -1;
	E_MEDIA_OSD_OPERATE_TYPE eOsdOptType = 0;
    unsigned int iChn = 0;
    unsigned int iRgnNum = 0;

    pthread_mutex_lock(&mutex);
    if( g_tMediaOsdCfg == 0 ) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    if( _config_.osd_enable ) {
        util_time_abs(&g_tMediaOsdCfg->current);
        for (iChn = E_MEDIA_STREAM_CHN_MAIN; iChn < E_MEDIA_STREAM_CHN_MAX; iChn++) {
            for (iRgnNum = E_MEDIA_OSD_RGNNUM0; iRgnNum < E_MEDIA_OSD_RGNNUM1; iRgnNum++)  { //maximum 2 now
                if( !g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iEnable) {
                    g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iEnable = 1;
                }
                ret = OsdUpdateContent(iChn, iRgnNum, &eOsdOptType);
                ret = OsdUpdateRgnAttr(iChn, iRgnNum, eOsdOptType);
                if ((g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iEnable) &&
                    (g_tMediaOsdCfg->param[iChn][iRgnNum].handle >= 0) &&
                    (g_tMediaOsdCfg->param[iChn][iRgnNum].handle < 128)) {
                    ret = goke_overlay_refresh_bitmap(&g_tMediaOsdCfg->param[iChn][iRgnNum]);
                    if (ret != 0) {
                        pthread_mutex_unlock(&mutex);
                        return 0;
                    }
                }
            }
        }
    } else {
        for (iChn = E_MEDIA_STREAM_CHN_MAIN; iChn < E_MEDIA_STREAM_CHN_MAX; iChn++) {
            for (iRgnNum = E_MEDIA_OSD_RGNNUM0; iRgnNum < E_MEDIA_OSD_RGNNUM1; iRgnNum++) {  //maximum 2 now
                if( g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iEnable) {
                    g_tMediaOsdCfg->tMediaOsdItem[iChn][iRgnNum].iEnable = 0;
                    ret = OsdUpdateRgnAttr(iChn, iRgnNum, eOsdOptType);
                }
            }
        }
    }
    pthread_mutex_unlock(&mutex);
    return 1;
}

int goke_osd_init()
{
    int ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;
	pthread_attr_t attr;
    g_tMediaOsdCfg = (T_MediaOsdCfg*)malloc(sizeof(T_MediaOsdCfg));
    memset(g_tMediaOsdCfg, 0, sizeof(T_MediaOsdCfg));

    g_tMediaOsdCfg->path_hzk = OSD_GB2312_FILE_PATH;
    g_tMediaOsdCfg->path_asc = OSD_ASC_FILE_PATH;
    g_tMediaOsdCfg->gap = 8;

    ret = bitmap_create(&bitmap,g_tMediaOsdCfg->path_hzk, g_tMediaOsdCfg->path_asc,0,16);
	printf("#####hisi_osd_init ret=%d\n",ret);
	OSD_CHECK(ret == 0, -1, "%d:Error with %d.\n",__LINE__, ret);

    g_tMediaOsdCfg->eOsdLanguage = E_MEDIA_OSDLANGUAGE_EN;//mk_get_language(); // 0 ���� 1 eng
    g_tMediaOsdCfg->eOsdTimeType = E_MEDIA_OSDTIME_TYPE0;

	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iEnable = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].eLocation = E_MEDIA_OSD_LOCATION_LEFT_TOP;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iHandleId = E_MEDIA_STREAM_CHN_MAIN * E_MEDIA_OSD_RGNNUM_MAX + E_MEDIA_OSD_RGNNUM0;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iFontSize = 4;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iBindEncChn = 0;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iOsdType = E_MEDIA_OSD_TYPE_TIME;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iOsdWidth = 1920;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iOsdHeight = 1080;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].stream_id = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iBindEncChn;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].handle = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iHandleId;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].factor = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM0].iFontSize;
	
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iEnable = 0;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].eLocation = E_MEDIA_OSD_LOCATION_RIGHT_BOTTOM;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iHandleId = E_MEDIA_STREAM_CHN_MAIN * E_MEDIA_OSD_RGNNUM_MAX + E_MEDIA_OSD_RGNNUM1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iFontSize = 4;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iBindEncChn = 0;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iOsdType = E_MEDIA_OSD_TYPE_TEXT;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iOsdWidth = 1920;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iOsdHeight = 1080;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].stream_id = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iBindEncChn;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].handle = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iHandleId;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].factor = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_MAIN][E_MEDIA_OSD_RGNNUM1].iFontSize;
	
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iEnable = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].eLocation = E_MEDIA_OSD_LOCATION_LEFT_TOP;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iHandleId = E_MEDIA_STREAM_CHN_SUB * E_MEDIA_OSD_RGNNUM_MAX + E_MEDIA_OSD_RGNNUM0;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iFontSize = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iBindEncChn = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iOsdType = E_MEDIA_OSD_TYPE_TIME;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iOsdWidth = 640;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iOsdHeight = 480;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].stream_id = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iBindEncChn;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].handle = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iHandleId;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].factor = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM0].iFontSize;
	
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iEnable = 0;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].eLocation = E_MEDIA_OSD_LOCATION_RIGHT_BOTTOM;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iHandleId = E_MEDIA_STREAM_CHN_SUB * E_MEDIA_OSD_RGNNUM_MAX + E_MEDIA_OSD_RGNNUM1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iFontSize = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iBindEncChn = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iOsdType = E_MEDIA_OSD_TYPE_TEXT;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iOsdWidth = 640;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iOsdHeight = 480;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].stream_id = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iBindEncChn;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].handle = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iHandleId;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].factor = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_SUB][E_MEDIA_OSD_RGNNUM1].iFontSize;


	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iEnable = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].eLocation = E_MEDIA_OSD_LOCATION_LEFT_TOP;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iHandleId = E_MEDIA_STREAM_CHN_JPEG * E_MEDIA_OSD_RGNNUM_MAX + E_MEDIA_OSD_RGNNUM0;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iFontSize = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iBindEncChn = 1;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iOsdType = E_MEDIA_OSD_TYPE_TIME;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iOsdWidth = 640;
	g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iOsdHeight = 480;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].stream_id = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iBindEncChn;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].handle = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iHandleId;
	g_tMediaOsdCfg->param[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].factor = g_tMediaOsdCfg->tMediaOsdItem[E_MEDIA_STREAM_CHN_JPEG][E_MEDIA_OSD_RGNNUM0].iFontSize;
	
    g_tMediaOsdCfg->running = 1;

    return 0;
}

int goke_osd_exit()
{
    OSD_CHECK(NULL != g_tMediaOsdCfg, -1,"moudle is not inited.\n");

    int ret = -1;
    unsigned int i = 0;
    unsigned int j = 0;
    pthread_mutex_lock(&mutex);
    if (g_tMediaOsdCfg->running)
    {
        g_tMediaOsdCfg->running = 0;
//        pthread_join(g_tMediaOsdCfg->pid, NULL);
    }

    for (i = E_MEDIA_STREAM_CHN_MAIN; i < E_MEDIA_STREAM_CHN_MAX; i++)
    {
        for (j = E_MEDIA_OSD_RGNNUM0; j < E_MEDIA_OSD_RGNNUM_MAX; j++)
        {
            if (g_tMediaOsdCfg->param[i][j].bitmap_data)
            {
                ret = goke_overlay_destory(&g_tMediaOsdCfg->param[i][j]);
                OSD_CHECK(ret == 0, -1, "%d:Error with %#x.\n",__LINE__, ret);
            }
        }
    }

    ret = bitmap_destroy(&bitmap);
    OSD_CHECK(ret == 0, -1, "%d:Error with %d.\n",__LINE__, ret);
    free(g_tMediaOsdCfg);
    g_tMediaOsdCfg = NULL;
    pthread_mutex_unlock(&mutex);
    return 0;
}


