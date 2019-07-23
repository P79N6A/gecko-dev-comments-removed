










































#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
    unsigned int buf[1024];
    ssize_t cb;

    while ((cb = read(STDIN_FILENO, buf, sizeof buf)) > 0) {
        if (cb % sizeof buf[0])
            fprintf(stderr, "unaligned read\n");

        unsigned int *addr = buf;
        unsigned int *limit = buf + (cb / 4);

        for (; addr < limit; ++addr)
            printf("%x\n", *addr);
    }

    return 0;
}
