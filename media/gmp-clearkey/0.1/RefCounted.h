















#ifndef __RefCount_h__
#define __RefCount_h__

#include <stdint.h>
#include <assert.h>
#include "ClearKeyUtils.h"

#if defined(_MSC_VER)
#include <atomic>
typedef std::atomic<uint32_t> AtomicRefCount;
#else
class AtomicRefCount {
public:
  explicit AtomicRefCount(uint32_t aValue)
    : mCount(aValue)
    , mMutex(GMPCreateMutex())
  {
    assert(mMutex);
  }
  ~AtomicRefCount()
  {
    if (mMutex) {
      mMutex->Destroy();
    }
  }
  uint32_t operator--() {
    AutoLock lock(mMutex);
    return --mCount;
  }
  uint32_t operator++() {
    AutoLock lock(mMutex);
    return ++mCount;
  }
  operator uint32_t() {
    AutoLock lock(mMutex);
    return mCount;
  }
private:
  uint32_t mCount;
  GMPMutex* mMutex;
};
#endif


class RefCounted {
public:
  void AddRef() {
    ++mRefCount;
  }

  uint32_t Release() {
    uint32_t newCount = --mRefCount;
    if (!newCount) {
      delete this;
    }
    return newCount;
  }

protected:
  RefCounted()
    : mRefCount(0)
  {
  }
  virtual ~RefCounted()
  {
    assert(!mRefCount);
  }
  AtomicRefCount mRefCount;
};

template<class T>
class RefPtr {
public:
  explicit RefPtr(T* aPtr) : mPtr(nullptr) {
    Assign(aPtr);
  }
  ~RefPtr() {
    Assign(nullptr);
  }
  T* operator->() const { return mPtr; }

  RefPtr& operator=(T* aVal) {
    Assign(aVal);
    return *this;
  }

private:
  void Assign(T* aPtr) {
    if (mPtr) {
      mPtr->Release();
    }
    mPtr = aPtr;
    if (mPtr) {
      aPtr->AddRef();
    }
  }
  T* mPtr;
};

#endif 
