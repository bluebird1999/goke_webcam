//system
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
//library
#include <iot_export_linkkit.h>
#include <iot_export.h>
#include <iot_import.h>
#include <connect_ap.h>
//server
#include "../../common/tools_interface.h"
#include "../../server/video/video_interface.h"
#include "../../server/audio/audio_interface.h"
#include "../../server/player/player_interface.h"
//local
#include "linkkit_client.h"
#include "linkvisual_client.h"
#include "aliyun_interface.h"
#include "aliyun.h"
#include "config.h"

/*
 * static
 */
//variable
static bool need_bind = 0;
static message_buffer_t message;
static server_info_t info;
static pthread_rwlock_t ilock = PTHREAD_RWLOCK_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static int aliyun_time_sync_label = 0;

static char product_key[IOTX_PRODUCT_KEY_LEN + 1] ;//= "a1uTBpO8IX9";//"a2PZnDV3LQw";
static char product_secret[IOTX_DEVICE_SECRET_LEN + 1] ;//= "SZrOvCxZ5ZYRXG1J";
static char device_name[IOTX_DEVICE_NAME_LEN + 1] ;//= "fakl1MVW3KyMqKZcsOvY";
static char device_secret[IOTX_DEVICE_SECRET_LEN + 1] ;//= "fca5f358a4885953d9bf882b0170f6c6";//8daf0be2444606808c2d036a34a460c8";

static wifi_status_e wifi_st;
static char ssid[HAL_MAX_SSID_LEN];
static char passwd[HAL_MAX_PASSWD_LEN];
static char bssid[BSSID_LENGTH];
static char token[TOKEN_LENGTH];

//common
static void *server_func(void);

static int server_message_proc(void);

static void state_machine(void);

static void task_proc(void);

static void server_release_1(void);

static void server_release_2(void);

static void server_release_3(void);

static int server_init(void);

static int server_setup(void);

static int server_stop(void);

static int server_restart(void);

static void server_thread_termination(int sign);
//
static void init_product(void);

static int wifi_configure(FILE *fp);

char *strerror(int errnum);
/*
 *
 * helper
 *
 *
 */
static int get_qrcode_callback(message_t *msg) {
    FILE *fp;
    int ret = -1;
    cJSON *root;
    cJSON *item;
    char *result;
    int len;
    char *json_item_str;
    message_t send_msg;

    result = (char *) msg->arg;
    if (result == 0) {
        send_qr_scan(); //repeat
        return 1;
    }
    len = strlen(result);
    log_goke(DEBUG_WARNING, "qrcode result = %s\r", result);
    memset(ssid, 0, sizeof(ssid));
    memset(passwd, 0, sizeof(passwd));
    memset(bssid, 0, sizeof(bssid));
    memset(token, 0, sizeof(token));

    root = cJSON_Parse(result);
    if (root == NULL) {
        log_goke(DEBUG_WARNING, "cJSON_Parse err");
        send_qr_scan();
        return ret;
    }

    item = cJSON_GetObjectItem(root, "s");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(ssid, json_item_str, sizeof(ssid) - 1);
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            send_qr_scan();
            return ret;
        }
    } else {
        cJSON_Delete(root);
        send_qr_scan();
        return ret;
    }

    item = cJSON_GetObjectItem(root, "p");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(passwd, json_item_str, sizeof(passwd) - 1);
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            send_qr_scan();
            return ret;
        }
    } else {
        cJSON_Delete(root);
        send_qr_scan();
        return ret;
    }

    item = cJSON_GetObjectItem(root, "b");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(bssid, json_item_str, sizeof(bssid) - 1);
            misc_str_hex_2_hex((unsigned char *) bssid, bssid, strlen(bssid));
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            send_qr_scan();
            return ret;
        }
    } else {
        cJSON_Delete(root);
        send_qr_scan();
        return ret;
    }

    item = cJSON_GetObjectItem(root, "t");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(token, json_item_str, sizeof(token) - 1);
            misc_str_hex_2_hex((unsigned char *) token, token, strlen(token));
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            send_qr_scan();
            return ret;
        }
    } else {
        cJSON_Delete(root);
        send_qr_scan();
        return ret;
    }
    //success find all the parameters
