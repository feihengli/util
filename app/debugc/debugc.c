#include "../sal_debug.h"


int main(int argc, char** argv)
{
    printf("Usage: %s [filter].\nexample: %s debug\n", argv[0], argv[0]);
    if (argc > 2)
    {
        printf("invalid args.argc=%d\n", argc);
        return 0;
    }

    Message recv;
    while (1)
    {
        if (debug_recv(&recv) < 0)
        {
            break;
        }
        if (argc == 2)
        {
            if (strstr(recv.buffer, argv[1]))
            {
                fprintf(stdout, "%s", recv.buffer);
            }
        }
        else
        {
            fprintf(stdout, "%s", recv.buffer);
        }
    }

    return 0;
}

