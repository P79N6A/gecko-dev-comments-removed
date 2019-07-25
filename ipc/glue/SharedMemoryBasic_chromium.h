






































#ifndef mozilla_ipc_SharedMemoryBasic_chromium_h
#define mozilla_ipc_SharedMemoryBasic_chromium_h

#include "base/shared_memory.h"
#include "SharedMemory.h"

#include "nsDebug.h"






namespace mozilla {
namespace ipc {

class SharedMemoryBasic : public SharedMemory
{
public:
  typedef base::SharedMemoryHandle Handle;

  SharedMemoryBasic()
  {
  }

  SharedMemoryBasic(const Handle& aHandle)
    : mSharedMemory(aHandle, false)
  {
  }

  NS_OVERRIDE
  virtual bool Create(size_t aNbytes)
  {
    bool ok = mSharedMemory.Create("", false, false, aNbytes);
    if (ok) {
      Created(aNbytes);
    }
    return ok;
  }

  NS_OVERRIDE
  virtual bool Map(size_t nBytes)
  {
    bool ok = mSharedMemory.Map(nBytes);
    if (ok) {
      Mapped(nBytes);
    }
    return ok;
  }

  NS_OVERRIDE
  virtual void* memory() const
  {
    return mSharedMemory.memory();
  }

  NS_OVERRIDE
  virtual SharedMemoryType Type() const
  {
    return TYPE_BASIC;
  }

  static Handle NULLHandle()
  {
    return base::SharedMemory::NULLHandle();
  }

  static bool IsHandleValid(const Handle &aHandle)
  {
    return base::SharedMemory::IsHandleValid(aHandle);
  }

  bool ShareToProcess(base::ProcessHandle process,
                      Handle* new_handle)
  {
    base::SharedMemoryHandle handle;
    bool ret = mSharedMemory.ShareToProcess(process, &handle);
    if (ret)
      *new_handle = handle;
    return ret;
  }

private:
  base::SharedMemory mSharedMemory;
};

} 
} 


#endif 
