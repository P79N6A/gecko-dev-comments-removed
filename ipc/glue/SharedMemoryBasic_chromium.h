






#ifndef mozilla_ipc_SharedMemoryBasic_chromium_h
#define mozilla_ipc_SharedMemoryBasic_chromium_h

#include "base/shared_memory.h"
#include "SharedMemory.h"

#include "nsDebug.h"






namespace mozilla {
namespace ipc {

class SharedMemoryBasic final : public SharedMemory
{
public:
  typedef base::SharedMemoryHandle Handle;

  SharedMemoryBasic()
  {
  }

  explicit SharedMemoryBasic(const Handle& aHandle)
    : mSharedMemory(aHandle, false)
  {
  }

  virtual bool Create(size_t aNbytes) override
  {
    bool ok = mSharedMemory.Create("", false, false, aNbytes);
    if (ok) {
      Created(aNbytes);
    }
    return ok;
  }

  virtual bool Map(size_t nBytes) override
  {
    bool ok = mSharedMemory.Map(nBytes);
    if (ok) {
      Mapped(nBytes);
    }
    return ok;
  }

  virtual void* memory() const override
  {
    return mSharedMemory.memory();
  }

  virtual SharedMemoryType Type() const override
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

  bool ShareToProcess(base::ProcessId aProcessId,
                      Handle* new_handle)
  {
    base::SharedMemoryHandle handle;
    bool ret = mSharedMemory.ShareToProcess(aProcessId, &handle);
    if (ret)
      *new_handle = handle;
    return ret;
  }

private:
  ~SharedMemoryBasic()
  {
  }

  base::SharedMemory mSharedMemory;
};

} 
} 


#endif 
