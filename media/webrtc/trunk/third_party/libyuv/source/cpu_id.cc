









#include "libyuv/cpu_id.h"

#ifdef _MSC_VER
#include <intrin.h>  
#endif
#if !defined(__CLR_VER) && defined(_M_X64) && \
    defined(_MSC_VER) && (_MSC_FULL_VER >= 160040219)
#include <immintrin.h>  
#endif

#include <stdlib.h>  


#include <stdio.h>
#include <string.h>

#include "libyuv/basic_types.h"  


#if (defined(__pic__) || defined(__APPLE__)) && defined(__i386__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (
    "mov %%ebx, %%edi                          \n"
    "cpuid                                     \n"
    "xchg %%edi, %%ebx                         \n"
    : "=a"(cpu_info[0]), "=D"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type));
}
#elif defined(__i386__) || defined(__x86_64__)
static __inline void __cpuid(int cpu_info[4], int info_type) {
  asm volatile (
    "cpuid                                     \n"
    : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
    : "a"(info_type));
}
#endif

#ifdef __cplusplus
namespace libyuv {
extern "C" {
#endif


#if !defined(__CLR_VER) && (defined(_M_IX86) || defined(_M_X64) || \
    defined(__i386__) || defined(__x86_64__))
LIBYUV_API
void CpuId(int cpu_info[4], int info_type) {
  __cpuid(cpu_info, info_type);
}
#else
LIBYUV_API
void CpuId(int cpu_info[4], int) {
  cpu_info[0] = cpu_info[1] = cpu_info[2] = cpu_info[3] = 0;
}
#endif


#if !defined(__CLR_VER) && defined(_M_X64) && \
    defined(_MSC_VER) && (_MSC_FULL_VER >= 160040219)
#define HAS_XGETBV
static uint32 XGetBV(unsigned int xcr) {
  return static_cast<uint32>(_xgetbv(xcr));
}
#elif !defined(__CLR_VER) && defined(_M_IX86)
#define HAS_XGETBV
__declspec(naked) __declspec(align(16))
static uint32 XGetBV(unsigned int xcr) {
  __asm {
    mov        ecx, [esp + 4]    
    _asm _emit 0x0f _asm _emit 0x01 _asm _emit 0xd0  
    ret
  }
}
#elif defined(__i386__) || defined(__x86_64__)
#define HAS_XGETBV
static uint32 XGetBV(unsigned int xcr) {
  uint32 xcr_feature_mask;
  asm volatile (
    ".byte 0x0f, 0x01, 0xd0\n"
    : "=a"(xcr_feature_mask)
    : "c"(xcr)
    : "memory", "cc", "edx");  
  return xcr_feature_mask;
}
#endif
#ifdef HAS_XGETBV
static const int kXCR_XFEATURE_ENABLED_MASK = 0;
#endif



LIBYUV_API
int ArmCpuCaps(const char* cpuinfo_name) {
  int flags = 0;
  FILE* fin = fopen(cpuinfo_name, "r");
  if (fin) {
    char buf[512];
    while (fgets(buf, 511, fin)) {
      if (memcmp(buf, "Features", 8) == 0) {
        flags |= kCpuInitialized;
        char* p = strstr(buf, " neon");
        if (p && (p[5] == ' ' || p[5] == '\n')) {
          flags |= kCpuHasNEON;
          break;
        }
      }
    }
    fclose(fin);
  }
  return flags;
}


LIBYUV_API
int cpu_info_ = 0;

LIBYUV_API
int InitCpuFlags(void) {
#if !defined(__CLR_VER) && defined(CPU_X86)
  int cpu_info[4];
  __cpuid(cpu_info, 1);
  cpu_info_ = ((cpu_info[3] & 0x04000000) ? kCpuHasSSE2 : 0) |
              ((cpu_info[2] & 0x00000200) ? kCpuHasSSSE3 : 0) |
              ((cpu_info[2] & 0x00080000) ? kCpuHasSSE41 : 0) |
              ((cpu_info[2] & 0x00100000) ? kCpuHasSSE42 : 0) |
              (((cpu_info[2] & 0x18000000) == 0x18000000) ? kCpuHasAVX : 0) |
              kCpuInitialized | kCpuHasX86;
#ifdef HAS_XGETBV
  if (cpu_info_ & kCpuHasAVX) {
    __cpuid(cpu_info, 7);
    if ((cpu_info[1] & 0x00000020) &&
        ((XGetBV(kXCR_XFEATURE_ENABLED_MASK) & 0x06) == 0x06)) {
      cpu_info_ |= kCpuHasAVX2;
    }
  }
#endif

  
  if (getenv("LIBYUV_DISABLE_X86")) {
    cpu_info_ &= ~kCpuHasX86;
  }
  if (getenv("LIBYUV_DISABLE_SSE2")) {
    cpu_info_ &= ~kCpuHasSSE2;
  }
  if (getenv("LIBYUV_DISABLE_SSSE3")) {
    cpu_info_ &= ~kCpuHasSSSE3;
  }
  if (getenv("LIBYUV_DISABLE_SSE41")) {
    cpu_info_ &= ~kCpuHasSSE41;
  }
  if (getenv("LIBYUV_DISABLE_SSE42")) {
    cpu_info_ &= ~kCpuHasSSE42;
  }
  if (getenv("LIBYUV_DISABLE_AVX")) {
    cpu_info_ &= ~kCpuHasAVX;
  }
  if (getenv("LIBYUV_DISABLE_AVX2")) {
    cpu_info_ &= ~kCpuHasAVX2;
  }
  if (getenv("LIBYUV_DISABLE_ASM")) {
    cpu_info_ = kCpuInitialized;
  }
#elif defined(__arm__)
#if defined(__linux__) && defined(__ARM_NEON__)
  
  cpu_info_ = ArmCpuCaps("/proc/cpuinfo");
#elif defined(__ARM_NEON__)
  
  
  
  cpu_info_ = kCpuHasNEON;
#endif
  cpu_info_ |= kCpuInitialized | kCpuHasARM;
#endif
  return cpu_info_;
}

LIBYUV_API
void MaskCpuFlags(int enable_flags) {
  InitCpuFlags();
  cpu_info_ = (cpu_info_ & enable_flags) | kCpuInitialized;
}

#ifdef __cplusplus
}  
}  
#endif
