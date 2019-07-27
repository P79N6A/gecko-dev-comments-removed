





#include "SandboxChroot.h"

#include "SandboxLogging.h"
#include "LinuxCapabilities.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "base/posix/eintr_wrapper.h"
#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/NullPtr.h"

#define MOZ_ALWAYS_ZERO(e) MOZ_ALWAYS_TRUE((e) == 0)

namespace mozilla {

SandboxChroot::SandboxChroot()
{
  pthread_mutexattr_t attr;
  MOZ_ALWAYS_ZERO(pthread_mutexattr_init(&attr));
  MOZ_ALWAYS_ZERO(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK));
  MOZ_ALWAYS_ZERO(pthread_mutex_init(&mMutex, &attr));
  MOZ_ALWAYS_ZERO(pthread_cond_init(&mWakeup, nullptr));
  mCommand = NO_THREAD;
}

SandboxChroot::~SandboxChroot()
{
  SendCommand(JUST_EXIT);
  MOZ_ALWAYS_ZERO(pthread_mutex_destroy(&mMutex));
  MOZ_ALWAYS_ZERO(pthread_cond_destroy(&mWakeup));
}

bool
SandboxChroot::SendCommand(Command aComm)
{
  MOZ_ALWAYS_ZERO(pthread_mutex_lock(&mMutex));
  if (mCommand == NO_THREAD) {
    MOZ_ALWAYS_ZERO(pthread_mutex_unlock(&mMutex));
    return false;
  } else {
    MOZ_ASSERT(mCommand == NO_COMMAND);
    mCommand = aComm;
    MOZ_ALWAYS_ZERO(pthread_mutex_unlock(&mMutex));
    MOZ_ALWAYS_ZERO(pthread_cond_signal(&mWakeup));
    void *retval;
    if (pthread_join(mThread, &retval) != 0 || retval != nullptr) {
      MOZ_CRASH("Failed to stop privileged chroot thread");
    }
    MOZ_ASSERT(mCommand == NO_THREAD);
  }
  return true;
}

static void
AlwaysClose(int fd)
{
  if (IGNORE_EINTR(close(fd)) != 0) {
    SANDBOX_LOG_ERROR("close: %s", strerror(errno));
    MOZ_CRASH("failed to close()");
  }
}

static int
OpenDeletedDirectory()
{
  
  
  
  
  char path[] = "/tmp/mozsandbox.XXXXXX";
  if (!mkdtemp(path)) {
    SANDBOX_LOG_ERROR("mkdtemp: %s", strerror(errno));
    return -1;
  }
  int fd = HANDLE_EINTR(open(path, O_RDONLY | O_DIRECTORY));
  if (fd < 0) {
    SANDBOX_LOG_ERROR("open %s: %s", path, strerror(errno));
    
    DebugOnly<bool> ok = HANDLE_EINTR(rmdir(path)) == 0;
    MOZ_ASSERT(ok);
    return -1;
  }
  if (HANDLE_EINTR(rmdir(path)) != 0) {
    SANDBOX_LOG_ERROR("rmdir %s: %s", path, strerror(errno));
    AlwaysClose(fd);
    return -1;
  }
  return fd;
}

bool
SandboxChroot::Prepare()
{
  LinuxCapabilities caps;
  if (!caps.GetCurrent() || !caps.Effective(CAP_SYS_CHROOT)) {
    SANDBOX_LOG_ERROR("don't have permission to chroot");
    return false;
  }
  mFd = OpenDeletedDirectory();
  if (mFd < 0) {
    SANDBOX_LOG_ERROR("failed to create empty directory for chroot");
    return false;
  }
  MOZ_ALWAYS_ZERO(pthread_mutex_lock(&mMutex));
  MOZ_ASSERT(mCommand == NO_THREAD);
  if (pthread_create(&mThread, nullptr, StaticThreadMain, this) != 0) {
    MOZ_ALWAYS_ZERO(pthread_mutex_unlock(&mMutex));
    SANDBOX_LOG_ERROR("pthread_create: %s", strerror(errno));
    return false;
  }
  while (mCommand != NO_COMMAND) {
    MOZ_ASSERT(mCommand == NO_THREAD);
    MOZ_ALWAYS_ZERO(pthread_cond_wait(&mWakeup, &mMutex));
  }
  MOZ_ALWAYS_ZERO(pthread_mutex_unlock(&mMutex));
  return true;
}

void
SandboxChroot::Invoke()
{
  MOZ_ALWAYS_TRUE(SendCommand(DO_CHROOT));
}

static bool
ChrootToFileDesc(int fd)
{
  if (fchdir(fd) != 0) {
    SANDBOX_LOG_ERROR("fchdir: %s", strerror(errno));
    return false;
  }
  if (chroot(".") != 0) {
    SANDBOX_LOG_ERROR("chroot: %s", strerror(errno));
    return false;
  }
  return true;
}

 void*
SandboxChroot::StaticThreadMain(void* aVoidPtr)
{
  static_cast<SandboxChroot*>(aVoidPtr)->ThreadMain();
  return nullptr;
}

void
SandboxChroot::ThreadMain()
{
  
  
  
  LinuxCapabilities caps;
  caps.Effective(CAP_SYS_CHROOT) = true;
  if (!caps.SetCurrent()) {
    SANDBOX_LOG_ERROR("capset: %s", strerror(errno));
    MOZ_CRASH("Can't limit chroot thread's capabilities");
  }

  MOZ_ALWAYS_ZERO(pthread_mutex_lock(&mMutex));
  MOZ_ASSERT(mCommand == NO_THREAD);
  mCommand = NO_COMMAND;
  MOZ_ALWAYS_ZERO(pthread_cond_signal(&mWakeup));
  while (mCommand == NO_COMMAND) {
    MOZ_ALWAYS_ZERO(pthread_cond_wait(&mWakeup, &mMutex));
  }
  if (mCommand == DO_CHROOT) {
    MOZ_ASSERT(mFd >= 0);
    if (!ChrootToFileDesc(mFd)) {
      MOZ_CRASH("Failed to chroot");
    }
  } else {
    MOZ_ASSERT(mCommand == JUST_EXIT);
  }
  if (mFd >= 0) {
    AlwaysClose(mFd);
    mFd = -1;
  }
  mCommand = NO_THREAD;
  MOZ_ALWAYS_ZERO(pthread_mutex_unlock(&mMutex));
  
  
  if (!LinuxCapabilities().SetCurrent()) {
    MOZ_CRASH("can't drop capabilities");
  }
}

} 

#undef MOZ_ALWAYS_ZERO
