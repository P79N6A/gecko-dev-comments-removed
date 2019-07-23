






































#ifndef mozilla_SSE_h_
#define mozilla_SSE_h_


#include "nscore.h"
































































































#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))




#if 0
  
  
  
  
  
  #define MOZILLA_SSE_HAVE_PRAGMA_TARGET
#endif

#ifdef MOZILLA_SSE_HAVE_PRAGMA_TARGET
  
  
  
  #if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1) 
    #define MOZILLA_COMPILE_WITH_MMX 1
    #define MOZILLA_COMPILE_WITH_SSE 1
  #endif 
  #if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) 
    #define MOZILLA_COMPILE_WITH_SSE2 1
    #define MOZILLA_COMPILE_WITH_SSE3 1
  #endif 
  #if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) 
    #define MOZILLA_COMPILE_WITH_SSSE3 1
    #define MOZILLA_COMPILE_WITH_SSE4A 1
    #define MOZILLA_COMPILE_WITH_SSE4_1 1
    #define MOZILLA_COMPILE_WITH_SSE4_2 1
  #endif 
  
  
  
#else
  #ifdef __MMX__
    #define MOZILLA_COMPILE_WITH_MMX 1
  #endif
  #ifdef __SSE__
    #define MOZILLA_COMPILE_WITH_SSE 1
  #endif
  #ifdef __SSE2__
    #define MOZILLA_COMPILE_WITH_SSE2 1
  #endif
  #ifdef __SSE3__
    #define MOZILLA_COMPILE_WITH_SSE3 1
  #endif
  #ifdef __SSSE3__
    #define MOZILLA_COMPILE_WITH_SSSE3 1
  #endif
  #ifdef __SSE4A__
    #define MOZILLA_COMPILE_WITH_SSE4A 1
  #endif
  #ifdef __SSE4_1__
    #define MOZILLA_COMPILE_WITH_SSE4_1 1
  #endif
  #ifdef __SSE4_2__
    #define MOZILLA_COMPILE_WITH_SSE4_2 1
  #endif
#endif

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


#include <cpuid.h>
#define MOZILLA_SSE_HAVE_CPUID_DETECTION

namespace mozilla {

  namespace sse_private {

    enum CPUIDRegister { eax = 0, ebx = 1, ecx = 2, edx = 3 };

    inline bool
    has_cpuid_bit(unsigned int level, CPUIDRegister reg, unsigned int bit)
    {
      unsigned int regs[4];
      return __get_cpuid(level, &regs[0], &regs[1], &regs[2], &regs[3]) &&
             (regs[reg] & bit);
    }

  }

}

#endif











#ifdef MOZILLA_SSE_HAVE_PRAGMA_TARGET
#pragma GCC push_options
#endif

#if defined(MOZILLA_COMPILE_WITH_MMX) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_MMX)
  #if !defined(__MMX__)
    #pragma GCC target ("mmx")
  #endif
  #include <mmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE)
  #if !defined(__SSE__)
    #pragma GCC target ("sse")
  #endif
  #include <xmmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE2) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE2)
  #if !defined(__SSE2__)
    #pragma GCC target ("sse2")
  #endif
  #include <emmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE3) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE3)
  #if !defined(__SSE3__)
    #pragma GCC target ("sse3")
  #endif
  #include <pmmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSSE3) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSSE3)
  #if !defined(__SSSE3__)
    #pragma GCC target ("ssse3")
  #endif
  #include <tmmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE4A) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE4A)
  #if !defined(__SSE4A__)
    #pragma GCC target ("sse4a")
  #endif
  #include <ammintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE4_1) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE4_1)
  #if !defined(__SSE4_1__)
    #pragma GCC target ("sse4.1")
  #endif
  #include <smmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE4_2) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE4_2)
  #if !defined(__SSE4_2__)
    #pragma GCC target ("sse4.2")
  #endif
  #include <nmmintrin.h>
#endif

#ifdef MOZILLA_SSE_HAVE_PRAGMA_TARGET
#pragma GCC pop_options
#endif

#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_AMD64))




#if 1 
#define MOZILLA_COMPILE_WITH_MMX 1
#define MOZILLA_COMPILE_WITH_SSE 1
#define MOZILLA_COMPILE_WITH_SSE2 1
#endif 

