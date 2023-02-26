#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <time.h>
#include "hi_ptz.h"

#define HI_PTZ_DEVICE		"/dev/gk_ptz"

int main()
{
	int fd = -1,iCnt = 100;
	int time_begin = 0,time_end = 0;
	fd = open(HI_PTZ_DEVICE,O_RDWR);
	time_begin = time(NULL);
	struct stPtzCtrlInfo stInfo;
	stInfo.iOption = MOTOR_SELECT_XY;
	stInfo.xMotor.iEnable = 1;
	stInfo.xMotor.iDir = MOTOR_FOREWARD;
	stInfo.xMotor.iSteps = 128;	
	stInfo.yMotor.iEnable = 1;
	stInfo.yMotor.iDir = MOTOR_FOREWARD;
	stInfo.yMotor.iSteps = 64;	
	ioctl(fd, HI_PTZ_IOC_RUN, &stInfo);
	sleep(1);
	stInfo.xMotor.iDir = MOTOR_BACKWARD;
	stInfo.yMotor.iDir = MOTOR_BACKWARD;
	ioctl(fd, HI_PTZ_IOC_RUN, &stInfo);
	//sleep(1);
	//ioctl(fd, HI_PTZ_IOC_STOP,NULL);
	time_end = time(NULL);
	printf("use time:%d\r\n",time_end-time_begin);
	close(fd);
	return 0;
}
