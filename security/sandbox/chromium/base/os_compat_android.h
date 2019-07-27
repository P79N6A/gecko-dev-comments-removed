



#ifndef BASE_OS_COMPAT_ANDROID_H_
#define BASE_OS_COMPAT_ANDROID_H_

#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>


extern "C" int futimes(int fd, const struct timeval tv[2]);


extern "C" char* mkdtemp(char* path);


extern "C" time_t timegm(struct tm* const t);


#define F_LOCK LOCK_EX
#define F_ULOCK LOCK_UN
inline int lockf(int fd, int cmd, off_t ignored_len) {
  return flock(fd, cmd);
}

#endif  
