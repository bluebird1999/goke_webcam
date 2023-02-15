/*
 * header
 */
//system header
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
//program header
#include "../../common/tools_interface.h"
#include "../../server/goke/hisi/hisi.h"
#include "../../server/goke/goke_interface.h"
//server header
#include "osd.h"
#include "config.h"


/*
 * static
 */
#define         CHAR_NUM        13

//variable
static osd_run_t *osd_run[VPSS_MAX_PHY_CHN_NUM];
static char patt[CHAR_NUM] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', ' ', ':'};
static char offset_x[CHAR_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2};
static char offset_y[CHAR_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1};

/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */

/*
 * helper
 */
static int osd_uninit_layer(osd_run_t *osd_run) {
    int ret = 0;
    MPP_CHN_S stChn;

    if (osd_run == NULL) {
        return -1;
    }
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = osd_run->device;
    stChn.s32ChnId = osd_run->venc_chn;
    ret = hisi_detach_from_chn(osd_run->layer, &stChn);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "SAMPLE_COMM_REGION_AttachToChn failed");
        return HI_FAILURE;
    }
    ret = hisi_destroy_region(osd_run->layer);
    if (HI_SUCCESS != ret) {
        log_goke(DEBUG_WARNING, "SAMPLE_COMM_REGION_AttachToChn failed");
        return HI_FAILURE;
    }
    //memory
    if (osd_run->ipattern) {
        free(osd_run->ipattern);
        osd_run->ipattern = NULL;
    }
    if (osd_run->image2222) {
        free(osd_run->image2222);
        osd_run->image2222 = NULL;
    }
    if (osd_run->image4444) {
        free(osd_run->image4444);
        osd_run->image4444 = NULL;
    }
    //font
    if (osd_run->face) {
        ret = FT_Done_Face(osd_run->face);
        osd_run->face = NULL;
    }
    if (osd_run->library) {
        ret = FT_Done_FreeType(osd_run->library);
        osd_run->library = NULL;
    }
    free(osd_run);
    return ret;
}

static void osd_image_to_8888(unsigned char *src, unsigned char *dst, unsigned int len, int alpha) {
    int i, j;
    for (i = 0; i < len; i++) {
        j = i * 4;
        dst[j] = (src[i] << 6) & 0xC0;
        dst[j + 1] = (src[i] << 4) & 0xC0;
        dst[j + 2] = (src[i] << 2) & 0xC0;
        if (alpha != 0)
            dst[j + 3] = alpha;
        else
            dst[j + 3] = (src[i] & 0xC0) ? (src[i] & 0xC0) : 0;
    }
}

static void osd_image_to_4444(unsigned char *src, unsigned char *dst, unsigned int len, int alpha) {
    int i, j;
    for (i = 0; i < len; i++) {
        j = i * 2;
        dst[j] = ((src[i] << 2) & 0x0C) | ((src[i] << 4) & 0xC0);
        dst[j + 1] = (src[i] >> 2) & 0x0C;
        if (alpha != 0)
            dst[j + 1] |= (alpha & 0xF0);
        else
            dst[j + 1] |= (((src[i] & 0xC0) ? (src[i] & 0xC0) : 0) & 0xF0);
    }
}

static void osd_draw_image_pattern(osd_run_t *config, FT_Bitmap *bitmap, FT_Int x, FT_Int y,
                                   unsigned char *buf, int flag_ch) {
    FT_Int i, j, p, q;
    FT_Int x_max = x + bitmap->width;
    FT_Int y_max = y + bitmap->rows;
    int width;
    int height;
    unsigned int len;
    if (config->rotate) {
        width = config->size;
        height = config->size / 2;
    } else {
        width = config->size / 2;
        height = config->size;
    }
    if (flag_ch) {
        if (config->rotate)
            height = config->size;
        else
            width = config->size;
    }
    for (i = x, p = 0; i < x_max; i++, p++) {
        for (j = y, q = 0; j < y_max; j++, q++) {
            if (i < 0 || j < 0 || i >= width || j >= height)
                continue;
            buf[j * width + i] |= bitmap->buffer[q * bitmap->width + p];
        }
    }
    len = width * height;
    for (i = 0; i < len; i++)
        buf[i] = buf[i] > 0 ? ((buf[i] & 0xC0) | config->color) : buf[i];
}

