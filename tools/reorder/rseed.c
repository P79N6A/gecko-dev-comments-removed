









































#include <stdio.h>
#include <fcntl.h>

int
main(int argc, char *argv[])
{
    int fd, ok, seed = 0;

    fd = open("/dev/random", O_RDONLY);
    if (fd < 0) {
        perror("/dev/random");
        return 1;
    }

    ok = read(fd, &seed, sizeof seed);
    if (ok > 0)
        printf("%d\n", seed);
    else
        perror("/dev/random");

    close(fd);
    return 0;
}
