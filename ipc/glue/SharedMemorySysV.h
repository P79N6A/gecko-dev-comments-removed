






#ifndef mozilla_ipc_SharedMemorySysV_h
#define mozilla_ipc_SharedMemorySysV_h

#if (defined(OS_LINUX) && !defined(ANDROID)) || defined(OS_BSD)




#define MOZ_HAVE_SHAREDMEMORYSYSV

#include "SharedMemory.h"

#include "nsDebug.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>






namespace mozilla {
namespace ipc {


class SharedMemorySysV : public SharedMemory
{
public:
  typedef int Handle;

  SharedMemorySysV() :
    mHandle(-1),
    mData(nullptr)
  {
  }

  explicit SharedMemorySysV(Handle aHandle) :
    mHandle(aHandle),
    mData(nullptr)
  {
  }

  virtual ~SharedMemorySysV()
  {
    shmdt(mData);
    mHandle = -1;
    mData = nullptr;
  }

  virtual bool Create(size_t aNbytes) MOZ_OVERRIDE
  {
    int id = shmget(IPC_PRIVATE, aNbytes, IPC_CREAT | 0600);
    if (id == -1)
      return false;

    mHandle = id;
    mAllocSize = aNbytes;
    Created(aNbytes);

    return Map(aNbytes);
  }

  virtual bool Map(size_t nBytes) MOZ_OVERRIDE
  {
    
    if (mData)
      return true;

    if (!IsHandleValid(mHandle))
      return false;

    void* mem = shmat(mHandle, nullptr, 0);
    if (mem == (void*) -1) {
      char warning[256];
      ::snprintf(warning, sizeof(warning)-1,
                 "shmat(): %s (%d)\n", strerror(errno), errno);

      NS_WARNING(warning);

      return false;
    }

    
    
    shmctl(mHandle, IPC_RMID, 0);

    mData = mem;

#ifdef DEBUG
    struct shmid_ds info;
    if (shmctl(mHandle, IPC_STAT, &info) < 0)
      return false;

    NS_ABORT_IF_FALSE(nBytes <= info.shm_segsz,
                      "Segment doesn't have enough space!");
#endif

    Mapped(nBytes);
    return true;
  }

  virtual void* memory() const MOZ_OVERRIDE
  {
    return mData;
  }

  virtual SharedMemoryType Type() const MOZ_OVERRIDE
  {
    return TYPE_SYSV;
  }

  Handle GetHandle() const
  {
    NS_ABORT_IF_FALSE(IsHandleValid(mHandle), "invalid handle");
    return mHandle;
  }

  static Handle NULLHandle()
  {
    return -1;
  }

  static bool IsHandleValid(Handle aHandle)
  {
    return aHandle != -1;
  }

private:
  Handle mHandle;
  void* mData;
};

} 
} 

#endif 

#endif 
