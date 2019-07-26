






#ifndef SkAtomics_win_DEFINED
#define SkAtomics_win_DEFINED



#include <intrin.h>
#include <stdint.h>





#pragma intrinsic(_InterlockedIncrement, _InterlockedExchangeAdd, _InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange)

static inline int32_t sk_atomic_inc(int32_t* addr) {
    
    return _InterlockedIncrement(reinterpret_cast<long*>(addr)) - 1;
}

static inline int32_t sk_atomic_add(int32_t* addr, int32_t inc) {
    return _InterlockedExchangeAdd(reinterpret_cast<long*>(addr), static_cast<long>(inc));
}

static inline int32_t sk_atomic_dec(int32_t* addr) {
    
    return _InterlockedDecrement(reinterpret_cast<long*>(addr)) + 1;
}

static inline void sk_membar_acquire__after_atomic_dec() { }

static inline int32_t sk_atomic_conditional_inc(int32_t* addr) {
    long value = *addr;
    while (true) {
        if (value == 0) {
            return 0;
        }

        long before = _InterlockedCompareExchange(reinterpret_cast<long*>(addr), value + 1, value);

        if (before == value) {
            return value;
        } else {
            value = before;
        }
    }
}

static inline bool sk_atomic_cas(int32_t* addr, int32_t before, int32_t after) {
    return _InterlockedCompareExchange(reinterpret_cast<long*>(addr), after, before) == before;
}

static inline void sk_membar_acquire__after_atomic_conditional_inc() { }

#endif
