






#ifndef mozilla_ipc_SharedMemoryBasic_android_h
#define mozilla_ipc_SharedMemoryBasic_android_h

#include "base/file_descriptor_posix.h"

#include "SharedMemory.h"






namespace mozilla {
namespace ipc {

class SharedMemoryBasic MOZ_FINAL : public SharedMemory
{
public:
  typedef base::FileDescriptor Handle;

  SharedMemoryBasic();

  SharedMemoryBasic(const Handle& aHandle);

  virtual bool Create(size_t aNbytes) MOZ_OVERRIDE;

  virtual bool Map(size_t nBytes) MOZ_OVERRIDE;

  virtual void* memory() const MOZ_OVERRIDE
  {
    return mMemory;
  }

  virtual SharedMemoryType Type() const MOZ_OVERRIDE
  {
    return TYPE_BASIC;
  }

  static Handle NULLHandle()
  {
    return Handle();
  }

  static bool IsHandleValid(const Handle &aHandle)
  {
    return aHandle.fd >= 0;
  }

  bool ShareToProcess(base::ProcessHandle aProcess,
                      Handle* aNewHandle);

private:
  ~SharedMemoryBasic();

  void Unmap();
  void Destroy();

  
  int mShmFd;
  
  void *mMemory;
};

} 
} 

#endif 
