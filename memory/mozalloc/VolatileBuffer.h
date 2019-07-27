



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
  bool OnHeap() const;

protected:
  bool Lock(void** aBuf);
  void Unlock();

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
    Lock();
  }

  ~VolatileBufferPtr_base() {
    Unlock();
  }

  bool WasBufferPurged() const {
    return mPurged;
  }

protected:
  RefPtr<VolatileBuffer> mVBuf;
  void* mMapping;

  void Set(VolatileBuffer* vbuf) {
    Unlock();
    mVBuf = vbuf;
    Lock();
  }

private:
  bool mPurged;

  void Lock() {
    if (mVBuf) {
      mPurged = !mVBuf->Lock(&mMapping);
    } else {
      mMapping = nullptr;
      mPurged = false;
    }
  }

  void Unlock() {
    if (mVBuf) {
      mVBuf->Unlock();
    }
  }
};

template <class T>
class VolatileBufferPtr : public VolatileBufferPtr_base
{
public:
  explicit VolatileBufferPtr(VolatileBuffer* vbuf) : VolatileBufferPtr_base(vbuf) {}
  VolatileBufferPtr() : VolatileBufferPtr_base(nullptr) {}

  VolatileBufferPtr(VolatileBufferPtr&& aOther)
    : VolatileBufferPtr_base(aOther.mVBuf)
  {
    aOther.Set(nullptr);
  }

  operator T*() const {
    return (T*) mMapping;
  }

  VolatileBufferPtr& operator=(VolatileBuffer* aVBuf)
  {
    Set(aVBuf);
    return *this;
  }

  VolatileBufferPtr& operator=(VolatileBufferPtr&& aOther)
  {
    MOZ_ASSERT(this != &aOther, "Self-moves are prohibited");
    Set(aOther.mVBuf);
    aOther.Set(nullptr);
    return *this;
  }

private:
  VolatileBufferPtr(VolatileBufferPtr const& vbufptr) MOZ_DELETE;
};

}; 

#endif 
