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
#include <signal.h>

#include "sal_gpio.h"

#define MUXCTRL_OFFSET_GPIO1_0 (0x7c)             //LED red
#define MUXCTRL_OFFSET_GPIO1_4 (0x8c)             //LED green
#define MUXCTRL_OFFSET_GPIO1_5 (0x90)             //LED blue
#define MUXCTRL_OFFSET_GPIO8_0 (0x100)             //IRCUT
#define MUXCTRL_OFFSET_GPIO1_2 (0x84)             //infrared light

static int running = 1;

static void sigint(int dummy)
{
    printf("[%s %d]got int signal,exit!\n", __FUNCTION__, __LINE__);
    running = 0;
}

static void init_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGINT);

    sa.sa_handler = sigint;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = sigint;
    sigaction(SIGINT, &sa, NULL);

    signal(SIGPIPE, SIG_IGN);
}

static void usage(char *path)
{
    printf("Usage: %s [-rgbf]\n", path);
    printf("\t-r\tred light\n");
    printf("\t-g\tgreen light\n");
    printf("\t-b\tblue light\n");
    printf("\t-f\tflicker light\n");
    printf("\t-c\tclose light\n");
}

int main(int argc, char** argv)
{
    init_signals();

    int red = 0;
    int green = 0;
    int blue = 0;
    int flicker = 0;

    int ch = 0;
    while ((ch = getopt(argc, argv, "rgbfhc")) != -1)
    {
        switch (ch)
        {
            case 'r':
                red = 1;
                break;
            case 'g':
                green = 1;
                break;
            case 'b':
                blue = 1;
                break;
            case 'f':
                flicker = 1;
                break;
            case 'c':
                break;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    unsigned int count = 0;
    gpio_init();
    gpio_write(1, 0, 0); //led red off
    gpio_write(1, 4, 0); //led green off
    gpio_write(1, 5, 0); //led blue off

    while (running)
    {
        int value = 1;
        if (flicker)
        {
            value = (count%2 == 0) ? 0 : 1;
        }

        if (red)
        {
            gpio_write(1, 0, value);
        }
        else if (green)
        {
            gpio_write(1, 4, value);
        }
        else if (blue)
        {
            gpio_write(1, 5, value);
        }

        count++;
        usleep(500*1000);
    }


    gpio_write(1, 0, 0); //led red off
    gpio_write(1, 4, 0); //led green off
    gpio_write(1, 5, 0); //led blue off

    return 0;
}





