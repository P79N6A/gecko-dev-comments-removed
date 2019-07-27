







#ifndef jit_shared_AtomicOperations_x86_shared_h
#define jit_shared_AtomicOperations_x86_shared_h

#include "jit/AtomicOperations.h"

































#if defined(__clang__) || defined(__GNUC__)
















# define LOCKFREE8









# if defined(__clang__) && defined(__i386)
#  undef LOCKFREE8
# endif

inline bool
js::jit::AtomicOperations::isLockfree8()
{
# ifndef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    MOZ_ASSERT(__atomic_always_lock_free(sizeof(int8_t), 0));
    MOZ_ASSERT(__atomic_always_lock_free(sizeof(int16_t), 0));
    MOZ_ASSERT(__atomic_always_lock_free(sizeof(int32_t), 0));
# endif
# ifdef LOCKFREE8
#  ifndef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    MOZ_ASSERT(__atomic_always_lock_free(sizeof(int64_t), 0));
#  endif
    return true;
# else
    return false;
# endif
}

inline void
js::jit::AtomicOperations::fenceSeqCst()
{
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    __sync_synchronize();
# else
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
# endif
}

template<typename T>
inline T
js::jit::AtomicOperations::loadSeqCst(T* addr)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    
    
    
    
    T v = *static_cast<T volatile*>(addr);
# else
    T v;
    __atomic_load(addr, &v, __ATOMIC_SEQ_CST);
# endif
    return v;
}

# ifndef LOCKFREE8
template<>
inline int64_t
js::jit::AtomicOperations::loadSeqCst(int64_t* addr)
{
    MOZ_CRASH();
}

template<>
inline uint64_t
js::jit::AtomicOperations::loadSeqCst(uint64_t* addr)
{
    MOZ_CRASH();
}
# endif 

template<typename T>
inline void
js::jit::AtomicOperations::storeSeqCst(T* addr, T val)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    *static_cast<T volatile*>(addr) = val;
    __sync_synchronize();
# else
    __atomic_store(addr, &val, __ATOMIC_SEQ_CST);
# endif
}

# ifndef LOCKFREE8
template<>
inline void
js::jit::AtomicOperations::storeSeqCst(int64_t* addr, int64_t val)
{
    MOZ_CRASH();
}

template<>
inline void
js::jit::AtomicOperations::storeSeqCst(uint64_t* addr, uint64_t val)
{
    MOZ_CRASH();
}
# endif 

template<typename T>
inline T
js::jit::AtomicOperations::exchangeSeqCst(T* addr, T val)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    T v;
    do {
        
        
        v = *addr;
    } while (!__sync_bool_compare_and_swap(addr, v, val));
    return v;
# else
    T v;
    __atomic_exchange(addr, &val, &v, __ATOMIC_SEQ_CST);
    return v;
# endif
}

# ifndef LOCKFREE8
template<>
inline int64_t
js::jit::AtomicOperations::exchangeSeqCst(int64_t* addr, int64_t val)
{
    MOZ_CRASH();
}

template<>
inline uint64_t
js::jit::AtomicOperations::exchangeSeqCst(uint64_t* addr, uint64_t val)
{
    MOZ_CRASH();
}
# endif 

template<typename T>
inline T
js::jit::AtomicOperations::compareExchangeSeqCst(T* addr, T oldval, T newval)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    return __sync_val_compare_and_swap(addr, oldval, newval);
# else
    __atomic_compare_exchange(addr, &oldval, &newval, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return oldval;
# endif
}

# ifndef LOCKFREE8
template<>
inline int64_t
js::jit::AtomicOperations::compareExchangeSeqCst(int64_t* addr, int64_t oldval, int64_t newval)
{
    MOZ_CRASH();
}

template<>
inline uint64_t
js::jit::AtomicOperations::compareExchangeSeqCst(uint64_t* addr, uint64_t oldval, uint64_t newval)
{
    MOZ_CRASH();
}
# endif 

