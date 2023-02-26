#ifndef SERVER_DEVICE_GOKE_MOTOR_H_
#define SERVER_DEVICE_GOKE_MOTOR_H_

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

#define GK_PTZ_DEVICE		"/dev/gk_ptz"

typedef enum aliyun_ptz_action_t {
    ALIYUN_PTZ_LEFT = 0,
    ALIYUN_PTZ_RIGHT,
    ALIYUN_PTZ_UP,
    ALIYUN_PTZ_DOWN,
    ALIYUN_PTZ_LEFT_UP,
    ALIYUN_PTZ_RIGHT_UP,
    ALIYUN_PTZ_LEFT_DOWN,
    ALIYUN_PTZ_RIGHT_DOWN,
    ALIYUN_PTZ_ZOOM_IN = 8,
    ALIYUN_PTZ_ZOOM_OUT,
} aliyun_ptz_action_t;

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


typedef enum motor_status_t {
    MOTOR_STATUS_IDLE = 0,  //before start
    MOTOR_STATUS_RUN,      //init
    MOTOR_STATUS_PAUSE,       //working status
    MOTOR_STATUS_CALIBRATE,//calbration
    MOTOR_STATUS_BUTT,
} motor_status_t;

int motor_proc();
int motor_control( aliyun_ptz_action_t action, int step);
int motor_init(void);
int motor_uninit(void);

#endif
