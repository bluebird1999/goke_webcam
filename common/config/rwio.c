/*
 * header
 */
//system header
#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
//program header
#include "../log.h"
//server header
#include "rwio.h"

/*
 * static
 */
//variable

//function




/*
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 * %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
 */
/*
 * internal interface
 */
int read_config_file(config_map_t *map, char *fpath)
{
	cJSON *root = 0;
	root = load_json(fpath);
	if( cjson_to_data_by_map(map,root) ) {
		printf("Reading configuration file failed: %s\n", fpath);
		return -1;
	}
	cJSON_Delete(root);
	return 0;
}

int write_config_file(config_map_t *map, char *fpath)
{
    cJSON *root;
    char *out;
    root = cJSON_CreateObject();
    data_to_json_by_map(map,root);
    out = cJSON_Print(root);
    int ret = write_json_file(fpath, out);
    if (ret != 0) {
        printf("CfgWriteToFile %s error.\n", fpath);
        free(out);
        return -1;
    }
    free(out);
    cJSON_Delete(root);
    return 0;
}


/*
 * interface
 */
char* read_json_file(const char *filename)
{
    //PRINT_INFO("to load %s", filename);
    FILE *fp = NULL;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return NULL;
    }

    int fileSize;
    if (0 != fseek(fp, 0, SEEK_END)) {
        fclose(fp);
        return NULL;
    }
    fileSize = ftell(fp);

    char *data = NULL;
//  data = malloc(fileSize);
    data = malloc(fileSize + 1);
    if(!data) {
        fclose(fp);
        return NULL;
    }
    memset(data, 0, fileSize+1);
    if(0 != fseek(fp, 0, SEEK_SET)) {
    	free(data);
        fclose(fp);
        return NULL;
    }
    if (fread(data, 1, fileSize, fp) != (fileSize)) {
    	free(data);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    return data;
}

int write_json_file(const char *file, const char *data)
{
    FILE *fp = NULL;
    fp = fopen(file, "wb+");
    if (fp == NULL) {
        printf("fopen:%s fail\n", file);
        return -1;
    }

    int len = strlen(data);
    if (fwrite(data, 1, len, fp) != len) {
        printf("fwrite:%s fail\n", file);
        fclose(fp);
        return -1;
    }

	fflush(fp);
    fclose(fp);
    return 0;
}

cJSON* load_json(const char *file)
{
    char *data = NULL;
    data = read_json_file(file);
    if (data == NULL) {
        printf("load %s error, so to load default cfg param.\n", file);
        return NULL;
    }
    cJSON *json = NULL;
    json = cJSON_Parse(data);
    if (!json) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        free(data);
        return NULL;
    }
    free(data);
    return json;
}

int cjson_to_data_by_map(config_map_t *map, cJSON *json)
{
	int ret=0;
    int tmp;
	double temp;
	cJSON *js = NULL;

    int i =0;
    while(map[i].string_name != NULL) {
    	js = cJSON_GetObjectItem(json, map[i].string_name);
        switch (map[i].data_type) {
            case cfg_u32:
                if( js )
                    tmp = js->valueint;
                if ( !js || tmp > map[i].max || tmp < map[i].min)
                    tmp = strtoul(map[i].default_value, NULL, 0);
                *((unsigned int *)(map[i].data_address)) = (unsigned int)tmp;
                break;
            case cfg_u16:
                if( js )
                    tmp = js->valueint;
                if ( !js || tmp > map[i].max || tmp < map[i].min)
                    tmp = strtoul(map[i].default_value, NULL, 0);
                *((unsigned short *)(map[i].data_address)) = (unsigned short)tmp;
                break;
            case cfg_u8:
                if( js )
                    tmp = js->valueint;
                if ( !js || tmp > map[i].max || tmp < map[i].min)
                    tmp = strtoul(map[i].default_value, NULL, 0);
                *((unsigned char *)(map[i].data_address)) = (unsigned char)tmp;
                break;
            case cfg_s32:
                if( js )
                    tmp = js->valueint;
                if ( !js || tmp > map[i].max || tmp < map[i].min)
                    tmp = strtol(map[i].default_value, NULL, 0);
                *((int *)(map[i].data_address)) = tmp;
                break;
            case cfg_s16:
                if( js )
                    tmp = js->valueint;
                if ( !js || tmp > map[i].max || tmp < map[i].min)
                    tmp = strtol(map[i].default_value, NULL, 0);
                *((signed short *)(map[i].data_address)) = (signed short)tmp;
                break;
            case cfg_s8:
                if( js )
                    tmp = js->valueint;
                if ( !js || tmp > map[i].max || tmp < map[i].min)
                    tmp = strtol(map[i].default_value, NULL, 0);
                *((signed char *)(map[i].data_address)) = (signed char)tmp;
                break;
    		case cfg_float:
                if( js )
                    temp = js->valuedouble;
                if ( !js || temp > map[i].max || temp < map[i].min)
                    temp = strtod(map[i].default_value, NULL);
                *(( float *)(map[i].data_address)) = ( float)temp;
                break;
            case cfg_string:
    			if ( map[i].max > 1 ) {
                    if( js ) {
                        tmp = strlen(js->valuestring);
                        strncpy((char *) map[i].data_address, js->valuestring, tmp);
                        ((char *) map[i].data_address)[tmp] = '\0';
                    }
                    else {
                        tmp = strlen(map[i].default_value);
                        strncpy((char *) map[i].data_address, map[i].default_value, tmp);
                        ((char *) map[i].data_address)[tmp] = '\0';
                    }
                } else {
                    printf("if type is string, addr can't be null and the upper limit must greater than 1.\n");
                }
                break;
            default:
            	ret = 1;
                printf("unknown data type in map definition\n");
                break;
        }
        i++;
    }
    return ret;
}

