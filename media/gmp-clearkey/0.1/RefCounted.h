



#ifndef __RefCount_h__
#define __RefCount_h__


class RefCounted {
public:
  void AddRef() {
    ++mRefCount;
  }

  void Release() {
    if (mRefCount == 1) {
      delete this;
    } else {
      --mRefCount;
    }
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
