









#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "webrtc/base/linuxfdwalk.h"



static int parse_fd(const char *s) {
  if (!*s) {
    
    return -1;
  }
  int val = 0;
  do {
    if (*s < '0' || *s > '9') {
      
      return -1;
    }
    int digit = *s++ - '0';
    val = val * 10 + digit;
  } while (*s);
  return val;
}

int fdwalk(void (*func)(void *, int), void *opaque) {
  DIR *dir = opendir("/proc/self/fd");
  if (!dir) {
    return -1;
  }
  int opendirfd = dirfd(dir);
  int parse_errors = 0;
  struct dirent *ent;
  
  while (errno = 0, (ent = readdir(dir)) != NULL) {
    if (strcmp(ent->d_name, ".") == 0 ||
        strcmp(ent->d_name, "..") == 0) {
      continue;
    }
    
    
    
    int fd = parse_fd(ent->d_name);
    if (fd < 0) {
      parse_errors = 1;
      continue;
    }
    if (fd != opendirfd) {
      (*func)(opaque, fd);
    }
  }
  int saved_errno = errno;
  if (closedir(dir) < 0) {
    if (!saved_errno) {
      
      return -1;
    }
    
  }
  if (saved_errno) {
    errno = saved_errno;
    return -1;
  } else if (parse_errors) {
    errno = EBADF;
    return -1;
  } else {
    return 0;
  }
}