#if _MSC_VER >= 1400
#include <intrin.h>
#define MOZILLA_SSE_HAVE_CPUID_DETECTION

namespace mozilla {

  namespace sse_private {

    enum CPUIDRegister { eax = 0, ebx = 1, ecx = 2, edx = 3 };

    inline bool
    has_cpuid_bit(unsigned int level, CPUIDRegister reg, unsigned int bit)
    {
      
      int regs[4];
      __cpuid(regs, level & 0x80000000u);
      if (unsigned(regs[0]) < level)
        return false;

      __cpuid(regs, level);
      return !!(unsigned(regs[reg]) & bit);
    }

  }

}

#endif

#if defined(_M_AMD64)
  
  #define MOZILLA_PRESUME_MMX

  #define MOZILLA_PRESUME_SSE
  
  #define MOZILLA_PRESUME_SSE2
#endif

#if defined(MOZILLA_COMPILE_WITH_MMX) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_MMX)
#include <mmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE)
#include <xmmintrin.h>
#endif

#if defined(MOZILLA_COMPILE_WITH_SSE2) && \
    defined(MOZILLA_SSE_INCLUDE_HEADER_FOR_SSE2)
#include <emmintrin.h>
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
  inline bool supports_mmx() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_mmx() { return sse_private::mmx_enabled; }
#else
  inline bool supports_mmx() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE)
  inline bool supports_sse() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_sse() { return sse_private::sse_enabled; }
#else
  inline bool supports_sse() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE2)
  inline bool supports_sse2() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_sse2() { return sse_private::sse2_enabled; }
#else
  inline bool supports_sse2() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE3)
  inline bool supports_sse3() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_sse3() { return sse_private::sse3_enabled; }
#else
  inline bool supports_sse3() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSSE3)
  inline bool supports_ssse3() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_ssse3() { return sse_private::ssse3_enabled; }
#else
  inline bool supports_ssse3() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE4A)
  inline bool supports_sse4a() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_sse4a() { return sse_private::sse4a_enabled; }
#else
  inline bool supports_sse4a() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE4_1)
  inline bool supports_sse4_1() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_sse4_1() { return sse_private::sse4_1_enabled; }
#else
  inline bool supports_sse4_1() { return false; }
#endif

#if defined(MOZILLA_PRESUME_SSE4_2)
  inline bool supports_sse4_2() { return true; }
#elif defined(MOZILLA_SSE_HAVE_CPUID_DETECTION)
  inline bool supports_sse4_2() { return sse_private::sse4_2_enabled; }
#else
  inline bool supports_sse4_2() { return false; }
#endif



#ifdef MOZILLA_COMPILE_WITH_MMX
  inline bool use_mmx() { return supports_mmx(); }
#else
  inline bool use_mmx() { return false; }
#endif

#ifdef MOZILLA_COMPILE_WITH_SSE
  inline bool use_sse() { return supports_sse(); }
#else
  inline bool use_sse() { return false; }
#endif

#ifdef MOZILLA_COMPILE_WITH_SSE2
  inline bool use_sse2() { return supports_sse2(); }
#else
  inline bool use_sse2() { return false; }
#endif

#ifdef MOZILLA_COMPILE_WITH_SSE3
  inline bool use_sse3() { return supports_sse3(); }
#else
  inline bool use_sse3() { return false; }
#endif

#ifdef MOZILLA_COMPILE_WITH_SSSE3
  inline bool use_ssse3() { return supports_ssse3(); }
#else
  inline bool use_ssse3() { return false; }
#endif

#ifdef MOZILLA_COMPILE_WITH_SSE4a
  inline bool use_sse4a() { return supports_sse4a(); }
#else
  inline bool use_sse4a() { return false; }
#endif

#ifdef MOZILLA_COMPILE_WITH_SSE4_1
  inline bool use_sse4_1() { return supports_sse4_1(); }
#else
  inline bool use_sse4_1() { return false; }
#endif

#ifdef MOZILLA_COMPILE_WITH_SSE4_2
  inline bool use_sse4_2() { return supports_sse4_2(); }
#else
  inline bool use_sse4_2() { return false; }
#endif

}

#endif 