//    //connect to iot
//    awss_connect(ssid, passwd, (uint8_t *) bssid, sizeof(bssid) / 2,
//                 (uint8_t *) token, sizeof(token) / 2, TOKEN_TYPE_CLOUD);
    //write the temporary file
    fp = fopen(WIFI_INFO_CONF_TEMP, "w+");
    if (fp == NULL) {
        send_qr_scan();
        log_goke(DEBUG_WARNING, "open wifi_info.conf failed");
    } else {
        fwrite(result, 1, strlen(result), fp);
    }
    fclose(fp);
    //recheck
    fp = fopen(WIFI_INFO_CONF_TEMP, "r");
    if (wifi_configure(fp) == 0) {
        wifi_st = WIFI_STA;
        msg_init(&send_msg);
        send_msg.sender = send_msg.receiver = SERVER_ALIYUN;
        send_msg.arg_in.chick = 1;
        send_msg.message = MSG_ALIYUN_CONNECT;
        global_common_send_message(SERVER_ALIYUN, &send_msg);
    } else {
        wifi_st = WIFI_QRCODE;
        send_qr_scan();
    }
    fclose(fp);
    //
    cJSON_Delete(root);
    return 0;
}

static int wifi_configure(FILE *fp) {
    int ret = -1;
    char *json_str, *json_item_str;
    int length;

    cJSON *root;
    cJSON *item;

    if (fp == NULL) {
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    length = ftell(fp);
    json_str = (char *) malloc(length);
    if (!json_str) {
        log_goke(DEBUG_WARNING, "malloc error");
        return -1;
    }

    fseek(fp, 0, SEEK_SET);
    fread(json_str, 1, length, fp);

    root = cJSON_Parse(json_str);
    if (root == NULL) {
        free(json_str);
        return -1;
    }

    item = cJSON_GetObjectItem(root, "s");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(ssid, json_item_str, sizeof(ssid) - 1);
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            free(json_str);
            return ret;
        }
    } else {
        cJSON_Delete(root);
        free(json_str);
        return ret;
    }

    item = cJSON_GetObjectItem(root, "p");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(passwd, json_item_str, sizeof(passwd) - 1);
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            free(json_str);
            return ret;
        }
    } else {
        cJSON_Delete(root);
        free(json_str);
        return ret;
    }

    item = cJSON_GetObjectItem(root, "b");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(bssid, json_item_str, sizeof(bssid) - 1);
            misc_str_hex_2_hex((unsigned char *) bssid, bssid, strlen(bssid));
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            free(json_str);
        }
    } else {
        cJSON_Delete(root);
        free(json_str);
    }

    item = cJSON_GetObjectItem(root, "t");
    if (item) {
        json_item_str = cJSON_Print(item);
        if (json_item_str) {
            json_del_yinhao(json_item_str);
            strncpy(token, json_item_str, sizeof(token) - 1);
            misc_str_hex_2_hex((unsigned char *) token, token, strlen(token));
            cJSON_free(json_item_str);
        } else {
            cJSON_Delete(root);
            free(json_str);
        }
    } else {
        cJSON_Delete(root);
        free(json_str);
    }

    cJSON_Delete(root);
    free(json_str);
    ret = 0;
    return ret;
}

int wifi_link_ok()
{
	int iRet = 0,iCount = 0;
	char line[128];
	FILE *fp = NULL;
	fp = popen("wpa_cli status","r");
	if(fp)
	{
		while((fgets(line,sizeof(line),fp) != NULL)&&(iCount++ < 100))
		{
			if(strstr(line,"wpa_state=COMPLETED"))
			{
				iRet = 1;
				break;
			}
		}
		pclose(fp);
	}
	return iRet;
}