static int osd_get_picture_from_pattern(osd_run_t *config, osd_text_info_t *txt) {
    int i;
    int val;
    char *text = txt->text;
    int len = strlen(text);
    for (i = 0; i < len; i++) {
        if (text[i] == '-')
            val = 10;
        else if (text[i] == ' ')
            val = 11;
        else if (text[i] == ':')
            val = 12;
        else
            val = (int) (text[i] - '0');
        if (config->rotate) {
            memcpy((config->image2222 + (len - i - 1) * config->size * config->size / 2),
                   (config->ipattern + val * config->size * config->size / 2),
                   config->size * config->size / 2);
        } else {
            int p;
            int q;
            unsigned char *src = config->ipattern + val * config->size * config->size / 2;
            for (p = 0; p < config->size; p++) {
                for (q = 0; q < config->size / 2; q++)
                    config->image2222[p * config->size / 2 * txt->cnt + i * config->size / 2 + q] =
                            src[p * config->size / 2 + q];
            }
        }
    }
    osd_image_to_4444(config->image2222, config->image4444, config->size * config->size / 2 * txt->cnt,
                      config->alpha);
    txt->pdata = config->image4444;
    txt->len = config->size * config->size / 2 * txt->cnt * 2;
    return 0;
}

static int osd_load_char(osd_run_t *config, unsigned short c, unsigned char *pdata) {
    int ret = 0, j = 0;
    FT_GlyphSlot slot;
    FT_Matrix matrix;                 /* transformation matrix */
    FT_Vector pen;
    FT_Error error;
    double angle;
    int target_height;
    double angle_tmp;
    int origin_x;
    int origin_y;
    unsigned int width;
    if (!pdata)
        return -1;
    int flag_ch = c > 127 ? 1 : 0;
    if (flag_ch)
        width = config->size;
    else
        width = config->size / 2;
    if (config->rotate) {
        angle_tmp = 90.0;
        target_height = width;
        origin_x = config->size;// - osd_run.offset_x;
        if (flag_ch)
            origin_y = config->size / 2 * 2;
        else
            origin_y = config->size / 2;
    } else {
        angle_tmp = 0.0;
        target_height = config->size;
        angle_tmp = 0.0;
        target_height = config->size;
        origin_x = 0;
        origin_y = config->size;
        for (j = 0; j < CHAR_NUM; j++) {
            if (patt[j] == c) {
                origin_x += offset_x[j] * (int) (config->size / 16);
                origin_y -= offset_y[j] * (int) (config->size / 16);
                break;
            }
        }
    }
    FT_Face *pface = &config->face;
    angle = (angle_tmp / 360) * 3.14159 * 2;
    slot = (*pface)->glyph;
    /* set up matrix */
    matrix.xx = (FT_Fixed) (cos(angle) * 0x10000L);
    matrix.xy = (FT_Fixed) (-sin(angle) * 0x10000L);
    matrix.yx = (FT_Fixed) (sin(angle) * 0x10000L);
    matrix.yy = (FT_Fixed) (cos(angle) * 0x10000L);
    /* the pen position in 26.6 cartesian space coordinates; */
    /* start at (origin_x, origin_y) relative to the upper left corner  */
    pen.x = origin_x * 64;
    pen.y = (target_height - origin_y) * 64;
    /* set transformation */
    FT_Set_Transform(*pface, &matrix, &pen);
    /* load glyph image into the slot (erase previous one) */
    error = FT_Load_Char(*pface, c, FT_LOAD_RENDER);
    if (error)
        return -1;           /* ignore errors */
    /* now, draw to our target surface (convert position) */
    osd_draw_image_pattern(config, &slot->bitmap, slot->bitmap_left,
                           target_height - slot->bitmap_top,
                           pdata, flag_ch);
    return ret;
}

static int osd_set_osd_timedate(osd_run_t *config, osd_text_info_t *text) {
    int ret = 0;
    BITMAP_S stBitmap;
    ret = osd_get_picture_from_pattern(config,text);
    if (ret < 0) {
        log_goke(DEBUG_SERIOUS, "%s, get blk pict fail", __func__);
        return ret;
    }
    stBitmap.u32Width = (int) (text->len / config->size);
    stBitmap.u32Height = config->size;
    stBitmap.enPixelFormat = config->pixel_format;
    stBitmap.pData = text->pdata;
    //set bitmap
    ret = hisi_set_region_bitmap(&stBitmap, config->layer);
    if (ret)
        log_goke(DEBUG_INFO, "set osd fail, ret = %d", ret);
    return ret;
}

/*
 * interface
 */
