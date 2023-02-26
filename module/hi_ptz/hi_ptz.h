#ifndef __HI_PTZ_H__
#define __HI_PTZ_H__

#define X_MOTOR_GPIO1		71
#define X_MOTOR_GPIO2		69
#define X_MOTOR_GPIO3		70
#define X_MOTOR_GPIO4		68

#define Y_MOTOR_GPIO1		59
#define Y_MOTOR_GPIO2		58
#define Y_MOTOR_GPIO3		57
#define Y_MOTOR_GPIO4		56

#define MOTOR_FOREWARD		1//正转
#define MOTOR_BACKWARD		2//反转

struct stMotorCtrl {
	int iEnable;//使能，0停止，1使能
	int iDir;//移动反向，正转和反转
	int iSteps;//步长，范围为1-100
};

struct stPtzCtrlInfo {
	struct stMotorCtrl xMotor;
	struct stMotorCtrl yMotor;
};

typedef enum
{
	HI_PTZ_IOC_BASE =0xABCD0000,
	HI_PTZ_IOC_RUN,
	HI_PTZ_IOC_STOP,
}HI_PTZ_IOC_TYPE;
#endif
