






#ifndef mozilla_ipc_SharedMemory_h
#define mozilla_ipc_SharedMemory_h

#include "nsDebug.h"
#include "nsISupportsImpl.h"    
#include "mozilla/Attributes.h"





namespace {
enum Rights {
  RightsNone = 0,
  RightsRead = 1 << 0,
  RightsWrite = 1 << 1
};
}

namespace mozilla {
namespace ipc {

class SharedMemory
{
public:
  virtual ~SharedMemory()
  {
    MOZ_COUNT_DTOR(SharedMemory);
    Unmapped();
    Destroyed();
  }

  enum SharedMemoryType {
    TYPE_BASIC,
    TYPE_SYSV,
    TYPE_UNKNOWN
  };

  size_t Size() const { return mMappedSize; }

  virtual void* memory() const = 0;

  virtual bool Create(size_t size) = 0;
  virtual bool Map(size_t nBytes) = 0;

  virtual SharedMemoryType Type() const = 0;

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

  NS_INLINE_DECL_REFCOUNTING(SharedMemory)

  static void SystemProtect(char* aAddr, size_t aSize, int aRights);
  static size_t SystemPageSize();
  static size_t PageAlignedSize(size_t aSize);

protected:
  SharedMemory();

  
  
  
  
  
  
  
  void Created(size_t aNBytes);
  void Mapped(size_t aNBytes);
  void Unmapped();
  void Destroyed();

  
  
  
  
  size_t mAllocSize;
  
  
  size_t mMappedSize;
};

} 
} 


#endif 
