



#ifndef mozalloc_VolatileBuffer_h
#define mozalloc_VolatileBuffer_h

#include "mozilla/mozalloc.h"
#include "mozilla/RefPtr.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/NullPtr.h"
































namespace mozilla {

class MOZALLOC_EXPORT VolatileBuffer : public RefCounted<VolatileBuffer>
{
  friend class VolatileBufferPtr_base;
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(VolatileBuffer)
  VolatileBuffer();
  ~VolatileBuffer();

  
  bool Init(size_t aSize, size_t aAlignment = sizeof(void*));

  size_t HeapSizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;
  size_t NonHeapSizeOfExcludingThis() const;

protected:
  bool Lock(void** aBuf);
  void Unlock();
  bool OnHeap() const;

private:
  void* mBuf;
  size_t mSize;
  int mLockCount;
#if defined(ANDROID)
  int mFd;
#elif defined(XP_DARWIN)
  bool mHeap;
#elif defined(XP_WIN)
  bool mHeap;
  bool mFirstLock;
#endif
};

class VolatileBufferPtr_base {
public:
  explicit VolatileBufferPtr_base(VolatileBuffer* vbuf) : mVBuf(vbuf) {
    if (vbuf) {
      mPurged = !vbuf->Lock(&mMapping);
    } else {
      mMapping = nullptr;
      mPurged = false;
    }
  }

  ~VolatileBufferPtr_base() {
    if (mVBuf) {
      mVBuf->Unlock();
    }
  }

  bool WasBufferPurged() const {
    return mPurged;
  }

protected:
  void* mMapping;

private:
  RefPtr<VolatileBuffer> mVBuf;
  bool mPurged;
};

template <class T>
class VolatileBufferPtr : public VolatileBufferPtr_base
{
public:
  explicit VolatileBufferPtr(VolatileBuffer* vbuf) : VolatileBufferPtr_base(vbuf) {}

  operator T*() const {
    return (T*) mMapping;
  }
};

}; 

#endif 
