






































#ifndef mozilla_Util_h_
#define mozilla_Util_h_

#include "mozilla/Types.h"















MOZ_BEGIN_EXTERN_C

extern MFBT_API(void)
JS_Assert(const char *s, const char *file, JSIntn ln);

MOZ_END_EXTERN_C






#ifdef DEBUG

# define MOZ_ASSERT(expr_)                                      \
    ((expr_) ? (void)0 : JS_Assert(#expr_, __FILE__, __LINE__))

#else

# define MOZ_ASSERT(expr_) ((void)0)

#endif  







#ifndef MOZ_INLINE
# if defined __cplusplus
#  define MOZ_INLINE          inline
# elif defined _MSC_VER
#  define MOZ_INLINE          __inline
# elif defined __GNUC__
#  define MOZ_INLINE          __inline__
# else
#  define MOZ_INLINE          inline
# endif
#endif








#ifndef MOZ_ALWAYS_INLINE
# if defined DEBUG
#  define MOZ_ALWAYS_INLINE   MOZ_INLINE
# elif defined _MSC_VER
#  define MOZ_ALWAYS_INLINE   __forceinline
# elif defined __GNUC__
#  define MOZ_ALWAYS_INLINE   __attribute__((always_inline)) MOZ_INLINE
# else
#  define MOZ_ALWAYS_INLINE   MOZ_INLINE
# endif
#endif







#ifndef MOZ_NEVER_INLINE
# if defined _MSC_VER
#  define MOZ_NEVER_INLINE __declspec(noinline)
# elif defined __GNUC__
#  define MOZ_NEVER_INLINE __attribute__((noinline))
# else
#  define MOZ_NEVER_INLINE
# endif
#endif

#ifdef __cplusplus

namespace mozilla {


















template <typename T>
struct DebugOnly
{
#ifdef DEBUG
    T value;

    DebugOnly() {}
    DebugOnly(const T& other) : value(other) {}
    DebugOnly& operator=(const T& rhs) {
        value = rhs;
        return *this;
    }
    void operator++(int) {
        value++;
    }
    void operator--(int) {
        value--;
    }

    operator T&() { return value; }
    operator const T&() const { return value; }

    T& operator->() { return value; }

#else
    DebugOnly() {}
    DebugOnly(const T&) {}
    DebugOnly& operator=(const T&) { return *this; }
    void operator++(int) {}
    void operator--(int) {}
#endif






    ~DebugOnly() {}
};





template<class T>
struct AlignmentFinder
{
private:
  struct Aligner
  {
    char c;
    T t;
  };

public:
  static const int alignment = sizeof(Aligner) - sizeof(T);
};

#define MOZ_ALIGNOF(T) mozilla::AlignmentFinder<T>::alignment











#if defined(__GNUC__)
#define MOZ_ALIGNED_DECL(_type, _align) \
  _type __attribute__((aligned(_align)))

#elif defined(_MSC_VER)
#define MOZ_ALIGNED_DECL(_type, _align) \
  __declspec(align(_align)) _type

#else

#warning "We don't know how to align variables on this compiler."
#define MOZ_ALIGNED_DECL(_type, _align) _type

#endif






template<size_t align>
struct AlignedElem;






template<>
struct AlignedElem<1>
{
  MOZ_ALIGNED_DECL(uint8 elem, 1);
};

template<>
struct AlignedElem<2>
{
  MOZ_ALIGNED_DECL(uint8 elem, 2);
};

template<>
struct AlignedElem<4>
{
  MOZ_ALIGNED_DECL(uint8 elem, 4);
};

template<>
struct AlignedElem<8>
{
  MOZ_ALIGNED_DECL(uint8 elem, 8);
};

template<>
struct AlignedElem<16>
{
  MOZ_ALIGNED_DECL(uint8 elem, 16);
};











template <size_t nbytes>
struct AlignedStorage
{
    union U {
        char bytes[nbytes];
        uint64 _;
    } u;

    const void *addr() const { return u.bytes; }
    void *addr() { return u.bytes; }
};

template <class T>
struct AlignedStorage2
{
    union U {
        char bytes[sizeof(T)];
        uint64 _;
    } u;

    const T *addr() const { return (const T *)u.bytes; }
    T *addr() { return (T *)(void *)u.bytes; }
};












template <class T>
class Maybe
{
    AlignedStorage2<T> storage;
    bool constructed;

    T &asT() { return *storage.addr(); }

    explicit Maybe(const Maybe &other);
    const Maybe &operator=(const Maybe &other);

  public:
    Maybe() { constructed = false; }
    ~Maybe() { if (constructed) asT().~T(); }

    bool empty() const { return !constructed; }

    void construct() {
        MOZ_ASSERT(!constructed);
        new(storage.addr()) T();
        constructed = true;
    }

    template <class T1>
    void construct(const T1 &t1) {
        MOZ_ASSERT(!constructed);
        new(storage.addr()) T(t1);
        constructed = true;
    }

    template <class T1, class T2>
    void construct(const T1 &t1, const T2 &t2) {
        MOZ_ASSERT(!constructed);
        new(storage.addr()) T(t1, t2);
        constructed = true;
    }

    template <class T1, class T2, class T3>
    void construct(const T1 &t1, const T2 &t2, const T3 &t3) {
        MOZ_ASSERT(!constructed);
        new(storage.addr()) T(t1, t2, t3);
        constructed = true;
    }

    template <class T1, class T2, class T3, class T4>
    void construct(const T1 &t1, const T2 &t2, const T3 &t3, const T4 &t4) {
        MOZ_ASSERT(!constructed);
        new(storage.addr()) T(t1, t2, t3, t4);
        constructed = true;
    }

    T *addr() {
        MOZ_ASSERT(constructed);
        return &asT();
    }

    T &ref() {
        MOZ_ASSERT(constructed);
        return asT();
    }

    const T &ref() const {
        MOZ_ASSERT(constructed);
        return const_cast<Maybe *>(this)->asT();
    }

    void destroy() {
        ref().~T();
        constructed = false;
    }

    void destroyIfConstructed() {
        if (!empty())
            destroy();
    }
};







template <class T>
MOZ_ALWAYS_INLINE size_t
PointerRangeSize(T* begin, T* end)
{
    MOZ_ASSERT(end >= begin);
    return (size_t(end) - size_t(begin)) / sizeof(T);
}

} 

#endif 

#endif  
