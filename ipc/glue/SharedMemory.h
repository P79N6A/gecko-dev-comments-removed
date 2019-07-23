






































#ifndef mozilla_ipc_SharedMemory_h
#define mozilla_ipc_SharedMemory_h

#include "base/shared_memory.h"

#include "nsDebug.h"





namespace {
enum Rights {
  RightsNone = 0,
  RightsRead = 1 << 0,
  RightsWrite = 1 << 1
};
}

namespace mozilla {
namespace ipc {

class SharedMemory : public base::SharedMemory
{
public:
  typedef base::SharedMemoryHandle SharedMemoryHandle;

  SharedMemory() :
    base::SharedMemory(),
    mSize(0)
  {
  }

  SharedMemory(const SharedMemoryHandle& aHandle) :
    base::SharedMemory(aHandle, false),
    mSize(0)
  {
  }

  bool Map(size_t nBytes)
  {
    bool ok = base::SharedMemory::Map(nBytes);
    if (ok)
      mSize = nBytes;
    return ok;
  }

  size_t Size()
  {
    return mSize;
  }

  void
  Protect(char* aAddr, size_t aSize, int aRights)
  {
    char* memStart = reinterpret_cast<char*>(memory());
    if (!memStart)
      NS_RUNTIMEABORT("SharedMemory region points at NULL!");
    char* memEnd = memStart + Size();

    char* protStart = aAddr;
    if (!protStart)
      NS_RUNTIMEABORT("trying to Protect() a NULL region!");
    char* protEnd = protStart + aSize;

    if (!(memStart <= protStart
          && protEnd <= memEnd))
      NS_RUNTIMEABORT("attempt to Protect() a region outside this SharedMemory");

    
    SystemProtect(aAddr, aSize, aRights);
  }

  static void SystemProtect(char* aAddr, size_t aSize, int aRights);
  static size_t SystemPageSize();

private:
  
  size_t mSize;
};

} 
} 


#endif 