int wifi_connect(void) {
    char cmd[256];
    FILE *fp;
    int ret = 0;
    if( !need_bind ) {
        return 0;
    }
    log_goke(DEBUG_INFO, "awss_connect:\r");
    log_goke(DEBUG_INFO, "ssid = %s\r", ssid);
    log_goke(DEBUG_INFO, "passwd = %s\r", passwd);
    log_goke(DEBUG_INFO, "bssid: ");
    for (int i = 0; i < sizeof(bssid) / 2; i++) {
        log_goke(DEBUG_INFO, "0x%02x ", bssid[i] & 0xff);
    }
    log_goke(DEBUG_INFO, "\r");
    log_goke(DEBUG_INFO, "token: ");
    for (int i = 0; i < sizeof(token) / 2; i++) {
        log_goke(DEBUG_INFO, "0x%02x ", token[i] & 0xff);
    }
    log_goke(DEBUG_INFO, "\r");
//
    memset(cmd, 0, sizeof(cmd));
    snprintf( cmd, sizeof(cmd), "/usr/sbin/wpa_cli -i wlan0 add_network");
    ret = system(cmd);
    log_goke( DEBUG_INFO, "ret=%d, %s", ret, cmd);
    sleep(1);

    memset(cmd, 0, sizeof(cmd));
    snprintf( cmd, sizeof(cmd), "/usr/sbin/wpa_cli set_network 0 ssid \'\"%s\"\'", ssid);
    ret = system(cmd);
    log_goke( DEBUG_INFO, "ret=%d, %s", ret, cmd);
    sleep(1);

    memset(cmd, 0, sizeof(cmd));
    snprintf( cmd, sizeof(cmd), "/usr/sbin/wpa_cli set_network 0 psk \'\"%s\"\'", passwd);
    ret = system(cmd);
    log_goke( DEBUG_INFO, "ret=%d, %s", ret, cmd);
    sleep(1);

    memset(cmd, 0, sizeof(cmd));
    snprintf( cmd, sizeof(cmd), "/usr/sbin/wpa_cli set_network 0 scan_ssid 1");
    ret = system(cmd);
    log_goke( DEBUG_INFO, "ret=%d, %s", ret, cmd);
    sleep(1);

    memset(cmd, 0, sizeof(cmd));
    snprintf( cmd, sizeof(cmd), "/usr/sbin/wpa_cli set_network 0 priority 1");
    ret = system(cmd);
    log_goke( DEBUG_INFO, "ret=%d, %s", ret, cmd);
    sleep(1);

    memset(cmd, 0, sizeof(cmd));
    snprintf( cmd, sizeof(cmd), "/usr/sbin/wpa_cli -i wlan0 enable_network 0");
    ret = system(cmd);
    log_goke( DEBUG_INFO, "ret=%d, %s", ret, cmd);
    sleep(1);
    return 0;
}

void send_qr_scan(void) {
    message_t send_msg;
    int ret;
    do {
        /****************************/
        msg_init(&send_msg);
        send_msg.sender = send_msg.receiver = SERVER_ALIYUN;
        send_msg.arg_in.cat = SNAP_TYPE_BARCODE;
        ret = server_video_snap_message(&send_msg);
        sleep(1);
    } while (ret == -1);
}


void wifi_init(void) {
    FILE *fp;
    message_t send_msg;
    int ret;
    wifi_st = WIFI_QRCODE;
    fp = fopen(WIFI_INFO_CONF, "r");
    if (fp == NULL) {
        need_bind = 1;
        wifi_st = WIFI_QRCODE;
        send_qr_scan();
        return;
    }
    if (wifi_configure(fp) == 0) {
        wifi_st = WIFI_STA;
        msg_init(&send_msg);
        send_msg.sender = send_msg.receiver = SERVER_ALIYUN;
        send_msg.arg_in.chick = 1;
        send_msg.message = MSG_ALIYUN_CONNECT;
        global_common_send_message(SERVER_ALIYUN, &send_msg);
    } else {
        wifi_st = WIFI_QRCODE;
        send_qr_scan();
    }
    fclose(fp);
}

static void aliyun_broadcast_thread_exit(void) {
}

static void server_release_1(void) {
    linkvisual_client_stop();
    linkkit_client_destroy();
    usleep(1000 * 10);
}

static void server_release_2(void) {
    msg_buffer_release2(&message, &mutex);
}

static void server_release_3(void) {
    msg_free(&info.task.msg);
    memset(&info, 0, sizeof(server_info_t));
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_MANAGER_EXIT_ACK;
    msg.sender = SERVER_ALIYUN;
    global_common_send_message(SERVER_MANAGER, &msg);
    /***************************/
}

static void server_thread_termination(int sign) {
    /********message body********/
    message_t msg;
    msg_init(&msg);
    msg.message = MSG_ALIYUN_SIGINT;
    msg.sender = msg.receiver = SERVER_ALIYUN;
    global_common_send_message(SERVER_MANAGER, &msg);
    /****************************/
}

static int aliyun_event_processor(message_t *msg) {
    int ret = 0;
    return ret;
}

