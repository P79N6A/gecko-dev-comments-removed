






































#ifndef mozilla_ipc_SharedMemorySysV_h
#define mozilla_ipc_SharedMemorySysV_h

#if defined(OS_LINUX) && !defined(ANDROID)




#define MOZ_HAVE_SHAREDMEMORYSYSV

#include "SharedMemory.h"

#include "nsDebug.h"

#include <errno.h>
#include <fcntl.h>
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
    mData(nsnull),
    mSize(0)
  {
  }

  SharedMemorySysV(Handle aHandle) :
    mHandle(aHandle),
    mData(nsnull),
    mSize(0)
  {
  }

  virtual ~SharedMemorySysV()
  {
    shmdt(mData);
    mHandle = -1;
    mData = nsnull;
    mSize = 0;
    
  }

  NS_OVERRIDE
  virtual bool Create(size_t aNbytes)
  {
    int id = shmget(IPC_PRIVATE, aNbytes, IPC_CREAT | 0600);
    if (id == -1)
      return false;

    mHandle = id;

    if (!Map(aNbytes))
      return false;

    return true;
  }

  NS_OVERRIDE
  virtual bool Map(size_t nBytes)
  {
    
    if (mData)
      return true;

    if (!IsHandleValid(mHandle))
      return false;

    void* mem = shmat(mHandle, nsnull, 0);
    if (mem == (void*) -1) {
      char warning[256];
      snprintf(warning, sizeof(warning)-1,
               "shmat(): %s (%d)\n", strerror(errno), errno);

      NS_WARNING(warning);

      return false;
    }

    
    
    shmctl(mHandle, IPC_RMID, 0);

    mData = mem;
    mSize = nBytes;

#ifdef NS_DEBUG
    struct shmid_ds info;
    if (shmctl(mHandle, IPC_STAT, &info) < 0)
      return false;

    NS_ABORT_IF_FALSE(nBytes <= info.shm_segsz,
                      "Segment doesn't have enough space!");
#endif

    return true;
  }

  NS_OVERRIDE
  virtual size_t Size() const
  {
    return mSize;
  }

  NS_OVERRIDE
  virtual void* memory() const
  {
    return mData;
  }

  NS_OVERRIDE
  virtual SharedMemoryType Type() const
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
  size_t mSize;
};

} 
} 

#endif 

#endif 
