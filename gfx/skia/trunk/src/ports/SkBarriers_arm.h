






#ifndef SkBarriers_arm_DEFINED
#define SkBarriers_arm_DEFINED

static inline void sk_compiler_barrier() { asm volatile("" : : : "memory"); }

template <typename T>
T sk_acquire_load(T* ptr) {
    T val = *ptr;
    __sync_synchronize();  
    return val;
}

template <typename T>
T sk_consume_load(T* ptr) {
    T val = *ptr;
    
    
    
    sk_compiler_barrier();
    return val;
}

template <typename T>
void sk_release_store(T* ptr, T val) {
    __sync_synchronize();  
    *ptr = val;
}

#endif
