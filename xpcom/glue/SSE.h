







































#ifndef mozilla_SSE_h_
#define mozilla_SSE_h_


#include "nscore.h"














































































#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))

#ifdef __MMX__
  
  
  #define MOZILLA_PRESUME_MMX 1
#endif
#ifdef __SSE__
  
  
  #define MOZILLA_PRESUME_SSE 1
#endif
#ifdef __SSE2__
  
  
  #define MOZILLA_PRESUME_SSE2 1
#endif
#ifdef __SSE3__
  
  
  #define MOZILLA_PRESUME_SSE3 1
#endif
#ifdef __SSSE3__
  
  #define MOZILLA_PRESUME_SSSE3 1
#endif
#ifdef __SSE4A__
  
  #define MOZILLA_PRESUME_SSE4A 1
#endif
#ifdef __SSE4_1__
  
  #define MOZILLA_PRESUME_SSE4_1 1
#endif
#ifdef __SSE4_2__
  
  #define MOZILLA_PRESUME_SSE4_2 1
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
  #define MOZILLA_SSE_HAVE_CPUID_DETECTION
#endif

#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64))

#if _MSC_VER >= 1400
  
  #define MOZILLA_SSE_HAVE_CPUID_DETECTION
#endif

#if defined(_M_IX86_FP)

#if _M_IX86_FP >= 1
  
  #define MOZILLA_PRESUME_SSE
#endif
#if _M_IX86_FP >= 2
  
  #define MOZILLA_PRESUME_SSE2
#endif

#elif defined(_M_AMD64)
  

  
  #define MOZILLA_PRESUME_SSE

  #define MOZILLA_PRESUME_SSE2
#endif

#elif defined(__SUNPRO_CC) && (defined(__i386) || defined(__x86_64__))


#define MOZILLA_SSE_HAVE_CPUID_DETECTION

#if defined(__x86_64__)
  
  #define MOZILLA_PRESUME_MMX

  #define MOZILLA_PRESUME_SSE
  
  #define MOZILLA_PRESUME_SSE2
#endif

#endif

namespace mozilla {

  namespace sse_private {
#if defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#if !defined(MOZILLA_PRESUME_MMX)
    extern bool NS_COM_GLUE mmx_enabled;
#endif
#if !defined(MOZILLA_PRESUME_SSE)
    extern bool NS_COM_GLUE sse_enabled;
#endif
#if !defined(MOZILLA_PRESUME_SSE2)
    extern bool NS_COM_GLUE sse2_enabled;
#endif
#if !defined(MOZILLA_PRESUME_SSE3)
    extern bool NS_COM_GLUE sse3_enabled;
#endif
#if !defined(MOZILLA_PRESUME_SSSE3)
    extern bool NS_COM_GLUE ssse3_enabled;
#endif
#if !defined(MOZILLA_PRESUME_SSE4A)
    extern bool NS_COM_GLUE sse4a_enabled;
#endif
#if !defined(MOZILLA_PRESUME_SSE4_1)
    extern bool NS_COM_GLUE sse4_1_enabled;
#endif
#if !defined(MOZILLA_PRESUME_SSE4_2)
    extern bool NS_COM_GLUE sse4_2_enabled;
#endif
#endif
  }

#if defined(MOZILLA_PRESUME_MMX)
#define MOZILLA_MAY_SUPPORT_MMX 1
  inline bool supports_mmx() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#if !(defined(_MSC_VER) && defined(_M_AMD64))
  
  
#define MOZILLA_MAY_SUPPORT_MMX 1
#endif
  inline bool supports_mmx() { return sse_private::mmx_enabled; }
#else
  inline bool supports_mmx() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE)
#define MOZILLA_MAY_SUPPORT_SSE 1
  inline bool supports_sse() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#define MOZILLA_MAY_SUPPORT_SSE 1
  inline bool supports_sse() { return sse_private::sse_enabled; }
#else
  inline bool supports_sse() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE2)
#define MOZILLA_MAY_SUPPORT_SSE2 1
  inline bool supports_sse2() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#define MOZILLA_MAY_SUPPORT_SSE2 1
  inline bool supports_sse2() { return sse_private::sse2_enabled; }
#else
  inline bool supports_sse2() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE3)
#define MOZILLA_MAY_SUPPORT_SSE3 1
  inline bool supports_sse3() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#define MOZILLA_MAY_SUPPORT_SSE3 1
  inline bool supports_sse3() { return sse_private::sse3_enabled; }
#else
  inline bool supports_sse3() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSSE3)
#define MOZILLA_MAY_SUPPORT_SSSE3 1
  inline bool supports_ssse3() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#define MOZILLA_MAY_SUPPORT_SSSE3 1
  inline bool supports_ssse3() { return sse_private::ssse3_enabled; }
#else
  inline bool supports_ssse3() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE4A)
#define MOZILLA_MAY_SUPPORT_SSE4A 1
  inline bool supports_sse4a() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#define MOZILLA_MAY_SUPPORT_SSE4A 1
  inline bool supports_sse4a() { return sse_private::sse4a_enabled; }
#else
  inline bool supports_sse4a() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE4_1)
#define MOZILLA_MAY_SUPPORT_SSE4_1 1
  inline bool supports_sse4_1() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#define MOZILLA_MAY_SUPPORT_SSE4_1 1
  inline bool supports_sse4_1() { return sse_private::sse4_1_enabled; }
#else
  inline bool supports_sse4_1() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE4_2)
#define MOZILLA_MAY_SUPPORT_SSE4_2 1
  inline bool supports_sse4_2() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
#define MOZILLA_MAY_SUPPORT_SSE4_2 1
  inline bool supports_sse4_2() { return sse_private::sse4_2_enabled; }
#else
  inline bool supports_sse4_2() { return false; }
#endif

}

#endif 
