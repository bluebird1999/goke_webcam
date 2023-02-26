#include <linux/list.h>
#include <linux/timer.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/init.h>  
#include <linux/module.h>
#include <linux/device.h>  
#include <linux/kernel.h>  
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/delay.h>
#include "hi_ptz.h"


MODULE_AUTHOR("qczx");
MODULE_LICENSE("GPL");
static struct timer_list gk_ptz_timer;
static int timeStart = 0;
static int majorNumber = 0;
/*Class ���ƣ���Ӧ/sys/class/�µ�Ŀ¼����*/
static const char *CLASS_NAME = "hi_ptz_ctrl_class";
/*Device ���ƣ���Ӧ/dev�µ�Ŀ¼����*/
static const char *DEVICE_NAME = "gk_ptz";


#define GPIO_IN				0
#define GPIO_OUT			1
static int iForewardTiming[4] = {3,6,12,9};//��תʱ��
static int iBackwardTiming[4] = {9,12,6,3};//��תʱ��


typedef struct stPtzRun{
	int iRuning;//0:ֹͣ���� 1:��������
	int iFlags;
	int iTimerUse;//0,ֹͣ��ʱ����1������ʱ��
	spinlock_t  spin_lock;
	struct stMotorCtrl xMotor;
	struct stMotorCtrl yMotor;
}stPtzRun;

static char recv_msg[20];
static int g_PtzCnt = 0;
static struct class *hi_ptz_ctrl_class = NULL;
static struct device *hi_ptz_ctrl_device = NULL;
static stPtzRun g_HiPtzRun;

void hi_ptz_running(unsigned long data)
{
	static int xx = 0,yy = 0,x_run = 0,y_run = 0,x_idle = 0,y_idle = 0;
	int iValueX[4]={0,0,0,0},iValueY[4]={0,0,0,0};
	int *piTimingX = NULL,*piTimingY = NULL;//ʱ��;//ʱ��
	spin_lock_irqsave(&g_HiPtzRun.spin_lock,g_HiPtzRun.iFlags);
	if(g_HiPtzRun.xMotor.iSteps <= 0)
	{
		g_HiPtzRun.xMotor.iEnable = 0;
	}
	
	if(g_HiPtzRun.yMotor.iSteps <= 0)
	{
		g_HiPtzRun.yMotor.iEnable = 0;
	}
	
	if((g_HiPtzRun.xMotor.iEnable == 0) && (g_HiPtzRun.yMotor.iEnable == 0))
	{
		g_HiPtzRun.iRuning = 0;
	}
	
	if(g_HiPtzRun.iRuning)
	{
		if(g_HiPtzRun.xMotor.iEnable)
		{
			if(g_HiPtzRun.xMotor.iDir == MOTOR_FOREWARD)
			{
				piTimingX = iForewardTiming;
			}
			else
			{
				piTimingX = iBackwardTiming;
			}
			iValueX[0] = !!(piTimingX[xx] & 0x01);
			iValueX[1] = !!(piTimingX[xx] & 0x02);
			iValueX[2] = !!(piTimingX[xx] & 0x04);
			iValueX[3] = !!(piTimingX[xx] & 0x08);
			if(xx++ >= 3) {xx = 0;if(g_HiPtzRun.xMotor.iSteps)g_HiPtzRun.xMotor.iSteps--;}
			x_run = 1;
		}
		else
		{
			x_run = 0;
			xx = 0;
		}
		if(g_HiPtzRun.yMotor.iEnable)
		{
			if(g_HiPtzRun.yMotor.iDir == MOTOR_FOREWARD)
			{
				piTimingY = iForewardTiming;
			}
			else
			{
				piTimingY = iBackwardTiming;
			}
			iValueY[0] = !!(piTimingY[yy] & 0x01);
			iValueY[1] = !!(piTimingY[yy] & 0x02);
			iValueY[2] = !!(piTimingY[yy] & 0x04);
			iValueY[3] = !!(piTimingY[yy] & 0x08);
			if(yy++ >= 3) {yy = 0;if(g_HiPtzRun.yMotor.iSteps)g_HiPtzRun.yMotor.iSteps--;}
			y_run = 1;
		}
		else
		{
			y_run = 0;
			yy = 0;
		}
		mod_timer(&gk_ptz_timer,jiffies);
		//printk(KERN_INFO "Xsteps:%d,Ysteps:%d\r\n",g_HiPtzRun.xMotor.iSteps,g_HiPtzRun.yMotor.iSteps);
	}
	else
	{
		xx = 0;
		yy = 0;
		x_run = 0;
		y_run = 0;
		g_HiPtzRun.iTimerUse = 0;
		g_HiPtzRun.xMotor.iEnable = 0;
		g_HiPtzRun.yMotor.iEnable = 0;
		del_timer_sync(&gk_ptz_timer);
		printk(KERN_INFO "Stop\r\n");
	}
	spin_unlock_irqrestore(&g_HiPtzRun.spin_lock,g_HiPtzRun.iFlags);
	
	if(x_run)
	{
		gpio_set_value(X_MOTOR_GPIO1,iValueX[0]);
		gpio_set_value(X_MOTOR_GPIO2,iValueX[1]);
		gpio_set_value(X_MOTOR_GPIO3,iValueX[2]);
		gpio_set_value(X_MOTOR_GPIO4,iValueX[3]);
		//printk(KERN_INFO "Xptz:%d%d%d%d!\n",iValueX[3],iValueX[2],iValueX[1],iValueX[0]);
		x_idle = 0;
	}
	else
	{
		if(x_idle == 0)
		{
			gpio_set_value(X_MOTOR_GPIO1,0);
			gpio_set_value(X_MOTOR_GPIO2,0);
			gpio_set_value(X_MOTOR_GPIO3,0);
			gpio_set_value(X_MOTOR_GPIO4,0);
			//printk(KERN_INFO "x motor stop\r\n");
		}
		x_idle = 1;
	}
	if(y_run)
	{
		gpio_set_value(Y_MOTOR_GPIO1,iValueY[0]);
		gpio_set_value(Y_MOTOR_GPIO2,iValueY[1]);
		gpio_set_value(Y_MOTOR_GPIO3,iValueY[2]);
		gpio_set_value(Y_MOTOR_GPIO4,iValueY[3]);
		y_idle = 0;
		//printk(KERN_INFO "Yptz:%d%d%d%d!\n",iValueY[3],iValueY[2],iValueY[1],iValueY[0]);
	}
	else
	{
		if(y_idle == 0)
		{
			gpio_set_value(Y_MOTOR_GPIO1,0);
			gpio_set_value(Y_MOTOR_GPIO2,0);
			gpio_set_value(Y_MOTOR_GPIO3,0);
			gpio_set_value(Y_MOTOR_GPIO4,0);
			//printk(KERN_INFO "y motor stop\r\n");
		}
		y_idle = 1;
	}
}

