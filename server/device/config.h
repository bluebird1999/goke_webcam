#ifndef DEVICE_CONFIG_H_
#define DEVICE_CONFIG_H_

/*
 * header
 */
#include "string.h"

/*
 * define
 */
#define 	CONFIG_DEVICE_CONFIG_PATH			"/app/config/device_profile.config"

/*
 * structure
 */
typedef struct device_config_t {
	unsigned int			volume_step;
	unsigned int			soft_hard_ldr;
	unsigned int			day_night_lim;
	unsigned int			reset_key_detect_enable;
	unsigned int			storage_detect;
	unsigned int			storage_detect_notify;
	unsigned int			storage_detect_lim;
	unsigned int			motor_enable;
	unsigned int			motor_step;
	unsigned int			motor_speed;
	unsigned int			led1_gpio;
	unsigned int			led1_gpio_mcu;
	unsigned int			led1_effect_level;
	unsigned int			led2_gpio;
	unsigned int			led2_gpio_mcu;
	unsigned int			led2_effect_level;
	unsigned int			spk_gpio;
	unsigned int			spk_gpio_mcu;
	unsigned int			spk_effect_level;
	unsigned int			ircut_ain;
	unsigned int			ircut_ain_mcu;
	unsigned int			ircut_a_effect_level;
	unsigned int			ircut_bin;
	unsigned int			ircut_bin_mcu;
	unsigned int			ircut_b_effect_level;
	unsigned int			irled;
	unsigned int			irled_mcu;
	unsigned int			irled_effect_level;
	unsigned int			motor_595enable;
	unsigned int			motor_595enable_mcu;
	unsigned int			moto_595_effect_level;
	char					user_mount_path[64];
	char					sd_mount_path[64];
} device_config_t;


device_config_t device_config;

#endif
