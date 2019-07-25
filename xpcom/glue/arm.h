





































#ifndef mozilla_arm_h_
#define mozilla_arm_h_


#include "nscore.h"











#if defined(__GNUC__) && defined(__arm__)

#  define MOZILLA_ARM_ARCH 3

#  if defined(__ARM_ARCH_4__) || defined(__ARM_ARCH_4T__) \
   || defined(_ARM_ARCH_4)
#    undef MOZILLA_ARM_ARCH
#    define MOZILLA_ARM_ARCH 4
#  endif

#  if defined(__ARM_ARCH_5__) || defined(__ARM_ARCH_5T__) \
   || defined(__ARM_ARCH_5E__) || defined(__ARM_ARCH_5TE__) \
   || defined(__ARM_ARCH_5TEJ__) || defined(_ARM_ARCH_5)
#    undef MOZILLA_ARM_ARCH
#    define MOZILLA_ARM_ARCH 5
#  endif

#  if defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) \
   || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) \
   || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) \
   || defined(__ARM_ARCH_6M__) || defined(_ARM_ARCH_6)
#    undef MOZILLA_ARM_ARCH
#    define MOZILLA_ARM_ARCH 6
#  endif

#  if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) \
   || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) \
   || defined(__ARM_ARCH_7EM__) || defined(_ARM_ARCH_7)
#    undef MOZILLA_ARM_ARCH
#    define MOZILLA_ARM_ARCH 7
#  endif


#  if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#    define MOZILLA_MAY_SUPPORT_EDSP 1
#  endif

#  if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
#    if defined(HAVE_ARM_SIMD)
#      define MOZILLA_MAY_SUPPORT_ARMV6 1
#    endif
#  endif

  
  
  
  
#  if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2)
#    if defined(HAVE_ARM_NEON)
#      define MOZILLA_MAY_SUPPORT_NEON 1
#    endif
#  endif

  
#  if defined(__linux__) || defined(ANDROID)
#    define MOZILLA_ARM_HAVE_CPUID_DETECTION 1
#  endif

#elif defined(_MSC_VER) && defined(_M_ARM)

#  define MOZILLA_ARM_HAVE_CPUID_DETECTION 1
  
  
#  define MOZILLA_ARM_ARCH 3

  
  
#  define MOZILLA_MAY_SUPPORT_EDSP 1
#  if defined(HAVE_ARM_SIMD)
#    define MOZILLA_MAY_SUPPORT_ARMV6 1
#  endif
#  if defined(HAVE_ARM_SIMD)
#    define MOZILLA_MAY_SUPPORT_NEON 1
#  endif

#endif

namespace mozilla {

  namespace arm_private {
#if defined(MOZILLA_ARM_HAVE_CPUID_DETECTION)
#if !defined(MOZILLA_PRESUME_EDSP)
    extern bool NS_COM_GLUE edsp_enabled;
#endif
#if !defined(MOZILLA_PRESUME_ARMV6)
    extern bool NS_COM_GLUE armv6_enabled;
#endif
#if !defined(MOZILLA_PRESUME_NEON)
    extern bool NS_COM_GLUE neon_enabled;
#endif
#endif
  }

#if defined(MOZILLA_PRESUME_EDSP)
#  define MOZILLA_MAY_SUPPORT_EDSP 1
  inline bool supports_edsp() { return true; }
#elif defined(MOZILLA_MAY_SUPPORT_EDSP) \
   && defined(MOZILLA_ARM_HAVE_CPUID_DETECTION)
  inline bool supports_edsp() { return arm_private::edsp_enabled; }
#else
  inline bool supports_edsp() { return false; }
#endif

#if defined(MOZILLA_PRESUME_ARMV6)
#  define MOZILLA_MAY_SUPPORT_ARMV6 1
  inline bool supports_armv6() { return true; }
#elif defined(MOZILLA_MAY_SUPPORT_ARMV6) \
   && defined(MOZILLA_ARM_HAVE_CPUID_DETECTION)
  inline bool supports_armv6() { return arm_private::armv6_enabled; }
#else
  inline bool supports_armv6() { return false; }
#endif

#if defined(MOZILLA_PRESUME_NEON)
#  define MOZILLA_MAY_SUPPORT_NEON 1
  inline bool supports_neon() { return true; }
#elif defined(MOZILLA_MAY_SUPPORT_NEON) \
   && defined(MOZILLA_ARM_HAVE_CPUID_DETECTION)
  inline bool supports_neon() { return arm_private::neon_enabled; }
#else
  inline bool supports_neon() { return false; }
#endif

}

#endif 
