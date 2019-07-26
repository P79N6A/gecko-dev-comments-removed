





#include <fcntl.h>
#include "UnixFdWatcher.h"

#ifdef CHROMIUM_LOG
#undef CHROMIUM_LOG
#endif

#if defined(MOZ_WIDGET_GONK)
#include <android/log.h>
#define CHROMIUM_LOG(args...)  __android_log_print(ANDROID_LOG_INFO, "I/O", args);
#else
#include <stdio.h>
#define IODEBUG true
#define CHROMIUM_LOG(args...) if (IODEBUG) printf(args);
#endif

namespace mozilla {
namespace ipc {

UnixFdWatcher::~UnixFdWatcher()
{
  NS_WARN_IF(IsOpen()); 
}

void
UnixFdWatcher::Close()
{
  MOZ_ASSERT(MessageLoopForIO::current() == mIOLoop);

  if (NS_WARN_IF(!IsOpen())) {
    
    return;
  }
  OnClose();
  RemoveWatchers(READ_WATCHER|WRITE_WATCHER);
  mFd.dispose();
}

void
UnixFdWatcher::AddWatchers(unsigned long aWatchers, bool aPersistent)
{
  MOZ_ASSERT(MessageLoopForIO::current() == mIOLoop);
  MOZ_ASSERT(IsOpen());

  
  
  
  RemoveWatchers(aWatchers);

  if (aWatchers & READ_WATCHER) {
    MessageLoopForIO::current()->WatchFileDescriptor(
      mFd,
      aPersistent,
      MessageLoopForIO::WATCH_READ,
      &mReadWatcher,
      this);
  }
  if (aWatchers & WRITE_WATCHER) {
    MessageLoopForIO::current()->WatchFileDescriptor(
      mFd,
      aPersistent,
      MessageLoopForIO::WATCH_WRITE,
      &mWriteWatcher,
      this);
  }
}

void
UnixFdWatcher::RemoveWatchers(unsigned long aWatchers)
{
  MOZ_ASSERT(MessageLoopForIO::current() == mIOLoop);
  MOZ_ASSERT(IsOpen());

  if (aWatchers & READ_WATCHER) {
    mReadWatcher.StopWatchingFileDescriptor();
  }
  if (aWatchers & WRITE_WATCHER) {
    mWriteWatcher.StopWatchingFileDescriptor();
  }
}

void
UnixFdWatcher::OnError(const char* aFunction, int aErrno)
{
  MOZ_ASSERT(MessageLoopForIO::current() == mIOLoop);

  CHROMIUM_LOG("%s failed with error %d (%s)",
               aFunction, aErrno, strerror(aErrno));
}

UnixFdWatcher::UnixFdWatcher(MessageLoop* aIOLoop)
: mIOLoop(aIOLoop)
{
  MOZ_ASSERT(mIOLoop);
}

UnixFdWatcher::UnixFdWatcher(MessageLoop* aIOLoop, int aFd)
: mIOLoop(aIOLoop)
, mFd(aFd)
{
  MOZ_ASSERT(mIOLoop);
}

void
UnixFdWatcher::SetFd(int aFd)
{
  MOZ_ASSERT(MessageLoopForIO::current() == mIOLoop);
  MOZ_ASSERT(!IsOpen());
  MOZ_ASSERT(FdIsNonBlocking(aFd));

  mFd = aFd;
}

bool
UnixFdWatcher::FdIsNonBlocking(int aFd)
{
  int flags = TEMP_FAILURE_RETRY(fcntl(aFd, F_GETFL));
  return (flags > 0) && (flags & O_NONBLOCK);
}

}
}
