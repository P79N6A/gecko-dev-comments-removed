



#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <syslog.h>
#include <inttypes.h>



































int
platGenerateCryptoRand(uint8_t *buf, int *len)
{
    int fd;
    int rc = 0;
    ssize_t s;

    if ((fd = open("/dev/urandom", O_RDONLY)) == -1) {
        syslog(LOG_ERR, "Failed to open prng driver");
        return 0;
    }

    





    s = read(fd, buf, (size_t) *len);

    if (s > 0) {
        *len = s;
        rc = 1; 
    } else {
        *len = 0;
        rc = 0; 
    }
        
    (void) close(fd);
    return rc;
}

