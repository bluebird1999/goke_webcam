#ifndef __GK_GPIO_APP_H__
#define __GK_GPIO_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GK_GPIO_RESET		9
#define GK_GPIO_IRCUT1		14  //gpio1-6
#define GK_GPIO_IRCUT2		13  //gpio1-5
#define GK_GPIO_LED1		52  //gpio6-4       red
#define GK_GPIO_LED2		53  //gpio6-5       green
#define GK_GPIO_IRLED		54  //gpio6-6
#define GK_GPIO_SPEAKER     4   //gpio0-4


int gk_gpio_init(void);
int gk_gpio_setvalue(int num, int value);
int gk_gpio_getvalue(int num, int *value);
int gk_ircut_daymode();
int gk_ircut_nightmode();
int gk_ircut_release();
int gk_enable_speaker();
int gk_disable_speaker();
int gk_led1_on();
int gk_led1_off();
int gk_led2_on();
int gk_led2_off();

#ifdef __cplusplus
}
#endif
#endif