void hi_gpio_init(void)
{
	int ii = 0;
	int iArrayX[4] = {X_MOTOR_GPIO1,X_MOTOR_GPIO2,X_MOTOR_GPIO3,X_MOTOR_GPIO4};
	int iArrayY[4] = {Y_MOTOR_GPIO1,Y_MOTOR_GPIO2,Y_MOTOR_GPIO3,Y_MOTOR_GPIO4};
	for(ii = 0;ii < 4;ii++)
	{//X motor
		gpio_request(iArrayX[ii],NULL);
		gpio_direction_output(iArrayX[ii],GPIO_OUT);
		gpio_set_value(iArrayX[ii],0);
	}
	
	for(ii = 0;ii < 4;ii++)
	{//Y motor
		gpio_request(iArrayY[ii],NULL);
		gpio_direction_output(iArrayY[ii],GPIO_OUT);
		gpio_set_value(iArrayY[ii],0);
	}
	
	//��ʼ����ʱ��
	gk_ptz_timer.expires=jiffies; /*��λ�ǽ���*/
    gk_ptz_timer.function=hi_ptz_running;
    gk_ptz_timer.data=0;
    init_timer(&gk_ptz_timer);
}

void hi_gpio_release(void)
{
	int ii = 0;
	int iArrayX[4] = {X_MOTOR_GPIO1,X_MOTOR_GPIO2,X_MOTOR_GPIO3,X_MOTOR_GPIO4};
	int iArrayY[4] = {Y_MOTOR_GPIO1,Y_MOTOR_GPIO2,Y_MOTOR_GPIO3,Y_MOTOR_GPIO4};
	for(ii = 0;ii < 4;ii++)
	{
		gpio_free(iArrayX[ii]);
		gpio_free(iArrayY[ii]);
	}
}



static int hi_ptz_open(struct inode *node, struct file *file)
{
	return 0;
}

static ssize_t hi_ptz_write(struct file *file,const char *buf,size_t len,loff_t *offset)
{
    int cnt = copy_from_user(recv_msg,buf,len);
    if(0 == cnt)
	{
        if(0 == memcmp(recv_msg,"on",2))
        {
			if(0 == timeStart)
			{
				add_timer(&gk_ptz_timer);
				timeStart = 1;
				g_PtzCnt = 128;
				printk(KERN_INFO "timer start!!\n");
			}
        }
        else
        {
			if(timeStart)
			{
				timeStart = 0;
				g_PtzCnt = 0;
			}
            printk(KERN_INFO "ptz OFF!\n");
			gpio_set_value(X_MOTOR_GPIO1,0);
			gpio_set_value(X_MOTOR_GPIO2,0);
			gpio_set_value(X_MOTOR_GPIO3,0);
			gpio_set_value(X_MOTOR_GPIO4,0);
			
        }
    }
    else{
        printk(KERN_ALERT "ERROR occur when writing!!\n");
        return -EFAULT;
    }
    return len;
}

