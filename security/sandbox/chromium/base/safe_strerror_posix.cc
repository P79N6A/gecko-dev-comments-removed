



#if defined(__ANDROID__)




#undef _GNU_SOURCE
#endif

#include "build/build_config.h"
#include "base/safe_strerror_posix.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#define USE_HISTORICAL_STRERRO_R (defined(__GLIBC__) || defined(OS_NACL))

#if USE_HISTORICAL_STRERRO_R && defined(__GNUC__)



#define POSSIBLY_UNUSED __attribute__((unused))
#else
#define POSSIBLY_UNUSED
#endif

#if USE_HISTORICAL_STRERRO_R



static void POSSIBLY_UNUSED wrap_posix_strerror_r(
    char *(*strerror_r_ptr)(int, char *, size_t),
    int err,
    char *buf,
    size_t len) {
  
  char *rc = (*strerror_r_ptr)(err, buf, len);
  if (rc != buf) {
    
    
    buf[0] = '\0';
    strncat(buf, rc, len - 1);
  }
  
  
}
#endif  






static void POSSIBLY_UNUSED wrap_posix_strerror_r(
    int (*strerror_r_ptr)(int, char *, size_t),
    int err,
    char *buf,
    size_t len) {
  int old_errno = errno;
  
  
  
  
  
  int result = (*strerror_r_ptr)(err, buf, len);
  if (result == 0) {
    
    
    
    
    buf[len - 1] = '\0';
  } else {
    
    
    
    
    
    int strerror_error;  
    int new_errno = errno;
    if (new_errno != old_errno) {
      
      
      strerror_error = new_errno;
    } else {
      
      
      strerror_error = result;
    }
    
    snprintf(buf,
             len,
             "Error %d while retrieving error %d",
             strerror_error,
             err);
  }
  errno = old_errno;
}

void safe_strerror_r(int err, char *buf, size_t len) {
  if (buf == NULL || len <= 0) {
    return;
  }
  
  
  
  
  wrap_posix_strerror_r(&strerror_r, err, buf, len);
}

std::string safe_strerror(int err) {
  const int buffer_size = 256;
  char buf[buffer_size];
  safe_strerror_r(err, buf, sizeof(buf));
  return std::string(buf);
}