static void init_product(void) {
    FILE *pFile;
    char *json_str, *json_item_str;
    int length;

    cJSON *root;
    cJSON *item;

    pFile = fopen(PRODUCT_CONFIG_NAME, "r");
    if (pFile == NULL) {
        pFile = fopen(PRODUCT_CONFIG_NAME, "w");
        if (pFile == NULL) {
            printf("error can't open product.conf\r");
            return;
        }

        root = cJSON_CreateObject();
        cJSON_AddItemToObject(root, "pk", cJSON_CreateString(product_key));
        cJSON_AddItemToObject(root, "ps", cJSON_CreateString(product_secret));
        cJSON_AddItemToObject(root, "dn", cJSON_CreateString(device_name));
        cJSON_AddItemToObject(root, "ds", cJSON_CreateString(device_secret));
        json_str = cJSON_Print(root);

        fwrite(json_str, 1, strlen(json_str), pFile);

        cJSON_free(json_str);
        cJSON_Delete(root);
    } else {
        fseek(pFile, 0, SEEK_END);
        length = ftell(pFile);
        json_str = (char *) malloc(length);
        if (!json_str) {
            printf("malloc error\r");
            fclose(pFile);
            return;
        }
        fseek(pFile, 0, SEEK_SET);
        fread(json_str, 1, length, pFile);

        root = cJSON_Parse(json_str);
        if (root) {
            item = cJSON_GetObjectItem(root, "pk");
            if (item) {
                json_item_str = cJSON_Print(item);
                json_del_yinhao(json_item_str);
                strncpy(product_key, json_item_str, sizeof(product_key));
                cJSON_free(json_item_str);
            }

            item = cJSON_GetObjectItem(root, "ps");
            if (item) {
                json_item_str = cJSON_Print(item);
                json_del_yinhao(json_item_str);
                strncpy(product_secret, json_item_str, sizeof(product_secret));
                cJSON_free(json_item_str);
            }

            item = cJSON_GetObjectItem(root, "dn");
            if (item) {
                json_item_str = cJSON_Print(item);
                json_del_yinhao(json_item_str);
                strncpy(device_name, json_item_str, sizeof(device_name));
                cJSON_free(json_item_str);
            }

            item = cJSON_GetObjectItem(root, "ds");
            if (item) {
                json_item_str = cJSON_Print(item);
                json_del_yinhao(json_item_str);
                strncpy(device_secret, json_item_str, sizeof(device_secret));
                cJSON_free(json_item_str);
            }
            cJSON_Delete(root);
        }
        free(json_str);
    }

    printf("pk = %s\r", product_key);
    printf("ps = %s\r", product_secret);
    printf("dn = %s\r", device_name);
    printf("ds = %s\r", device_secret);

    fclose(pFile);
}

static int aliyun_get_uuid()
{
	devinfo_t tDevInfo;
	int iRet = -1;
	iRet = config_devinfo_get(&tDevInfo);
	if(iRet == 0)
	{
		if(tDevInfo.iFlag == DEVINFO_FLAG_MAGIC)
		{
			snprintf(product_key,IOTX_PRODUCT_KEY_LEN,tDevInfo.acProductKey);
			snprintf(product_secret,IOTX_DEVICE_SECRET_LEN,tDevInfo.acProductSecret);
			snprintf(device_name,IOTX_DEVICE_NAME_LEN,tDevInfo.acDeviceName);
            if( strcmp(_config_.device_name, device_name) != 0 ) {
                strcpy(_config_.device_name, device_name);
                config_set(&_config_);
            }
			snprintf(device_secret,IOTX_DEVICE_SECRET_LEN,tDevInfo.acDeviceSecret);
		}
		else
		{
			log_goke(DEBUG_SERIOUS,"unFind uuid");
			return -1;
		}
		log_goke(DEBUG_INFO,"product_key:%s\r\nproduct_secret:%s\r\ndevice_name:%s\r\ndevice_secret:%s\r\n",product_key,product_secret,device_name,device_secret);
	}
	return iRet;
}

static int aliyun_message_callback(message_t *msg) {
    int ret = 0;
    if (msg->message == MSG_PLAYER_GET_FILE_BY_DAY_ACK) {
        log_goke(DEBUG_INFO, "========get file list========");
    }
    return ret;
}

static int aliyun_send_player_finish(message_t *msg) {
    int ret = 0;
    lv_stream_send_cmd(msg->arg_in.wolf, LV_STORAGE_RECORD_COMPLETE);
    return ret;
}

