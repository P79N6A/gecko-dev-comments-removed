




#ifndef MOZILLA_SHAREDBUFFER_H_
#define MOZILLA_SHAREDBUFFER_H_

#include "mozilla/mozalloc.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

namespace mozilla {





class ThreadSharedObject {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ThreadSharedObject)
  virtual ~ThreadSharedObject() {}
};









class SharedBuffer : public ThreadSharedObject {
public:
  void* Data() { return this + 1; }

  static already_AddRefed<SharedBuffer> Create(size_t aSize)
  {
    void* m = moz_xmalloc(sizeof(SharedBuffer) + aSize);
    nsRefPtr<SharedBuffer> p = new (m) SharedBuffer();
    NS_ASSERTION((reinterpret_cast<char*>(p.get() + 1) - reinterpret_cast<char*>(p.get())) % 4 == 0,
                 "SharedBuffers should be at least 4-byte aligned");
    return p.forget();
  }

private:
  SharedBuffer() {}
};

}

#endif 
