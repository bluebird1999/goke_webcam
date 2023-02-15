#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "gk_gpio.h"

typedef struct gk_gpio_map{
	int iNum;//GPIO口
	int iDefValue;//默认值，输入不关注默认值
	int iDirection;//方向,0:输入 1:输出
}gk_gpio_map;


gk_gpio_map stGpioDef[] = {
	{GK_GPIO_RESET,0,0},//reset
	{GK_GPIO_IRCUT1,0,1},//ircut 1
	{GK_GPIO_IRCUT2,0,1},//ircut 2
	{GK_GPIO_LED1,0,1},//LED1
	{GK_GPIO_LED2,0,1},//LED2
	{GK_GPIO_IRLED,0,1},//IR LED
    {GK_GPIO_SPEAKER,0,1},//speaker
	{56,0,1},//motor M2B-
	{57,0,1},//motor M2B+
	{58,0,1},//motor M2A-
	{59,0,1},//motor M2A+
	{68,0,1},//motor M1B-
	{69,0,1},//motor M1A-
	{70,0,1},//motor M1B+
	{71,0,1}//motor M1A+
};

int gk_gpio_export(int num)
{
	FILE *fp = NULL;
    char temp[128] = {0};
	sprintf(temp,"/sys/class/gpio/gpio%d/value",num);
	if(access(temp,F_OK) == 0)
	{
		return 0;
	}
	else
	{
		fp = fopen("/sys/class/gpio/export", "w");
		if (NULL == fp)
		{
			printf("fail to export gpio%d!\r\n",num);
			return -1;
		}
		sprintf(temp,"%d",num);
		fwrite(temp,1,strlen(temp),fp);
		fclose(fp);
	}
    return 0;
}

int gk_gpio_setdir(int num,char *pdir)
{
	FILE *fp = NULL;
    char temp[128] = {0};

    if(num < 0 || !pdir)
    {
		printf("[%s:%d]param error\r\n",__FUNCTION__,__LINE__);
		return -1;
    }

    sprintf(temp,"/sys/class/gpio/gpio%d/direction", num);

    fp = fopen(temp, "w");
    if (NULL == fp)
    {
        printf( "[%s:%d]%s open fail\r\n",__FUNCTION__,__LINE__,temp);
        return -1;
    }

    fwrite(pdir,1,strlen(pdir), fp);

    fclose(fp);

    return 0;
}

int gk_gpio_setvalue(int num, int value)
{
    FILE* fp = NULL;
    char acValue[8] = {0};
    char temp[128] = {0};

    if(gk_gpio_export(num) < 0)
    {
        printf("gk_gpio_export %d error!\r\n", num);
        return -1;
    }

    sprintf(acValue, "%d", !!value);
    snprintf(temp, sizeof(temp), "/sys/class/gpio/gpio%d/value", num);

    fp = fopen(temp, "wb");
    if (fp == NULL)
    {
        printf("fail to open value device!%s",temp);
        return -1;
    }

    fwrite(acValue, 1, 1, fp);

    fclose(fp);

    return 0;
}

int gk_gpio_getvalue(int num, int *value)
{
    FILE *fp = NULL;
    char acValue[16] = {0};
    char temp[128] = {0};
    int len = 0;
	if(value == NULL)
	{
        printf("[%s:%d]param error\r\n",__FUNCTION__,__LINE__);
        return -1;
    }
    if(gk_gpio_export(num) < 0)
    {
        printf("gk_gpio_export %d error!\r\n", num);
        return -1;
    }

    sprintf(temp, "/sys/class/gpio/gpio%d/value", num);

    fp = fopen(temp, "rb");
    if (fp == NULL)
    {
        printf("gk_gpio_getvalue: fail to open!\r\n");
        return -1;
    }

    fseek(fp, 0, SEEK_END); //文件指针转到文件末尾
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
	memset(acValue,0,sizeof(acValue));
    fread(acValue, 1, len, fp);
    fclose(fp);
	*value = atoi(acValue);
	
    return 0;
}

int gk_ircut_daymode()
{
	gk_gpio_setvalue(GK_GPIO_IRCUT1,1);
	gk_gpio_setvalue(GK_GPIO_IRCUT2,0);
    gk_gpio_setvalue(GK_GPIO_IRLED, 0);
	return 0;
}

int gk_ircut_nightmode()
{
	gk_gpio_setvalue(GK_GPIO_IRCUT1,0);
	gk_gpio_setvalue(GK_GPIO_IRCUT2,1);
    gk_gpio_setvalue(GK_GPIO_IRLED, 1);
	return 0;
}

int gk_ircut_release()
{
	gk_gpio_setvalue(GK_GPIO_IRCUT1,0);
	gk_gpio_setvalue(GK_GPIO_IRCUT2,0);
	return 0;
}

int gk_enable_speaker()
{
    gk_gpio_setvalue(GK_GPIO_SPEAKER,0);
    return 0;
}

int gk_disable_speaker()
{
    gk_gpio_setvalue(GK_GPIO_SPEAKER,1);
    return 0;
}

int gk_led1_on()
{
    gk_gpio_setvalue(GK_GPIO_LED1,1);
    return 0;
}

int gk_led1_off()
{
    gk_gpio_setvalue(GK_GPIO_LED1,0);
    return 0;
}

int gk_led2_on()
{
    gk_gpio_setvalue(GK_GPIO_LED2,1);
    return 0;
}

int gk_led2_off()
{
    gk_gpio_setvalue(GK_GPIO_LED2,0);
    return 0;
}

int gk_gpio_init(void)
{
	int iGpioNum = 0,ii = 0;
	iGpioNum = sizeof(stGpioDef)/sizeof(gk_gpio_map);
	for(ii=0;ii<iGpioNum;ii++)
	{
		gk_gpio_export(stGpioDef[ii].iNum);
		if(stGpioDef[ii].iDirection == 0)
		{//输入
			gk_gpio_setdir(stGpioDef[ii].iNum,"in");
		}
		else
		{
			gk_gpio_setdir(stGpioDef[ii].iNum,"out");
			gk_gpio_setvalue(stGpioDef[ii].iNum,stGpioDef[ii].iDefValue);
		}
	}
	return 0;
}
