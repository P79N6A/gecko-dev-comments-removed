



#ifndef __RefCount_h__
#define __RefCount_h__


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

#endif 
