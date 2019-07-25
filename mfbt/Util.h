






































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

    operator T&() { return value; }
    operator const T&() const { return value; }

    T& operator->() { return value; }

#else
    DebugOnly() {}
    DebugOnly(const T&) {}
    DebugOnly& operator=(const T&) {}   
#endif

    




    ~DebugOnly() {}
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
    T *addr() { return (T *)u.bytes; }
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

    void destroy() {
        ref().~T();
        constructed = false;
    }

    void destroyIfConstructed() {
        if (!empty())
            destroy();
    }
};

} 

#endif 

#endif  
