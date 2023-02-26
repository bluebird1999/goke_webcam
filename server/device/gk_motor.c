#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <time.h>
#include "gk_motor.h"
#include "gk_gpio.h"
#include "../../common/tools_interface.h"
#include "../../global/global_interface.h"
#include "../../server/manager/manager_interface.h"

static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
static motor_status_t status = MOTOR_STATUS_IDLE;
static int fd = -1;
static int sleep_time = 100000;
struct stPtzCtrlInfo stInfo;

#ifdef RELEASE_VERSION
static char sd_address[32] = "";
#else
static char sd_address[32] = "e624";
#endif

int motor_control( aliyun_ptz_action_t action, int step) {
    pthread_rwlock_wrlock(&lock);
    memset( &stInfo, 0, sizeof(stInfo) );
    if( step <= 0 ) {
        log_goke( DEBUG_WARNING, " motor control parameter error with step = 0");
        pthread_rwlock_unlock(&lock);
        return -1;
    } else {
        if (action == ALIYUN_PTZ_LEFT) {
            stInfo.xMotor.iEnable = 1;
            stInfo.xMotor.iDir = MOTOR_BACKWARD;
            stInfo.xMotor.iSteps = step;
            stInfo.yMotor.iEnable = 0;
        } else if (action == ALIYUN_PTZ_RIGHT ) {
            stInfo.xMotor.iEnable = 1;
            stInfo.xMotor.iDir = MOTOR_FOREWARD;
            stInfo.xMotor.iSteps = step;
            stInfo.yMotor.iEnable = 0;
        } else if (action == ALIYUN_PTZ_DOWN ) {
            stInfo.yMotor.iEnable = 1;
            stInfo.yMotor.iDir = MOTOR_BACKWARD;
            stInfo.yMotor.iSteps = step;
            stInfo.xMotor.iEnable = 0;
        } else if (action == ALIYUN_PTZ_UP ) {
            stInfo.yMotor.iEnable = 1;
            stInfo.yMotor.iDir = MOTOR_FOREWARD;
            stInfo.yMotor.iSteps = step;
            stInfo.xMotor.iEnable = 0;
        } else if( action == ALIYUN_PTZ_LEFT_UP) {
            stInfo.xMotor.iEnable = stInfo.yMotor.iEnable = 1;
            stInfo.xMotor.iSteps = stInfo.yMotor.iSteps = step;
            stInfo.xMotor.iDir = MOTOR_BACKWARD;
            stInfo.yMotor.iDir = MOTOR_FOREWARD;
        } else if( action == ALIYUN_PTZ_RIGHT_UP) {
            stInfo.xMotor.iEnable = stInfo.yMotor.iEnable = 1;
            stInfo.xMotor.iSteps = stInfo.yMotor.iSteps = step;
            stInfo.xMotor.iDir = stInfo.yMotor.iDir = MOTOR_FOREWARD;
        } else if( action == ALIYUN_PTZ_LEFT_DOWN) {
            stInfo.xMotor.iEnable = stInfo.yMotor.iEnable = 1;
            stInfo.xMotor.iSteps = stInfo.yMotor.iSteps = step;
            stInfo.xMotor.iDir = stInfo.yMotor.iDir = MOTOR_BACKWARD;
        } else if( action == ALIYUN_PTZ_RIGHT_DOWN) {
            stInfo.xMotor.iEnable = stInfo.yMotor.iEnable = 1;
            stInfo.xMotor.iSteps = stInfo.yMotor.iSteps = step;
            stInfo.xMotor.iDir = MOTOR_FOREWARD;
            stInfo.yMotor.iDir = MOTOR_BACKWARD;
        } else {
            log_goke( DEBUG_WARNING, " motor control parameter error with unsupported action = %d", action);
            pthread_rwlock_unlock(&lock);
            return -1;
        }
        ioctl(fd, HI_PTZ_IOC_RUN, &stInfo);
    }
    pthread_rwlock_unlock(&lock);
    return 0;
}

int motor_init(void)
{
    fd = open(GK_PTZ_DEVICE, O_RDWR);
    if( fd < 0 ) {
        log_goke( DEBUG_SERIOUS, " motor driver open failed!");
        return -1;
    }
    return 0;
}

int motor_uninit(void)
{
    close(fd);
    return 0 ;
}

int motor_proc(void) {
    int ret = 0;
    char cmd[256];
    struct stPtzCtrlInfo stInfo;

    switch (status) {
        case MOTOR_STATUS_IDLE: { //
//            motor_control( ALIYUN_PTZ_RIGHT_DOWN, 128);
            status = MOTOR_STATUS_RUN;
        }
            break;
        case MOTOR_STATUS_RUN: { //
        }
            break;
        case MOTOR_STATUS_PAUSE: {
        }
            break;
        case MOTOR_STATUS_CALIBRATE: {
        }
            break;
        default: {
            log_goke(DEBUG_WARNING, " unknowing motor status = %d", status);
            break;
        }
    }
    usleep(sleep_time);
    return ret;
}