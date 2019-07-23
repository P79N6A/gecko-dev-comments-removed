






































#include "SSE.h"

namespace mozilla {

namespace sse_private {

#if defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)

#if !defined(MOZILLA_PRESUME_MMX)
  bool mmx_enabled
    = sse_private::has_cpuid_bit(1u, sse_private::edx, (1u<<23));
#endif

#if !defined(MOZILLA_PRESUME_SSE)
  bool sse_enabled
    = sse_private::has_cpuid_bit(1u, sse_private::edx, (1u<<25));
#endif

#if !defined(MOZILLA_PRESUME_SSE2)
  bool sse2_enabled
    = sse_private::has_cpuid_bit(1u, sse_private::edx, (1u<<26));
#endif

#if !defined(MOZILLA_PRESUME_SSE3)
  bool sse3_enabled
    = sse_private::has_cpuid_bit(1u, sse_private::ecx, (1u<<0));
#endif

#if !defined(MOZILLA_PRESUME_SSSE3)
  bool ssse3_enabled
    = sse_private::has_cpuid_bit(1u, sse_private::ecx, (1u<<9));
#endif

#if !defined(MOZILLA_PRESUME_SSE4A)
  bool sse4a_enabled
    = sse_private::has_cpuid_bit(0x80000001u, sse_private::ecx, (1u<<6));
#endif

#if !defined(MOZILLA_PRESUME_SSE4_1)
  bool sse4_1_enabled
    = sse_private::has_cpuid_bit(1u, sse_private::ecx, (1u<<19));
#endif

#if !defined(MOZILLA_PRESUME_SSE4_2)
  bool sse4_2_enabled
    = sse_private::has_cpuid_bit(1u, sse_private::ecx, (1u<<20));
#endif

#endif

}

}

