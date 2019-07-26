










#ifndef mozilla_Util_h
#define mozilla_Util_h

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/Types.h"

#ifdef __cplusplus

namespace mozilla {





template<class T>
class AlignmentFinder
{
    struct Aligner
    {
        char c;
        T t;
    };

  public:
    static const size_t alignment = sizeof(Aligner) - sizeof(T);
};

#define MOZ_ALIGNOF(T) mozilla::AlignmentFinder<T>::alignment











#if defined(__GNUC__)
#  define MOZ_ALIGNED_DECL(_type, _align) \
     _type __attribute__((aligned(_align)))
#elif defined(_MSC_VER)
#  define MOZ_ALIGNED_DECL(_type, _align) \
     __declspec(align(_align)) _type
#else
#  warning "We don't know how to align variables on this compiler."
#  define MOZ_ALIGNED_DECL(_type, _align) _type
#endif







template<size_t align>
struct AlignedElem;






template<>
struct AlignedElem<1>
{
    MOZ_ALIGNED_DECL(uint8_t elem, 1);
};

template<>
struct AlignedElem<2>
{
    MOZ_ALIGNED_DECL(uint8_t elem, 2);
};

template<>
struct AlignedElem<4>
{
    MOZ_ALIGNED_DECL(uint8_t elem, 4);
};

template<>
struct AlignedElem<8>
{
    MOZ_ALIGNED_DECL(uint8_t elem, 8);
};

template<>
struct AlignedElem<16>
{
    MOZ_ALIGNED_DECL(uint8_t elem, 16);
};











template<size_t nbytes>
struct AlignedStorage
{
    union U {
      char bytes[nbytes];
      uint64_t _;
    } u;

    const void* addr() const { return u.bytes; }
    void* addr() { return u.bytes; }
};

template<class T>
struct AlignedStorage2
{
    union U {
      char bytes[sizeof(T)];
      uint64_t _;
    } u;

    const T* addr() const { return reinterpret_cast<const T*>(u.bytes); }
    T* addr() { return static_cast<T*>(static_cast<void*>(u.bytes)); }
};












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







template<class T>
MOZ_ALWAYS_INLINE size_t
PointerRangeSize(T* begin, T* end)
{
  MOZ_ASSERT(end >= begin);
  return (size_t(end) - size_t(begin)) / sizeof(T);
}







template<typename T, size_t N>
MOZ_CONSTEXPR size_t
ArrayLength(T (&arr)[N])
{
  return N;
}






template<typename T, size_t N>
MOZ_CONSTEXPR T*
ArrayEnd(T (&arr)[N])
{
  return arr + ArrayLength(arr);
}

} 

#endif 






#ifdef MOZ_HAVE_CXX11_CONSTEXPR
#  define MOZ_ARRAY_LENGTH(array)   mozilla::ArrayLength(array)
#else
#  define MOZ_ARRAY_LENGTH(array)   (sizeof(array)/sizeof((array)[0]))
#endif

#endif 
