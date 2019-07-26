















#ifndef ANDROID_CUTILS_ATOMIC_H
#define ANDROID_CUTILS_ATOMIC_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif








void android_atomic_write(int32_t value, volatile int32_t* addr);






int32_t android_atomic_inc(volatile int32_t* addr);
int32_t android_atomic_dec(volatile int32_t* addr);

int32_t android_atomic_add(int32_t value, volatile int32_t* addr);
int32_t android_atomic_and(int32_t value, volatile int32_t* addr);
int32_t android_atomic_or(int32_t value, volatile int32_t* addr);

int32_t android_atomic_swap(int32_t value, volatile int32_t* addr);










int64_t android_quasiatomic_swap_64(int64_t value, volatile int64_t* addr);
int64_t android_quasiatomic_read_64(volatile int64_t* addr);
    





int android_atomic_cmpxchg(int32_t oldvalue, int32_t newvalue,
        volatile int32_t* addr);

int android_quasiatomic_cmpxchg_64(int64_t oldvalue, int64_t newvalue,
        volatile int64_t* addr);


    
#ifdef __cplusplus
} 
#endif

#endif
