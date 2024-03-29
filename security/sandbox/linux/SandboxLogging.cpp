





#include "SandboxLogging.h"

#ifdef ANDROID
#include <android/log.h>
#else
#include <algorithm>
#include <stdio.h>
#include <sys/uio.h>
#include <unistd.h>
#endif

#include "base/posix/eintr_wrapper.h"

namespace mozilla {

#ifndef ANDROID



static void
IOVecDrop(struct iovec* iov, int iovcnt, size_t toDrop)
{
  while (toDrop > 0 && iovcnt > 0) {
    size_t toDropHere = std::min(toDrop, iov->iov_len);
    iov->iov_base = static_cast<char*>(iov->iov_base) + toDropHere;
    iov->iov_len -= toDropHere;
    toDrop -= toDropHere;
    ++iov;
    --iovcnt;
  }
}
#endif

void
SandboxLogError(const char* message)
{
#ifdef ANDROID
  
  __android_log_write(ANDROID_LOG_ERROR, "Sandbox", message);
#else
  static const char logPrefix[] = "Sandbox: ", logSuffix[] = "\n";
  struct iovec iovs[3] = {
    { const_cast<char*>(logPrefix), sizeof(logPrefix) - 1 },
    { const_cast<char*>(message), strlen(message) },
    { const_cast<char*>(logSuffix), sizeof(logSuffix) - 1 },
  };
  while (iovs[2].iov_len > 0) {
    ssize_t written = HANDLE_EINTR(writev(STDERR_FILENO, iovs, 3));
    if (written <= 0) {
      break;
    }
    IOVecDrop(iovs, 3, static_cast<size_t>(written));
  }
#endif
}

}
