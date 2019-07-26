






#include "SSE.h"

namespace {






#ifdef HAVE_CPUID_H


#include <cpuid.h>

enum CPUIDRegister { eax = 0, ebx = 1, ecx = 2, edx = 3 };

static bool
has_cpuid_bit(unsigned int level, CPUIDRegister reg, unsigned int bit)
{
  unsigned int regs[4];
  return __get_cpuid(level, &regs[0], &regs[1], &regs[2], &regs[3]) &&
         (regs[reg] & bit);
}

#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64))


#include <intrin.h>

enum CPUIDRegister { eax = 0, ebx = 1, ecx = 2, edx = 3 };

static bool
has_cpuid_bit(unsigned int level, CPUIDRegister reg, unsigned int bit)
{
  
  int regs[4];
  __cpuid(regs, level & 0x80000000u);
  if (unsigned(regs[0]) < level)
    return false;

  __cpuid(regs, level);
  return !!(unsigned(regs[reg]) & bit);
}

#elif defined(__SUNPRO_CC) && (defined(__i386) || defined(__x86_64__))

enum CPUIDRegister { eax = 0, ebx = 1, ecx = 2, edx = 3 };

#ifdef __i386
static void
moz_cpuid(int CPUInfo[4], int InfoType)
{
  asm (
    "xchg %esi, %ebx\n"
    "cpuid\n"
    "movl %eax, (%edi)\n"
    "movl %ebx, 4(%edi)\n"
    "movl %ecx, 8(%edi)\n"
    "movl %edx, 12(%edi)\n"
    "xchg %esi, %ebx\n"
    :
    : "a"(InfoType), 
      "D"(CPUInfo) 
    : "%ecx", "%edx", "%esi"
  );
}
#else
static void
moz_cpuid(int CPUInfo[4], int InfoType)
{
  asm (
    "xchg %rsi, %rbx\n"
    "cpuid\n"
    "movl %eax, (%rdi)\n"
    "movl %ebx, 4(%rdi)\n"
    "movl %ecx, 8(%rdi)\n"
    "movl %edx, 12(%rdi)\n"
    "xchg %rsi, %rbx\n"
    :
    : "a"(InfoType), 
      "D"(CPUInfo) 
    : "%ecx", "%edx", "%rsi"
  );
}
#endif

static bool
has_cpuid_bit(unsigned int level, CPUIDRegister reg, unsigned int bit)
{
  
  volatile int regs[4];
  moz_cpuid((int *)regs, level & 0x80000000u);
  if (unsigned(regs[0]) < level)
    return false;

  moz_cpuid((int *)regs, level);
  return !!(unsigned(regs[reg]) & bit);
}

#endif 

}

namespace mozilla {

namespace sse_private {

#if defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)

#if !defined(MOZILLA_PRESUME_MMX)
  bool mmx_enabled = has_cpuid_bit(1u, edx, (1u<<23));
#endif

#if !defined(MOZILLA_PRESUME_SSE)
  bool sse_enabled = has_cpuid_bit(1u, edx, (1u<<25));
#endif

#if !defined(MOZILLA_PRESUME_SSE2)
  bool sse2_enabled = has_cpuid_bit(1u, edx, (1u<<26));
#endif

#if !defined(MOZILLA_PRESUME_SSE3)
  bool sse3_enabled = has_cpuid_bit(1u, ecx, (1u<<0));
#endif

#if !defined(MOZILLA_PRESUME_SSSE3)
  bool ssse3_enabled = has_cpuid_bit(1u, ecx, (1u<<9));
#endif

#if !defined(MOZILLA_PRESUME_SSE4A)
  bool sse4a_enabled = has_cpuid_bit(0x80000001u, ecx, (1u<<6));
#endif

#if !defined(MOZILLA_PRESUME_SSE4_1)
  bool sse4_1_enabled = has_cpuid_bit(1u, ecx, (1u<<19));
#endif

#if !defined(MOZILLA_PRESUME_SSE4_2)
  bool sse4_2_enabled = has_cpuid_bit(1u, ecx, (1u<<20));
#endif

#endif

} 
} 