int video_osd_write_time(int channel) {
    char now_time[20] = "";
    int ret;
    MPP_CHN_S stChn;
    osd_text_info_t text_tm;
    time_t now;
    struct tm tm = {0};
    osd_run_t *config = osd_run[channel];
    if(config==NULL) {
        log_goke( DEBUG_SERIOUS, "osd channel is null");
        return -1;
    }
    //**color
    now = time(NULL);
    localtime_r(&now, &tm);
    //***
    sprintf(now_time, "%04d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    text_tm.text = now_time;
    text_tm.cnt = strlen(now_time);
    if (config->rotate) {
        text_tm.x = config->offset_left;
        text_tm.y = config->offset_top;
    } else {
        text_tm.x = config->offset_left;
        text_tm.y = config->offset_top;
    }
    //channel attributes
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = config->device;
    stChn.s32ChnId = config->venc_chn;
//    ret = hisi_detach_from_chn(config->layer, &stChn);
//    if (ret) {
//        log_goke(DEBUG_WARNING, "%s region detached fail ret =%x", __func__, ret);
//        return -1;
//    }
    ret = osd_set_osd_timedate(config, &text_tm);
    if (ret < 0) {
        log_goke(DEBUG_INFO, "%s, set osd2 attr fail", __func__);
        return -1;
    }
    return ret;
}

int video_init_osd_layer(int device, region_layer_info_t *config) {
    int i, ret = 0;
    int size;
    char face_path[MAX_SYSTEM_STRING_SIZE];
    RGN_ATTR_S stRegion;
    MPP_CHN_S stChn;
    if (osd_run[config->id] == 0) {
        osd_run[config->id] = (osd_run_t *) calloc(sizeof(osd_run_t), 1);
        if (!osd_run[config->id]) {
            log_goke(DEBUG_WARNING, "%s calloc fail", __func__);
            return -1;
        }
        osd_run[config->id]->device = device;
        osd_run[config->id]->venc_chn = config->venc_chn;
        osd_run[config->id]->layer = config->layer;
        osd_run[config->id]->id = config->id;
        osd_run[config->id]->size = config->size;
        osd_run[config->id]->alpha = config->alpha;
        osd_run[config->id]->color = config->color;
        osd_run[config->id]->rotate = config->rotate;
        osd_run[config->id]->pixel_format = config->pixel_format;
        osd_run[config->id]->offset_left = config->offset_left;
        osd_run[config->id]->offset_top = config->offset_top;
    }
    //init freetype
    if (!osd_run[config->id]->library && !osd_run[config->id]->face) {
        FT_Init_FreeType(&osd_run[config->id]->library);
        memset(face_path, 0, sizeof(face_path));
        snprintf(face_path, 32, "%sfont/%s%s", manager_config.goke_path,
                 config->font_face, ".ttf");
        FT_New_Face(osd_run[config->id]->library, face_path, 0, &osd_run[config->id]->face);
        //font size
        size = config->size * config->size / 2;
        FT_Set_Pixel_Sizes(osd_run[config->id]->face, config->size, 0);
    }
    //memory allocation
    osd_run[config->id]->ipattern = (unsigned char *) calloc(size, CHAR_NUM);
    if (!osd_run[config->id]->ipattern) {
        log_goke(DEBUG_WARNING, "%s calloc fail", __func__);
        osd_uninit_layer(osd_run);
        return -1;
    }
    osd_run[config->id]->image2222 = (unsigned char *) calloc(MAX_OSD_LABLE_SIZE * size, 1);
    if (!osd_run[config->id]->image2222) {
        log_goke(DEBUG_WARNING, "%s calloc fail", __func__);
        osd_uninit_layer(osd_run);
        return -1;
    }
    osd_run[config->id]->image4444 = (unsigned char *) calloc(MAX_OSD_LABLE_SIZE * 2 * size, 1);
    if (!osd_run[config->id]->image4444) {
        log_goke(DEBUG_WARNING, "%s calloc fail", __func__);
        osd_uninit_layer(osd_run);
        return -1;
    }
    //load char based on size/color/rotate
    for (i = 0; i < sizeof(patt); i++) {
        osd_load_char(osd_run[config->id], (unsigned short) patt[i], osd_run[config->id]->ipattern + size * i);
    }
    //create overlay
    int width = config->size / 2 * MAX_OSD_LABLE_SIZE;
    ret = hisi_create_overlay_mix(config->layer, config->size/2,width,
                                  config->pixel_format, config->color);
    if (ret) {
        log_goke(DEBUG_WARNING, "%s calloc fail", __func__);
        osd_uninit_layer(osd_run[config->id]);
        return -1;
    }
    //attache to encoder
    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = device;
    stChn.s32ChnId = config->venc_chn;
    ret = hisi_attach_region_to_chn(config->layer, config->offset_left, config->offset_top, &stChn);
    if (ret) {
        log_goke(DEBUG_WARNING, "%s region attached fail ret =%x", __func__, ret);
        return -1;
    }
    return ret;
}

int video_uninit_osd_layer(void) {
    int ret = 0;
    for (int i = 0; i < VPSS_MAX_PHY_CHN_NUM; i++) {
        ret |= osd_uninit_layer(osd_run[i]);
    }
    return ret;
}


