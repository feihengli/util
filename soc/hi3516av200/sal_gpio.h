#ifndef __SAL_GPIO_H__
#define __SAL_GPIO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif

int gpio_init();
unsigned int gpio_read(int group, int channel);
unsigned int gpio_write(int group, int channel, int value);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif



