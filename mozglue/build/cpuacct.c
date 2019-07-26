


























#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cpuacct.h"

int cpuacct_add(uid_t uid)
{
    int count;
    int fd;
    char buf[80];

    count = snprintf(buf, sizeof(buf), "/acct/uid/%d/tasks", uid);
    fd = open(buf, O_RDWR|O_CREAT|O_TRUNC|O_SYNC);
    if (fd < 0) {
        
        buf[count - sizeof("tasks")] = 0;
        if (mkdir(buf, 0775) < 0)
            return -errno;

        
        buf[count - sizeof("tasks")] = '/';
        fd = open(buf, O_RDWR|O_CREAT|O_TRUNC|O_SYNC);
    }
    if (fd < 0)
        return -errno;

    write(fd, "0", 2);
    if (close(fd))
        return -errno;

    return 0;
}
