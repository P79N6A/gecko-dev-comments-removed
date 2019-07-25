







































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





#define ASHMEM_DEVICE  		"/dev/ashmem"
#define ASHMEM_NAME_LEN		256
#define __ASHMEMIOC 0x77
#define ASHMEM_SET_NAME		_IOW(__ASHMEMIOC, 1, char[ASHMEM_NAME_LEN])
#define ASHMEM_GET_NAME		_IOR(__ASHMEMIOC, 2, char[ASHMEM_NAME_LEN])
#define ASHMEM_SET_SIZE		_IOW(__ASHMEMIOC, 3, size_t)
#define ASHMEM_GET_SIZE		_IO(__ASHMEMIOC, 4)
#define ASHMEM_SET_PROT_MASK	_IOW(__ASHMEMIOC, 5, unsigned long)
#define ASHMEM_GET_PROT_MASK	_IO(__ASHMEMIOC, 6)

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
  , mSize(0)
  , mMemory(nsnull)
{ }

SharedMemoryBasic::SharedMemoryBasic(const Handle& aHandle)
  : mShmFd(aHandle.fd)
  , mSize(0)
  , mMemory(nsnull)
{ }

SharedMemoryBasic::~SharedMemoryBasic()
{
  Unmap();
  if (mShmFd > 0) {
    close(mShmFd);
  }
}

bool
SharedMemoryBasic::Create(size_t aNbytes)
{
  NS_ABORT_IF_FALSE(-1 == mShmFd, "Already Create()d");

  
  int shmfd = open(ASHMEM_DEVICE, O_RDWR, 0600);
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
}

} 
} 
