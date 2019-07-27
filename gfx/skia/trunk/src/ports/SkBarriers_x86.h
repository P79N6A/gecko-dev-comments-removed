






#ifndef SkBarriers_x86_DEFINED
#define SkBarriers_x86_DEFINED

#ifdef SK_BUILD_FOR_WIN
#  include <intrin.h>
static inline void sk_compiler_barrier() { _ReadWriteBarrier(); }
#else
static inline void sk_compiler_barrier() { asm volatile("" : : : "memory"); }
#endif

template <typename T>
T sk_acquire_load(T* ptr) {
    T val = *ptr;
    
    sk_compiler_barrier();
    return val;
}

template <typename T>
T sk_consume_load(T* ptr) {
    
    return sk_acquire_load(ptr);
}

template <typename T>
void sk_release_store(T* ptr, T val) {
    
    sk_compiler_barrier();
    *ptr = val;
}

#endif
