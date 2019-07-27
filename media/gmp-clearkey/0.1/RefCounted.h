















#ifndef __RefCount_h__
#define __RefCount_h__

#include <stdint.h>


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
  }
  uint32_t mRefCount;
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
