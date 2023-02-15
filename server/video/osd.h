#ifndef SERVER_VIDEO_OSD_H_
#define SERVER_VIDEO_OSD_H_

/*
 * header
 */
#include <ft2build.h>
#include FT_FREETYPE_H

#include "config.h"

/*
 * define
 */

/*
 * structure
 */
typedef struct osd_run_t {
    int                         id;
    int                         device;
    int                         venc_chn;
    int                         layer;
    unsigned char				*ipattern;
	unsigned char				*image2222;
	unsigned char				*image4444;
	FT_Library 					library;
    FT_Face 					face;
    int                         offset_left;
    int                         offset_top;
    int                         size;
    int                         alpha;
    PIXEL_FORMAT_E pixel_format;
    int         color;
    int         rotate;
} osd_run_t;

typedef struct osd_text_info_t {
    char        *text;
    int         cnt;
    uint32_t    x;
    uint32_t    y;
    uint8_t     *pdata;
    uint32_t    len;
} osd_text_info_t;

/*
 * function
 */
int video_init_osd_layer(int device, region_layer_info_t *config);
int video_uninit_osd_layer(void);
int video_osd_write_time(int channel);

#endif