static int aliyun_restart(void) {
    char cmd[256];
    int ret;
    log_goke(DEBUG_WARNING,"server_restart");
    linkkit_client_destroy();

    if( need_bind ) {
    //rescan logic
    #ifdef RELEASE_VERSION
        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "/bin/rm -f %s", WIFI_INFO_CONF);
        ret = system(cmd);

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "/bin/rm -f %s", WIFI_INFO_CONF_TEMP);
        ret = system(cmd);

        memset(cmd, 0, sizeof(cmd));
        snprintf(cmd, sizeof(cmd), "cp -f %s %s", WPA_SUPPLICANT_CONF_BAK_NAME, WPA_SUPPLICANT_CONF_NAME);
        system(cmd);
        usleep(100000);

        log_goke(DEBUG_WARNING, "rescaning qrcode ...\r\n");
    #else
        log_goke( DEBUG_WARNING, "---wifi rescan not available in DEBUG version!---");
    #endif
        need_bind = 0;
    }
    info.status = STATUS_SETUP;
    return 0;
}

/*
 *
 * stream function
 *
 *
 */
static void aliyun_yield(void) {
    message_t msg;
    IOT_Linkkit_Yield(10);
    /********message body********/
    msg_init(&msg);
    msg.message = MSG_ALIYUN_YIELD;
    msg.sender = msg.receiver = SERVER_ALIYUN;
    global_common_send_message(SERVER_ALIYUN, &msg);
    /****************************/
}

/*
 *
 * server function
 *
 */
static int server_init(void) {
    message_t msg;
    int ret = 0;
    if (misc_full_bit(info.init_status, ALIYUN_INIT_CONDITION_NUM)) {
        info.status = STATUS_WAIT;
    }
    return ret;
}

static int server_setup(void) {
    //init_product();
    message_t msg;
    if(aliyun_get_uuid() != 0) {
		info.status = STATUS_WAIT; //继续等待setup
    	return 0;
	}
	
    linkvisual_init_client();
    linkkit_init_client(product_key, product_secret, device_name, device_secret);
    wifi_init();
    info.status = STATUS_IDLE; //linkkit等待wifi连接
    return 0;
}

static int server_start(void) {
    int ret;
    static int retry = 0;
    ret = linkkit_client_start(need_bind);
    if (ret >= 0) {
        info.status = STATUS_RUN;
        aliyun_yield();
    } else {
        log_goke(DEBUG_WARNING,"linkkit_client_start failed sleep 5s continue...\r");
        sleep(5);
        retry++;
        if(retry > 4) {
            retry = 0;
            aliyun_restart();
        } else {
            message_t send_msg;
            msg_init(&send_msg);
            send_msg.sender = send_msg.receiver = SERVER_ALIYUN;
            send_msg.message = MSG_MANAGER_DUMMY;
            global_common_send_message( SERVER_ALIYUN, &send_msg);
        }
    }
    return 0;
}

static int server_stop(void) {
	log_goke(DEBUG_INFO,"server_stop");
    return 0;
}

/*
 *
 * message proc
 *
 *
 */
static int aliyun_message_block(void) {
    int ret = 0;
    int id = -1, id1, index = 0;
    message_t msg;
    //search for unblocked message and swap if necessory
    if (!info.msg_lock) {
        log_goke(DEBUG_MAX, "===aliyun message block, return 0 when first message is msg_lock=0");
        return 0;
    }
    index = 0;
    msg_init(&msg);
    ret = msg_buffer_probe_item(&message, index, &msg);
    if (ret) {
        log_goke(DEBUG_MAX, "===aliyun message block, return 0 when first message is empty");
        return 0;
    }
    if (msg_is_system(msg.message) || msg_is_response(msg.message)) {
        log_goke(DEBUG_MAX, "===aliyun message block, return 0 when first message is system or response message %s",
                 global_common_message_to_string(msg.message));
        return 0;
    }
    id = msg.message;
    do {
        index++;
        msg_init(&msg);
        ret = msg_buffer_probe_item(&message, index, &msg);
        if (ret) {
            log_goke(DEBUG_MAX, "===aliyun message block, return 1 when message index = %d is not found!", index);
            return 1;
        }
        if (msg_is_system(msg.message) ||
            msg_is_response(msg.message)) {    //find one behind system or response message
            msg_buffer_swap(&message, 0, index);
            id1 = msg.message;
            log_goke(DEBUG_INFO, "===aliyun: swapped message happend, message %s was swapped with message %s",
                     global_common_message_to_string(id), global_common_message_to_string(id1));
            return 0;
        }
    } while (!ret);
    return ret;
}