static long hi_ptz_ctrl_ioctl(struct file *filp,unsigned int cmd, unsigned long arg)
{
	int iRet;
	struct stPtzCtrlInfo stInfo;
	spin_lock_irqsave(&g_HiPtzRun.spin_lock,g_HiPtzRun.iFlags);
	switch (cmd)
	{
		case HI_PTZ_IOC_RUN:
		{
			//printk(KERN_INFO "HI_PTZ_IOC_RUN\r\n");
			if ((void *)arg != 0)
			iRet = copy_from_user(&stInfo, (void __user *)arg,
				sizeof(struct stPtzCtrlInfo));
			if(iRet != 0){break;}
			if(stInfo.xMotor.iEnable)
			{
				memcpy(&g_HiPtzRun.xMotor,&stInfo.xMotor,sizeof(struct stMotorCtrl));
				g_HiPtzRun.iRuning = 1;
			}
			if(stInfo.yMotor.iEnable)
			{
				memcpy(&g_HiPtzRun.yMotor,&stInfo.yMotor,sizeof(struct stMotorCtrl));
				g_HiPtzRun.iRuning = 1;
			}
		}
		break;
		case HI_PTZ_IOC_STOP:
		{
			//printk(KERN_INFO "HI_PTZ_IOC_STOP\r\n");
			g_HiPtzRun.iRuning = 0;
			memset(&g_HiPtzRun.xMotor,0,sizeof(struct stMotorCtrl));
			memset(&g_HiPtzRun.yMotor,0,sizeof(struct stMotorCtrl));
		}
		break;
		default:
		break;
	}

	if(!g_HiPtzRun.iTimerUse)
	{
		add_timer(&gk_ptz_timer);
		g_HiPtzRun.iTimerUse = 1;
	}
	
	spin_unlock_irqrestore(&g_HiPtzRun.spin_lock,g_HiPtzRun.iFlags);

	return 0;
}

static int hi_ptz_close(struct inode *node,struct file *file)
{
	return 0;
}


/*File opertion �ṹ�壬����ͨ������ṹ�彨��Ӧ�ó����ں�֮�������ӳ��*/
static struct file_operations file_oprts = 
{
    .open = hi_ptz_open,
    .write = hi_ptz_write,
	.unlocked_ioctl = hi_ptz_ctrl_ioctl,
    .release = hi_ptz_close,
};


static int __init hi_ptz_init(void)
{
    /*ע��һ���µ��ַ��豸���������豸��*/
    majorNumber = register_chrdev(0,DEVICE_NAME,&file_oprts);
    if(majorNumber < 0 ){
        printk(KERN_ALERT "Register failed!!\r\n");
        return majorNumber;
    }
    printk(KERN_ALERT "Registe success,major number is %d\r\n",majorNumber);

    /*��CLASS_NAME����һ��class�ṹ���������������/sys/classĿ¼����һ����ΪCLASS_NAME��Ŀ¼*/
    hi_ptz_ctrl_class = class_create(THIS_MODULE,CLASS_NAME);
    if(IS_ERR(hi_ptz_ctrl_class))
    {
        unregister_chrdev(majorNumber,DEVICE_NAME);
        return PTR_ERR(hi_ptz_ctrl_class);
    }

    /*��DEVICE_NAMEΪ�����ο�/sys/class/CLASS_NAME��/devĿ¼�´���һ���豸��/dev/DEVICE_NAME*/
    hi_ptz_ctrl_device = device_create(hi_ptz_ctrl_class,NULL,MKDEV(majorNumber,0),NULL,DEVICE_NAME);
    if(IS_ERR(hi_ptz_ctrl_device))
    {
        class_destroy(hi_ptz_ctrl_class);
        unregister_chrdev(majorNumber,DEVICE_NAME);
        return PTR_ERR(hi_ptz_ctrl_device);
    }
	memset(&g_HiPtzRun,0,sizeof(stPtzRun));
	hi_gpio_init();
	spin_lock_init(&g_HiPtzRun.spin_lock);
    return 0;
}

/*����ע���������Դ��ж��ģ�飬���Ǳ���linux�ں��ȶ�����Ҫһ��*/
static void __exit hi_ptz_exit(void)
{
    device_destroy(hi_ptz_ctrl_class,MKDEV(majorNumber,0));
    class_unregister(hi_ptz_ctrl_class);
    class_destroy(hi_ptz_ctrl_class);
    unregister_chrdev(majorNumber,DEVICE_NAME);
	hi_gpio_release();
}


module_init(hi_ptz_init);
module_exit(hi_ptz_exit);