int data_to_json_by_map(config_map_t *map, cJSON *root)
{
    int tmp;
	double temp;

    int i =0;
    while(map[i].string_name != NULL)
    {
		switch (map[i].data_type)
		{
			case cfg_u32:
				tmp = *((unsigned int *)(map[i].data_address));
				if (tmp > map[i].max || tmp < map[i].min)
					tmp = strtoul(map[i].default_value, NULL, 0);
				cJSON_AddNumberToObject(root, map[i].string_name, tmp);
				break;
			case cfg_u16:
				tmp = *((unsigned short *)(map[i].data_address));
				if (tmp > map[i].max || tmp < map[i].min)
					tmp = strtoul(map[i].default_value, NULL, 0);
				cJSON_AddNumberToObject(root, map[i].string_name, tmp);
				break;
			case cfg_u8:
				tmp = *((unsigned char *)(map[i].data_address));
				if (tmp > map[i].max || tmp < map[i].min)
					tmp = strtoul(map[i].default_value, NULL, 0);
				cJSON_AddNumberToObject(root, map[i].string_name, tmp);
				break;
			case cfg_s32:
				tmp = *((signed int *)(map[i].data_address));
				if (tmp > map[i].max || tmp < map[i].min)
					tmp = strtol(map[i].default_value, NULL, 0);
				cJSON_AddNumberToObject(root, map[i].string_name, tmp);
				break;
			case cfg_s16:
				tmp = *((signed short *)(map[i].data_address));
				if (tmp > map[i].max || tmp < map[i].min)
					tmp = strtol(map[i].default_value, NULL, 0);
				cJSON_AddNumberToObject(root, map[i].string_name, tmp);
				break;
			case cfg_s8:
				tmp = *((signed char *)(map[i].data_address));
				if (tmp > map[i].max || tmp < map[i].min)
					tmp = strtol(map[i].default_value, NULL, 0);
				cJSON_AddNumberToObject(root, map[i].string_name, tmp);
				break;
			case cfg_float:
				temp = *(( float *)(map[i].data_address));
				if (temp > map[i].max || temp < map[i].min)
					temp = strtod(map[i].default_value, NULL);
				cJSON_AddNumberToObject(root, map[i].string_name, temp);
				break;
			case cfg_string:
				cJSON_AddStringToObject(root, map[i].string_name, (char *)map[i].data_address);//
				break;
			default:
				printf("unknown data type in map definition\n");
				break;
		}
//	    config_add_property(map, root);
		i++;
    }
    return 0;
}

int config_add_property(config_map_t *map, cJSON *root)
{
	cJSON *Item = cJSON_CreateProperty();
	char etype[][8]={"U32","U16","U8","S32","S16","S8","FLOAT","STRING","STIME","SLICE"};
	char strname[128]={0};
	sprintf(strname,"%s%s",map->string_name,"Property");

	cJSON_AddItemToObject(Item, "type", cJSON_CreateString(etype[map->data_type]));
	cJSON_AddItemToObject(Item, "min", cJSON_CreateNumber(map->min));
	cJSON_AddItemToObject(Item, "max", cJSON_CreateNumber(map->max));
	if(map->data_type < cfg_string) {
		cJSON_AddItemToObject(Item, "def", cJSON_CreateNumber(strtoul(map->default_value, NULL, 0)));
	}
	else
		cJSON_AddItemToObject(Item, "def", cJSON_CreateString(map->default_value));

    cJSON_AddItemToObject(root, strname, Item);
    return 0;
}
