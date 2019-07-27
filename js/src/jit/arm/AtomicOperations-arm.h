







#ifndef jit_arm_AtomicOperations_arm_h
#define jit_arm_AtomicOperations_arm_h

#include "jit/arm/Architecture-arm.h"
#include "jit/AtomicOperations.h"

#if defined(__clang__) || defined(__GNUC__)













inline bool
js::jit::AtomicOperations::isLockfree8()
{
    
    
    
    
    
    
    
    
    
    
# ifndef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    MOZ_ASSERT(__atomic_always_lock_free(sizeof(int8_t), 0));
    MOZ_ASSERT(__atomic_always_lock_free(sizeof(int16_t), 0));
    MOZ_ASSERT(__atomic_always_lock_free(sizeof(int32_t), 0));
    return HasLDSTREXBHD() && __atomic_always_lock_free(sizeof(int64_t), 0);
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
    __sync_synchronize();
    T v = *addr;
    __sync_synchronize();
# else
    T v;
    __atomic_load(addr, &v, __ATOMIC_SEQ_CST);
# endif
    return v;
}

template<typename T>
inline void
js::jit::AtomicOperations::storeSeqCst(T* addr, T val)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    __sync_synchronize();
    *addr = val;
    __sync_synchronize();
# else
    __atomic_store(addr, &val, __ATOMIC_SEQ_CST);
# endif
}

template<typename T>
inline T
js::jit::AtomicOperations::exchangeSeqCst(T* addr, T val)
{
    MOZ_ASSERT(sizeof(T) < 8 || isLockfree8());
# ifdef ATOMICS_IMPLEMENTED_WITH_SYNC_INTRINSICS
    T v;
    __sync_synchronize();
    do {
	v = *addr;
    } while (__sync_val_compare_and_swap(addr, v, val) != v);
    return v;
# else
    T v;
    __atomic_exchange(addr, &val, &v, __ATOMIC_SEQ_CST);
    return v;
# endif
}

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
        ;
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

#elif defined(ENABLE_SHARED_ARRAY_BUFFER)

# error "Either disable JS shared memory, use GCC or Clang, or add code here"

#endif

#endif 
