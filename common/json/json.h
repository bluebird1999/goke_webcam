#ifndef COMMON_JSON_JSON_H_
#define COMMON_JSON_JSON_H_

/*
 * header
 */
#include <json-c/json.h>

/*
 * define
 */

/*
 * structure
 */

/*
 * function
 */
int json_verify(const char *string);
int json_verify_method(const char *string, char *method);
int json_verify_method_value(const char *string, char *method, void *value, enum json_type type);
int json_verify_get_int(const char *string, char *key, int *value);
int json_verify_get_string(const char *string, char *key, char *value, int val_len);
int json_verify_get_array(const char *string, char *key, char *value, int val_len);
void json_del_yinhao(char *str);

#endif
