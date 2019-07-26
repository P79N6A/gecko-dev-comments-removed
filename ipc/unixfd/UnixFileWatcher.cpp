





#include <fcntl.h>
#include "UnixFileWatcher.h"

namespace mozilla {
namespace ipc {

UnixFileWatcher::~UnixFileWatcher()
{
}

nsresult
UnixFileWatcher::Open(const char* aFilename, int aFlags, mode_t aMode)
{
  MOZ_ASSERT(MessageLoopForIO::current() == GetIOLoop());
  MOZ_ASSERT(aFlags & O_NONBLOCK);

  int fd = TEMP_FAILURE_RETRY(open(aFilename, aFlags, aMode));
  if (fd < 0) {
    OnError("open", errno);
    return NS_ERROR_FAILURE;
  }
  SetFd(fd);
  OnOpened();

  return NS_OK;
}

UnixFileWatcher::UnixFileWatcher(MessageLoop* aIOLoop)
: UnixFdWatcher(aIOLoop)
{
}

UnixFileWatcher::UnixFileWatcher(MessageLoop* aIOLoop, int aFd)
: UnixFdWatcher(aIOLoop, aFd)
{
}

}
}
