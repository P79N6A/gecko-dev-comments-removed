









#include "libyuv/cpu_id.h"

#include <stdlib.h>  
#ifdef _MSC_VER
#include <intrin.h>
#endif
#ifdef __ANDROID__
#include <cpu-features.h>
#endif

#include "libyuv/basic_types.h"  


#if (defined(__pic__) || defined(__APPLE__)) && defined(__i386__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (
    "mov %%ebx, %%edi                          \n"
    "cpuid                                     \n"
    "xchg %%edi, %%ebx                         \n"
    : "=a"(cpu_info[0]), "=D"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type)
  );
}
#elif defined(__i386__) || defined(__x86_64__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (
    "cpuid                                     \n"
    : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type)
  );
}
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


int cpu_info_ = 0;

int InitCpuFlags() {
#ifdef CPU_X86
  int cpu_info[4];
  __cpuid(cpu_info, 1);
  cpu_info_ = (cpu_info[3] & 0x04000000 ? kCpuHasSSE2 : 0) |
              (cpu_info[2] & 0x00000200 ? kCpuHasSSSE3 : 0) |
              kCpuInitialized;

  
  if (getenv("LIBYUV_DISABLE_SSE2")) {
    cpu_info_ &= ~kCpuHasSSE2;
  }
  
  if (getenv("LIBYUV_DISABLE_SSSE3")) {
    cpu_info_ &= ~kCpuHasSSSE3;
  }
#elif defined(__ANDROID__) && defined(__ARM_NEON__)
  uint64_t features = android_getCpuFeatures();
  cpu_info_ = ((features & ANDROID_CPU_ARM_FEATURE_NEON) ? kCpuHasNEON : 0) |
              kCpuInitialized;
#elif defined(__ARM_NEON__)
  
  
  
  cpu_info_ = kCpuHasNEON | kCpuInitialized;
#else
  cpu_info_ = kCpuInitialized;
#endif
  return cpu_info_;
}

void MaskCpuFlags(int enable_flags) {
  InitCpuFlags();
  cpu_info_ = (cpu_info_ & enable_flags) | kCpuInitialized;
}

#ifdef __cplusplus
}  
}  
#endif
