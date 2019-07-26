







#ifndef mozilla_Maybe_h
#define mozilla_Maybe_h

#include "mozilla/Alignment.h"
#include "mozilla/Assertions.h"


#include <new>

namespace mozilla {












template<class T>
class Maybe
{
  AlignedStorage2<T> storage;
  bool constructed;

  T& asT() { return *storage.addr(); }

public:
  Maybe() { constructed = false; }
  ~Maybe() { if (constructed) { asT().~T(); } }

  bool empty() const { return !constructed; }

  void construct()
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T();
    constructed = true;
  }

  template<class T1>
  void construct(const T1& aT1)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1);
    constructed = true;
  }

  template<class T1, class T2>
  void construct(const T1& aT1, const T2& aT2)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2);
    constructed = true;
  }

  template<class T1, class T2, class T3>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3);
    constructed = true;
  }

  template<class T1, class T2, class T3, class T4>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3, const T4& aT4)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3, aT4);
    constructed = true;
  }

  template<class T1, class T2, class T3, class T4, class T5>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3, const T4& aT4,
                 const T5& aT5)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3, aT4, aT5);
    constructed = true;
  }

  template<class T1, class T2, class T3, class T4, class T5, class T6>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3, const T4& aT4,
                 const T5& aT5, const T6& aT6)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3, aT4, aT5, aT6);
    constructed = true;
  }

  template<class T1, class T2, class T3, class T4, class T5, class T6,
           class T7>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3, const T4& aT4,
                 const T5& aT5, const T6& aT6, const T7& aT7)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3, aT4, aT5, aT6, aT7);
    constructed = true;
  }

  template<class T1, class T2, class T3, class T4, class T5, class T6,
           class T7, class T8>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3, const T4& aT4,
                 const T5& aT5, const T6& aT6, const T7& aT7, const T8& aT8)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3, aT4, aT5, aT6, aT7, aT8);
    constructed = true;
  }

  template<class T1, class T2, class T3, class T4, class T5, class T6,
           class T7, class T8, class T9>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3, const T4& aT4,
                 const T5& aT5, const T6& aT6, const T7& aT7, const T8& aT8,
                 const T9& aT9)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3, aT4, aT5, aT6, aT7, aT8, aT9);
    constructed = true;
  }

  template<class T1, class T2, class T3, class T4, class T5, class T6,
           class T7, class T8, class T9, class T10>
  void construct(const T1& aT1, const T2& aT2, const T3& aT3, const T4& aT4,
                 const T5& aT5, const T6& aT6, const T7& aT7, const T8& aT8,
                 const T9& aT9, const T10& aT10)
  {
    MOZ_ASSERT(!constructed);
    ::new (storage.addr()) T(aT1, aT2, aT3, aT4, aT5, aT6, aT7, aT8, aT9, aT10);
    constructed = true;
  }

  T* addr()
  {
    MOZ_ASSERT(constructed);
    return &asT();
  }

  T& ref()
  {
    MOZ_ASSERT(constructed);
    return asT();
  }

  const T& ref() const
  {
    MOZ_ASSERT(constructed);
    return const_cast<Maybe*>(this)->asT();
  }

  void destroy()
  {
    ref().~T();
    constructed = false;
  }

  void destroyIfConstructed()
  {
    if (!empty()) {
      destroy();
    }
  }

private:
  Maybe(const Maybe& aOther) MOZ_DELETE;
  const Maybe& operator=(const Maybe& aOther) MOZ_DELETE;
};

} 

#endif 
