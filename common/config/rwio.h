#ifndef COMMON_CONFIG_RWIO_H_
#define COMMON_CONFIG_RWIO_H_

/*
 * header
 */
#include "../cJSON/cJSON.h"

/*
 * define
 */

/*
 * structure
 */

//config map data type
typedef enum {
	cfg_u32 = 0,
	cfg_u16,
	cfg_u8,
	cfg_s32,
	cfg_s16,
	cfg_s8,
	cfg_float,
	cfg_string,
	cfg_stime,
	cfg_slice,
} config_data_type_t;

//config map item
typedef struct config_map_t
{
    char* 					string_name;
    void* 					data_address;
    config_data_type_t		data_type;
    char*                   default_value;
    double 					min;
    double 					max;
} config_map_t;

/*
 * function
 */
char* read_json_file(const char *filename);
int write_json_file(const char *file, const char *data);
cJSON* load_json(const char *file);
int cjson_to_data_by_map(config_map_t *map, cJSON *json);
int data_to_json_by_map(config_map_t *map, cJSON *root);
int config_add_property(config_map_t *map, cJSON *root);
int read_config_file(config_map_t *map, char *fpath);
int write_config_file(config_map_t *map, char *fpath);

#endif