static int aliyun_message_filter(message_t *msg) {
    int ret = 0;
    if (info.status >= STATUS_ERROR) { //only system message
        if (!msg_is_system(msg->message) && !msg_is_response(msg->message))
            return 1;
    }
    return ret;
}

static int server_message_proc(void) {
    int ret = 0;
    message_t msg,send_msg;
    char *qrcode;
    //condition
    pthread_mutex_lock(&mutex);
    if (message.head == message.tail) {
        if (info.status == info.old_status) {
            pthread_cond_wait(&cond, &mutex);
        }
    }
    if (aliyun_message_block()) {
        pthread_mutex_unlock(&mutex);
        return 0;
    }
    msg_init(&msg);
    ret = msg_buffer_pop(&message, &msg);
    pthread_mutex_unlock(&mutex);
    if (ret == 1)
        return 0;
    if (aliyun_message_filter(&msg)) {
        log_goke(DEBUG_MAX, "ALIYUN message filtered: sender=%s, message=%s, head=%d, tail=%d was screened",
                 global_common_get_server_name(msg.sender),
                 global_common_message_to_string(msg.message), message.head, message.tail);
        msg_free(&msg);
        return -1;
    }
    log_goke(DEBUG_MAX, "ALIYUN message popped: sender=%s, message=%s, head=%d, tail=%d",
            global_common_get_server_name(msg.sender),
             global_common_message_to_string(msg.message), message.head, message.tail);
    switch (msg.message) {
        case MSG_ALIYUN_CONNECT:
            if (info.status == STATUS_IDLE) {
                info.status = STATUS_START;
            }
            break;
        case MSG_ALIYUN_QRCODE_RESULT:
            get_qrcode_callback(&msg);
            break;
        case MSG_PLAYER_FINISH:
            aliyun_send_player_finish(&msg);
            break;
        case MSG_ALIYUN_EVENT:
            aliyun_event_processor(&msg);
            break;
        case MSG_ALIYUN_RESTART:
            aliyun_restart();
            break;
        case MSG_VIDEO_START_ACK:
        case MSG_VIDEO_STOP_ACK:
        case MSG_AUDIO_START_ACK:
        case MSG_AUDIO_STOP_ACK:
        case MSG_PLAYER_GET_PICTURE_LIST_ACK:
        case MSG_PLAYER_GET_INFOMATION_ACK:
            aliyun_message_callback(&msg);
            break;
        case MSG_VIDEO_PROPERTY_GET_ACK:
        case MSG_VIDEO_PROPERTY_SET_ACK:
        case MSG_VIDEO_PROPERTY_SET_EXT_ACK:
        case MSG_VIDEO_PROPERTY_SET_DIRECT_ACK:
            break;
        case MSG_MANAGER_EXIT:
            msg_init(&info.task.msg);
            msg_copy(&info.task.msg, &msg);
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case MSG_MANAGER_TIMER_ON:
            ((HANDLER) msg.arg_in.handler)();
            break;
        case MSG_MANAGER_EXIT_ACK:
            misc_clear_bit(&info.quit_status, msg.sender);
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "ALIYUN: server %s quit and quit lable now = %s",
                     global_common_get_server_name(msg.sender),
                     fp);
            if (fp) {
                free(fp);
            }
            break;
        case MSG_MANAGER_DUMMY:
            break;
        case MSG_ALIYUN_YIELD:
            aliyun_yield();
            break;
        case MSG_ALIYUN_TIME_SYNCHRONIZED:
            aliyun_time_sync_label = 1;
            break;
        case MSG_ALIYUN_GET_TIME_SYNC:
            /********message body********/
            msg_init(&send_msg);
            send_msg.message = MSG_ALIYUN_GET_TIME_SYNC_ACK;
            send_msg.sender = send_msg.receiver = SERVER_ALIYUN;
            send_msg.arg_in.cat = aliyun_time_sync_label;
            ret = global_common_send_message(msg.sender, &send_msg);
            /***************************/
            break;
        default:
            log_goke(DEBUG_SERIOUS, "ALIYUN not support message %s",
                     global_common_message_to_string(msg.message));
            break;
    }
    msg_free(&msg);
    return ret;
}

/*
 * task proc
 */
static void task_proc(void) {
    if (info.status == STATUS_NONE) {
        info.status = STATUS_INIT;
    } else if (info.status == STATUS_WAIT) {
        info.status = STATUS_SETUP;
    } else if (info.status == STATUS_IDLE) {
    }
    return;
}