template<typename T>
inline T
js::jit::AtomicOperations::fetchAddSeqCst(T* addr, T val)
{
    static_assert(sizeof(T) <= 4, "not available for 8-byte values yet");
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    return __sync_fetch_and_add(addr, val);
# else
    return __atomic_fetch_add(addr, val, __ATOMIC_SEQ_CST);
# endif
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchSubSeqCst(T* addr, T val)
{
    static_assert(sizeof(T) <= 4, "not available for 8-byte values yet");
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    return __sync_fetch_and_sub(addr, val);
# else
    return __atomic_fetch_sub(addr, val, __ATOMIC_SEQ_CST);
# endif
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchAndSeqCst(T* addr, T val)
{
    static_assert(sizeof(T) <= 4, "not available for 8-byte values yet");
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    return __sync_fetch_and_and(addr, val);
# else
    return __atomic_fetch_and(addr, val, __ATOMIC_SEQ_CST);
# endif
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchOrSeqCst(T* addr, T val)
{
    static_assert(sizeof(T) <= 4, "not available for 8-byte values yet");
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    return __sync_fetch_and_or(addr, val);
# else
    return __atomic_fetch_or(addr, val, __ATOMIC_SEQ_CST);
# endif
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchXorSeqCst(T* addr, T val)
{
    static_assert(sizeof(T) <= 4, "not available for 8-byte values yet");
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    return __sync_fetch_and_xor(addr, val);
# else
    return __atomic_fetch_xor(addr, val, __ATOMIC_SEQ_CST);
# endif
}

template<size_t nbytes>
inline void
js::jit::RegionLock::acquire(void* addr)
{
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    while (!__sync_bool_compare_and_swap(&spinlock, 0, 1))
        continue;
# else
    uint32_t zero = 0;
    uint32_t one = 1;
    while (!__atomic_compare_exchange(&spinlock, &zero, &one, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
        continue;
# endif
}

template<size_t nbytes>
inline void
js::jit::RegionLock::release(void* addr)
{
    MOZ_ASSERT(AtomicOperations::loadSeqCst(&spinlock) == 1, "releasing unlocked region lock");
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    __sync_sub_and_fetch(&spinlock, 1); 
# else
    uint32_t zero = 0;
    __atomic_store(&spinlock, &zero, __ATOMIC_SEQ_CST);
# endif
}

# undef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
# undef LOCKFREE8

#elif defined(_MSC_VER)






# define HAVE_EXCHANGE64

# if !_WIN64
#  undef HAVE_EXCHANGE64
# endif




inline bool
js::jit::AtomicOperations::isLockfree8()
{
    
    
    
    
    
    
    
    
    
    
    return true;
}

inline void
js::jit::AtomicOperations::fenceSeqCst()
{
    _ReadWriteBarrier();
# if JS_BITS_PER_WORD == 32
    
    
    
    __asm lock add [esp], 0;
# else
    _mm_mfence();
# endif
}

template<typename T>
inline T
js::jit::AtomicOperations::loadSeqCst(T* addr)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
    _ReadWriteBarrier();
    T v = *addr;
    _ReadWriteBarrier();
    return v;
}

template<typename T>
inline void
js::jit::AtomicOperations::storeSeqCst(T* addr, T val)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
    _ReadWriteBarrier();
    *addr = val;
    fenceSeqCst();
}

# define MSC_EXCHANGEOP(T, U, xchgop)                           \
    template<> inline T                                         \
    js::jit::AtomicOperations::exchangeSeqCst(T* addr, T val) { \
        MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());        \
        return (T)xchgop((U volatile*)addr, (U)val);            \
    }

# define MSC_EXCHANGEOP_CAS(T, U, cmpxchg)                           \
    template<> inline T                                              \
    js::jit::AtomicOperations::exchangeSeqCst(T* addr, T newval) {   \
        MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());             \
        T oldval;                                                    \
        do {                                                         \
            _ReadWriteBarrier();                                     \
            oldval = *addr;                                          \
        } while (!cmpxchg((U volatile*)addr, (U)newval, (U)oldval)); \
        return oldval;                                               \
    }

MSC_EXCHANGEOP(int8_t, char, _InterlockedExchange8)
MSC_EXCHANGEOP(uint8_t, char, _InterlockedExchange8)
MSC_EXCHANGEOP(int16_t, short, _InterlockedExchange16)
MSC_EXCHANGEOP(uint16_t, short, _InterlockedExchange16)
MSC_EXCHANGEOP(int32_t, long, _InterlockedExchange)
MSC_EXCHANGEOP(uint32_t, long, _InterlockedExchange)
# ifdef HAVE_EXCHANGE64
MSC_EXCHANGEOP(int64_t, __int64, _InterlockedExchange64)
MSC_EXCHANGEOP(uint64_t, __int64, _InterlockedExchange64)
# else
MSC_EXCHANGEOP_CAS(int64_t, __int64, _InterlockedCompareExchange64)
MSC_EXCHANGEOP_CAS(uint64_t, __int64, _InterlockedCompareExchange64)
# endif

# undef MSC_EXCHANGEOP
# undef MSC_EXCHANGEOP_CAS

# define MSC_CAS(T, U, cmpxchg)                                                     \
    template<> inline T                                                             \
    js::jit::AtomicOperations::compareExchangeSeqCst(T* addr, T oldval, T newval) { \
        MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());                            \
        return (T)cmpxchg((U volatile*)addr, (U)newval, (U)oldval);                 \
    }

MSC_CAS(int8_t, char, _InterlockedCompareExchange8)
MSC_CAS(uint8_t, char, _InterlockedCompareExchange8)
MSC_CAS(int16_t, short, _InterlockedCompareExchange16)
MSC_CAS(uint16_t, short, _InterlockedCompareExchange16)
MSC_CAS(int32_t, long, _InterlockedCompareExchange)
MSC_CAS(uint32_t, long, _InterlockedCompareExchange)
MSC_CAS(int64_t, __int64, _InterlockedCompareExchange64)
MSC_CAS(uint64_t, __int64, _InterlockedCompareExchange64)

# undef MSC_CAS

# define MSC_FETCHADDOP(T, U, xadd)                                           \
    template<> inline T                                                       \
    js::jit::AtomicOperations::fetchAddSeqCst(T* addr, T val) {               \
        static_assert(sizeof(T) <= 4, "not available for 8-byte values yet"); \
        return (T)xadd((U volatile*)addr, (U)val);                            \
    }                                                                         \
    template<> inline T                                                       \
    js::jit::AtomicOperations::fetchSubSeqCst(T* addr, T val) {               \
        static_assert(sizeof(T) <= 4, "not available for 8-byte values yet"); \
        return (T)xadd((U volatile*)addr, (U)-val);                           \
    }

MSC_FETCHADDOP(int8_t, char, _InterlockedExchangeAdd8)
MSC_FETCHADDOP(uint8_t, char, _InterlockedExchangeAdd8)
MSC_FETCHADDOP(int16_t, short, _InterlockedExchangeAdd16)
MSC_FETCHADDOP(uint16_t, short, _InterlockedExchangeAdd16)
MSC_FETCHADDOP(int32_t, long, _InterlockedExchangeAdd)
MSC_FETCHADDOP(uint32_t, long, _InterlockedExchangeAdd)

# undef MSC_FETCHADDOP

# define MSC_FETCHBITOP(T, U, andop, orop, xorop)                             \
    template<> inline T                                                       \
    js::jit::AtomicOperations::fetchAndSeqCst(T* addr, T val) {               \
        static_assert(sizeof(T) <= 4, "not available for 8-byte values yet"); \
        return (T)andop((U volatile*)addr, (U)val);                           \
    }                                                                         \
    template<> inline T                                                       \
    js::jit::AtomicOperations::fetchOrSeqCst(T* addr, T val) {                \
        static_assert(sizeof(T) <= 4, "not available for 8-byte values yet"); \
        return (T)orop((U volatile*)addr, (U)val);                            \
    }                                                                         \
    template<> inline T                                                       \
    js::jit::AtomicOperations::fetchXorSeqCst(T* addr, T val) {               \
        static_assert(sizeof(T) <= 4, "not available for 8-byte values yet"); \
        return (T)xorop((U volatile*)addr, (U)val);                           \
    }

MSC_FETCHBITOP(int8_t, char, _InterlockedAnd8, _InterlockedOr8, _InterlockedXor8)
MSC_FETCHBITOP(uint8_t, char, _InterlockedAnd8, _InterlockedOr8, _InterlockedXor8)
MSC_FETCHBITOP(int16_t, short, _InterlockedAnd16, _InterlockedOr16, _InterlockedXor16)
MSC_FETCHBITOP(uint16_t, short, _InterlockedAnd16, _InterlockedOr16, _InterlockedXor16)
MSC_FETCHBITOP(int32_t, long,  _InterlockedAnd, _InterlockedOr, _InterlockedXor)
MSC_FETCHBITOP(uint32_t, long, _InterlockedAnd, _InterlockedOr, _InterlockedXor)

# undef MSC_FETCHBITOP

template<size_t nbytes>
inline void
js::jit::RegionLock::acquire(void* addr)
{
    while (_InterlockedCompareExchange((long*)&spinlock, 1, 0) == 1)
        continue;
}

template<size_t nbytes>
inline void
js::jit::RegionLock::release(void* addr)
{
    MOZ_ASSERT(AtomicOperations::loadSeqCst(&spinlock) == 1, "releasing unlocked region lock");
    _InterlockedExchange((long*)&spinlock, 0);
}

# undef HAVE_EXCHANGE64

#elif defined(ENABLE_SHARED_ARRAY_BUFFER)

# error "Either disable JS shared memory, use GCC, Clang, or MSVC, or add code here"

#endif 

#endif 
