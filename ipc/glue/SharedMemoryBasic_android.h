







































#ifndef mozilla_ipc_SharedMemoryBasic_android_h
#define mozilla_ipc_SharedMemoryBasic_android_h

#include "base/file_descriptor_posix.h"

#include "SharedMemory.h"






namespace mozilla {
namespace ipc {

class SharedMemoryBasic : public SharedMemory
{
public:
  typedef base::FileDescriptor Handle;

  SharedMemoryBasic();

  SharedMemoryBasic(const Handle& aHandle);

  virtual ~SharedMemoryBasic();

  NS_OVERRIDE
  virtual bool Create(size_t aNbytes);

  NS_OVERRIDE
  virtual bool Map(size_t nBytes);

  NS_OVERRIDE
  virtual void* memory() const
  {
    return mMemory;
  }

  NS_OVERRIDE
  virtual SharedMemoryType Type() const
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
  void Unmap();
  void Destroy();

  
  int mShmFd;
  
  void *mMemory;
};

} 
} 

#endif 
