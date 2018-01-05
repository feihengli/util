#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <assert.h>

#include "../sal_gpio.h"

#define MUXCTRL_OFFSET_GPIO1_6 (0x94)             //button

/*
static void usage(char *path)
{
    printf("Usage: %s [-rgbf]\n", path);
    printf("\t-r\tred light\n");
    printf("\t-g\tgreen light\n");
    printf("\t-b\tblue light\n");
    printf("\t-f\tflicker light\n");
    printf("\t-c\tclose light\n");
}
*/

static int init_sh()
{
    int ret = -1;
    char cmd[256];
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "killall init.sh");
    printf("run cmd: %s\n", cmd);
    ret = system(cmd);
    printf("function system ret: %d\n", ret);

    snprintf(cmd, sizeof(cmd), "init.sh &");
    printf("run cmd: %s\n", cmd);
    ret = system(cmd);
    printf("function system ret: %d\n", ret);

    return 0;
}

int main(int argc, char** argv)
{
    int ret = -1;
    unsigned int count = 0;
    gpio_init();

    if (!access("/etc/wpa_supplicant.conf", F_OK))
    {
        init_sh();
    }

    while (1)
    {
        int value = gpio_read(1, 6);
        if (value == 0)
        {
            count++;
        }
        else
        {
            count = 0;
        }

        if (count >= 1)
        {
            system("rm /etc/wpa_supplicant.conf -rf");
            init_sh();
            sleep(10);
        }

        usleep(500*1000);
    }


    return 0;
}





