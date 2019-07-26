




























#include "client/linux/log/log.h"

#if defined(__ANDROID__)
#include <android/log.h>
#else
#include "third_party/lss/linux_syscall_support.h"
#endif

namespace logger {

int write(const char* buf, size_t nbytes) {
#if defined(__ANDROID__)
  return __android_log_write(ANDROID_LOG_WARN, "google-breakpad", buf);
#else
  return sys_write(2, buf, nbytes);
#endif
}

}  
