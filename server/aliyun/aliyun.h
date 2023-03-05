#ifndef SERVER_ALIYUN_ALIYUN_H
#define SERVER_ALIYUN_ALIYUN_H

#include "aliyun_interface.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum aliyun_init_condition_e {
    ALIYUN_INIT_CONDITION_NUM = 0,
};

typedef enum wifi_status_e{
    WIFI_QRCODE,
    WIFI_STA,
} wifi_status_e;

#define		ALIYUN_EXIT_CONDITION				    ( 0 )
#define     ALIYUN_MAX_NO_BUFFER_TIMES              10

#define		WPA_SUPPLICANT_CONF_BAK_NAME	"/app/config/wpa_wifi_conf.bak"
#define     WPA_SUPPLICANT_CONF_NAME        "/app/config/wpa_wifi.conf"
#define     PRODUCT_CONFIG_NAME             "/app/config/product.conf"
#define     WIFI_INFO_CONF_TEMP             "/app/config/wifi.temp"
#define     WIFI_INFO_CONF                  "/app/config/wifi.conf"
#define     FIRMWARE_INFO_TEMP              "/app/config/firmware.temp"
#define     APPLICATION_CONFIG_FILE         "/app/bin/webcam.config"

#define     EXAMPLE_MASTER_DEVID            (0)
#define     EXAMPLE_YIELD_TIMEOUT_MS        (200)

#define     BSSID_LENGTH        13
#define     TOKEN_LENGTH        7


int wifi_connect(void);
void send_qr_scan();
int wifi_link_ok();

#ifdef __cplusplus
}
#endif

#endif

