







































#include <android/log.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "base/process_util.h"

#include "SharedMemoryBasic.h"





#include <linux/ashmem.h>

namespace mozilla {
namespace ipc {

static void
LogError(const char* what)
{
  __android_log_print(ANDROID_LOG_ERROR, "Gecko",
                      "%s: %s (%d)", what, strerror(errno), errno);
}

SharedMemoryBasic::SharedMemoryBasic()
  : mShmFd(-1)
  , mAllocSize(0)
  , mSize(0)
  , mMemory(nsnull)
{ }

SharedMemoryBasic::SharedMemoryBasic(const Handle& aHandle)
  : mShmFd(aHandle.fd)
  , mAllocSize(0)
  , mSize(0)
  , mMemory(nsnull)
{ }

SharedMemoryBasic::~SharedMemoryBasic()
{
  Unmap();
  Destroy();
}

bool
SharedMemoryBasic::Create(size_t aNbytes)
{
  NS_ABORT_IF_FALSE(-1 == mShmFd, "Already Create()d");

  
  int shmfd = open("/" ASHMEM_NAME_DEF, O_RDWR, 0600);
  if (-1 == shmfd) {
    LogError("ShmemAndroid::Create():open");
    return false;
  }

  if (ioctl(shmfd, ASHMEM_SET_SIZE, aNbytes)) {
    LogError("ShmemAndroid::Unmap():ioctl(SET_SIZE)");
    close(shmfd);
    return false;
  }

  mShmFd = shmfd;
  mAllocSize = aNbytes;
  Created(aNbytes);
  return true;
}

bool
SharedMemoryBasic::Map(size_t nBytes)
{
  NS_ABORT_IF_FALSE(nsnull == mMemory, "Already Map()d");

  mMemory = mmap(nsnull, nBytes,
                 PROT_READ | PROT_WRITE,
                 MAP_SHARED,
                 mShmFd,
                 0);
  if (MAP_FAILED == mMemory) {
    LogError("ShmemAndroid::Map()");
    mMemory = nsnull;
    return false;
  }

  mSize = nBytes;
  Mapped(nBytes);
  return true;
}

bool
SharedMemoryBasic::ShareToProcess(base::ProcessHandle,
                                  Handle* aNewHandle)
{
  NS_ABORT_IF_FALSE(mShmFd >= 0, "Should have been Create()d by now");

  int shmfdDup = dup(mShmFd);
  if (-1 == shmfdDup) {
    LogError("ShmemAndroid::ShareToProcess()");
    return false;
  }

  aNewHandle->fd = shmfdDup;
  aNewHandle->auto_close = true;
  return true;
}

void
SharedMemoryBasic::Unmap()
{
  if (!mMemory) {
    return;
  }

  if (munmap(mMemory, mSize)) {
    LogError("ShmemAndroid::Unmap()");
  }
  mMemory = nsnull;
  mSize = 0;
  Unmapped(mSize);
}

void
SharedMemoryBasic::Destroy()
{
  if (mShmFd > 0) {
    close(mShmFd);
    if (mAllocSize) {
      Destroyed(mAllocSize);
    }
  }
}

} 
} 