/*
 * state machine
 */
static void state_machine(void) {
    switch (info.status) {
        case STATUS_NONE:
        case STATUS_WAIT:
        case STATUS_IDLE:
        case STATUS_RUN:
            break;
        case STATUS_INIT:
            server_init();
            break;
        case STATUS_SETUP:
            server_setup();
            break;
        case STATUS_START:
            server_start();
            break;
        case STATUS_STOP:
            server_stop();
            break;
        case STATUS_RESTART:
            server_restart();
            break;
        case STATUS_ERROR:
            info.status = EXIT_INIT;
            info.msg_lock = 0;
            break;
        case EXIT_INIT:
            log_goke(DEBUG_INFO, "ALIYUN: switch to exit task!");
            if (info.task.msg.sender == SERVER_MANAGER) {
                info.quit_status &= (info.task.msg.arg_in.cat);
            } else {
                info.quit_status = 0;
            }
            char *fp = global_common_get_server_names(info.quit_status);
            log_goke(DEBUG_INFO, "ALIYUN: Exit precondition =%s", fp);
            if (fp) {
                free(fp);
            }
            info.status = EXIT_SERVER;
            break;
        case EXIT_SERVER:
            if (!info.quit_status)
                info.status = EXIT_STAGE1;
            break;
        case EXIT_STAGE1:
            server_release_1();
            info.status = EXIT_THREAD;
            break;
        case EXIT_THREAD:
            info.thread_exit = info.thread_start;
            aliyun_broadcast_thread_exit();
            if (!info.thread_start)
                info.status = EXIT_STAGE2;
            break;
        case EXIT_STAGE2:
            server_release_2();
            info.status = EXIT_FINISH;
            break;
        case EXIT_FINISH:
            info.exit = 1;
            info.status = STATUS_NONE;
            break;
        default:
            log_goke(DEBUG_SERIOUS, "ALIYUN !!!!!!!unprocessed server status in state_machine = %d",
                     info.status);
            break;
    }
    return;
}

/*
 * server entry point
 */
static void *server_func(void) {
    signal(SIGINT, server_thread_termination);
    signal(SIGTERM, server_thread_termination);
    misc_set_thread_name("server_aliyun");
    pthread_detach(pthread_self());
    msg_buffer_init2(&message, manager_config.msg_overrun,
                     manager_config.msg_buffer_size_media, &mutex);
    info.quit_status = ALIYUN_EXIT_CONDITION;
    info.init = 1;
    while (!info.exit) {
        info.old_status = info.status;
        state_machine();
        task_proc();
        server_message_proc();
    }
    server_release_3();
    log_goke(DEBUG_INFO, "-----------thread exit: server_aliyun-----------");
    pthread_exit(0);
}

/*
 * internal interface
 */


/*
 * external interface
 */
int server_aliyun_start(void) {
    int ret = -1;
    memset(&info, 0, sizeof(info)); //初始化内部info
    ret = pthread_create(&info.id, NULL, server_func, NULL);
    if (ret != 0) {
        log_goke(DEBUG_SERIOUS, "aliyun server create error! ret = %d", ret);
        return ret;
    } else {
        log_goke(DEBUG_INFO, "aliyun server create successful!");
        return 0;
    }
}

int server_aliyun_message(message_t *msg) {
    int ret = 0;
    int a = 1;
    a = a + 1;
    pthread_mutex_lock(&mutex);
    if (!message.init) {
        log_goke(DEBUG_MAX, "ALIYUN server is not ready: sender=%s, message=%s",
                 global_common_get_server_name(msg->sender),
                 global_common_message_to_string(msg->message));
        pthread_mutex_unlock(&mutex);
        return -1;
    }
    ret = msg_buffer_push(&message, msg);
    log_goke(DEBUG_MAX, "ALIYUN message insert: sender=%s, message=%s, ret=%d, head=%d, tail=%d",
             global_common_get_server_name(msg->sender),
             global_common_message_to_string(msg->message),
             ret,message.head, message.tail);
    if (ret == MSG_BUFFER_PUSH_FAIL) {
        log_goke(DEBUG_WARNING, "ALIYUN message push in error =%d", ret);
    } else {
        pthread_cond_signal(&cond);
    }
    pthread_mutex_unlock(&mutex);
    return ret;
}
