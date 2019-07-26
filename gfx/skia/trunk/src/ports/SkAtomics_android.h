






#ifndef SkAtomics_android_DEFINED
#define SkAtomics_android_DEFINED



#include <cutils/atomic.h>
#include <stdint.h>

static inline __attribute__((always_inline)) int32_t sk_atomic_inc(int32_t* addr) {
    return android_atomic_inc(addr);
}

static inline __attribute__((always_inline)) int32_t sk_atomic_add(int32_t* addr, int32_t inc) {
    return android_atomic_add(inc, addr);
}

static inline __attribute__((always_inline)) int32_t sk_atomic_dec(int32_t* addr) {
    return android_atomic_dec(addr);
}

static inline __attribute__((always_inline)) void sk_membar_acquire__after_atomic_dec() {
    
    
    
    
}

static inline __attribute__((always_inline)) int32_t sk_atomic_conditional_inc(int32_t* addr) {
    while (true) {
        int32_t value = *addr;
        if (value == 0) {
            return 0;
        }
        if (0 == android_atomic_release_cas(value, value + 1, addr)) {
            return value;
        }
    }
}

static inline __attribute__((always_inline)) bool sk_atomic_cas(int32_t* addr,
                                                                 int32_t before,
                                                                 int32_t after) {
    
    return android_atomic_release_cas(before, after, addr) == 0;
}

static inline __attribute__((always_inline)) void sk_membar_acquire__after_atomic_conditional_inc() {
    
    
    
    
}

#endif
