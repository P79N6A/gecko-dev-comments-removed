







#ifndef jit_mips_AtomicOperations_mips_h
#define jit_mips_AtomicOperations_mips_h

#include "jit/AtomicOperations.h"

inline bool
js::jit::AtomicOperations::isLockfree8()
{
    
    
    return false;
}

inline void
js::jit::AtomicOperations::fenceSeqCst()
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::loadSeqCst(T* addr)
{
    MOZ_CRASH();
}

template<typename T>
inline void
js::jit::AtomicOperations::storeSeqCst(T* addr, T val)
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::compareExchangeSeqCst(T* addr, T oldval, T newval)
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchAddSeqCst(T* addr, T val)
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchSubSeqCst(T* addr, T val)
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchAndSeqCst(T* addr, T val)
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchOrSeqCst(T* addr, T val)
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::fetchXorSeqCst(T* addr, T val)
{
    MOZ_CRASH();
}

template<typename T>
inline T
js::jit::AtomicOperations::exchangeSeqCst(T* addr, T val)
{
    MOZ_CRASH();
}

template<size_t nbytes>
inline void
js::jit::RegionLock::acquire(void* addr)
{
    MOZ_CRASH();
}

template<size_t nbytes>
inline void
js::jit::RegionLock::release(void* addr)
{
    MOZ_CRASH();
}

#endif 
