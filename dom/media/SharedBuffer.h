




#ifndef MOZILLA_SHAREDBUFFER_H_
#define MOZILLA_SHAREDBUFFER_H_

#include "mozilla/CheckedInt.h"
#include "mozilla/mozalloc.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

namespace mozilla {





class ThreadSharedObject {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ThreadSharedObject)

  bool IsShared() { return mRefCnt.get() > 1; }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return 0;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }
protected:
  
  virtual ~ThreadSharedObject() {}
};









class SharedBuffer : public ThreadSharedObject {
public:
  void* Data() { return this + 1; }

  static already_AddRefed<SharedBuffer> Create(size_t aSize)
  {
    CheckedInt<size_t> size = sizeof(SharedBuffer);
    size += aSize;
    if (!size.isValid()) {
      MOZ_CRASH();
    }
    void* m = moz_xmalloc(size.value());
    nsRefPtr<SharedBuffer> p = new (m) SharedBuffer();
    NS_ASSERTION((reinterpret_cast<char*>(p.get() + 1) - reinterpret_cast<char*>(p.get())) % 4 == 0,
                 "SharedBuffers should be at least 4-byte aligned");
    return p.forget();
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

private:
  SharedBuffer() {}
};

}

#endif 
