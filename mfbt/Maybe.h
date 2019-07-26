







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
    ~Maybe() { if (constructed) asT().~T(); }

    bool empty() const { return !constructed; }

    void construct() {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T();
      constructed = true;
    }

    template<class T1>
    void construct(const T1& t1) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1);
      constructed = true;
    }

    template<class T1, class T2>
    void construct(const T1& t1, const T2& t2) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2);
      constructed = true;
    }

    template<class T1, class T2, class T3>
    void construct(const T1& t1, const T2& t2, const T3& t3) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3);
      constructed = true;
    }

    template<class T1, class T2, class T3, class T4>
    void construct(const T1& t1, const T2& t2, const T3& t3, const T4& t4) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3, t4);
      constructed = true;
    }

    template<class T1, class T2, class T3, class T4, class T5>
    void construct(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3, t4, t5);
      constructed = true;
    }

    template<class T1, class T2, class T3, class T4, class T5,
             class T6>
    void construct(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5,
                   const T6& t6) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3, t4, t5, t6);
      constructed = true;
    }

    template<class T1, class T2, class T3, class T4, class T5,
             class T6, class T7>
    void construct(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5,
                   const T6& t6, const T7& t7) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3, t4, t5, t6, t7);
      constructed = true;
    }

    template<class T1, class T2, class T3, class T4, class T5,
             class T6, class T7, class T8>
    void construct(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5,
                   const T6& t6, const T7& t7, const T8& t8) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3, t4, t5, t6, t7, t8);
      constructed = true;
    }

    template<class T1, class T2, class T3, class T4, class T5,
             class T6, class T7, class T8, class T9>
    void construct(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5,
                   const T6& t6, const T7& t7, const T8& t8, const T9& t9) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3, t4, t5, t6, t7, t8, t9);
      constructed = true;
    }

    template<class T1, class T2, class T3, class T4, class T5,
             class T6, class T7, class T8, class T9, class T10>
    void construct(const T1& t1, const T2& t2, const T3& t3, const T4& t4, const T5& t5,
                   const T6& t6, const T7& t7, const T8& t8, const T9& t9, const T10& t10) {
      MOZ_ASSERT(!constructed);
      ::new (storage.addr()) T(t1, t2, t3, t4, t5, t6, t7, t8, t9, t10);
      constructed = true;
    }

    T* addr() {
      MOZ_ASSERT(constructed);
      return &asT();
    }

    T& ref() {
      MOZ_ASSERT(constructed);
      return asT();
    }

    const T& ref() const {
      MOZ_ASSERT(constructed);
      return const_cast<Maybe*>(this)->asT();
    }

    void destroy() {
      ref().~T();
      constructed = false;
    }

    void destroyIfConstructed() {
      if (!empty())
        destroy();
    }

  private:
    Maybe(const Maybe& other) MOZ_DELETE;
    const Maybe& operator=(const Maybe& other) MOZ_DELETE;
};

} 

#endif 
