#ifndef __STAR_BITMAP_H__
#define __STAR_BITMAP_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

#define FORE_GROUND 	0xff   	//前景色
#define BACK_GROUND 	0		//背景色
#define FONT_EDGE 		'o'

#define MAX_LINE_NUM 1
#define MAX_LINE_PLL 100
typedef struct tag_bitmap_s
{
    int inited;
    int gbk; // 1: gbk 0: gb2312
	int bit_width;//16*16:16   32*32:32
    char hzk_path[128];
    char asc_path[128];
	
	FILE *hz_fp;
    FILE *asc_fp;
}bitmap_s;

typedef struct
{
    int row;
    int col;
    char* bit_map;
}result_s;

int bitmap_create(bitmap_s *bitmap,const char* hz_path, const char* asc_path, int is_gbk,int bit_width);
int bitmap_destroy(bitmap_s *bitmap);
int bitmap_alloc(bitmap_s *bitmap,const unsigned char* chs, int font_size, int edge_enable, int gap, result_s* result);
int bitmap_free(bitmap_s *bitmap,result_s* result);
//int bitmap_demo();




#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
