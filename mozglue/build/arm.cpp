





#include "arm.h"

#if defined(MOZILLA_ARM_HAVE_CPUID_DETECTION)






#  if defined(_MSC_VER)

#    define WIN32_LEAN_AND_MEAN
#    define WIN32_EXTRA_LEAN
#    include <windows.h>

#    if !defined(MOZILLA_PRESUME_EDSP)
static bool
check_edsp(void)
{
#      if defined(MOZILLA_MAY_SUPPORT_EDSP)
  __try
  {
    
    __emit(0xF5DDF000);
    return true;
  }
  __except(GetExceptionCode()==EXCEPTION_ILLEGAL_INSTRUCTION)
  {
    
  }
#      endif
  return false;
}
#    endif 

#    if !defined(MOZILLA_PRESUME_ARMV6)
static bool
check_armv6(void)
{
#      if defined(MOZILLA_MAY_SUPPORT_ARMV6)
  __try
  {
    
    __emit(0xE6333F93);
    return true;
  }
  __except(GetExceptionCode()==EXCEPTION_ILLEGAL_INSTRUCTION)
  {
    
  }
#      endif
  return false;
}
#    endif 

#    if !defined(MOZILLA_PRESUME_ARMV7)
static bool
check_armv7(void)
{
#      if defined(MOZILLA_MAY_SUPPORT_ARMV7)
  __try
  {
    
    
    
    
    emit(0xF57FF05E);
    return true;
  }
  __except(GetExceptionCode()==EXCEPTION_ILLEGAL_INSTRUCTION)
  {
    
  }
#      endif
  return false;
}
#    endif 

#    if !defined(MOZILLA_PRESUME_NEON)
static bool
check_neon(void)
{
#      if defined(MOZILLA_MAY_SUPPORT_NEON)
  __try
  {
    
    __emit(0xF2200150);
    return true;
  }
  __except(GetExceptionCode()==EXCEPTION_ILLEGAL_INSTRUCTION)
  {
    
  }
#      endif
  return false;
}
#    endif 

#  elif defined(__linux__) || defined(ANDROID)
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

enum{
  MOZILLA_HAS_EDSP_FLAG=1,
  MOZILLA_HAS_ARMV6_FLAG=2,
  MOZILLA_HAS_ARMV7_FLAG=4,
  MOZILLA_HAS_NEON_FLAG=8
};

static unsigned
get_arm_cpu_flags(void)
{
  unsigned  flags;
  FILE     *fin;
  bool      armv6_processor = false;
  flags = 0;
  




  fin = fopen ("/proc/cpuinfo","r");
  if (fin != nullptr)
  {
    

    char buf[512];
    while (fgets(buf, 511, fin) != nullptr)
    {
      if (memcmp(buf, "Features", 8) == 0)
      {
        char *p;
        p = strstr(buf, " edsp");
        if (p != nullptr && (p[5] == ' ' || p[5] == '\n'))
          flags |= MOZILLA_HAS_EDSP_FLAG;
        p = strstr(buf, " neon");
        if( p != nullptr && (p[5] == ' ' || p[5] == '\n'))
          flags |= MOZILLA_HAS_NEON_FLAG;
      }
      if (memcmp(buf, "CPU architecture:", 17) == 0)
      {
        int version;
        version = atoi(buf + 17);
        if (version >= 6)
          flags |= MOZILLA_HAS_ARMV6_FLAG;
        if (version >= 7)
          flags |= MOZILLA_HAS_ARMV7_FLAG;
      }
      








      if (memcmp(buf, "Processor\t:", 11) == 0) {
          if (strstr(buf, "(v6l)") != 0) {
              armv6_processor = true;
          }
      }
    }
    fclose(fin);
  }
  if (armv6_processor) {
      
      if (flags & MOZILLA_HAS_ARMV7_FLAG) {
          flags &= ~MOZILLA_HAS_ARMV7_FLAG;
      }
  }
  return flags;
}


static unsigned arm_cpu_flags = get_arm_cpu_flags();

#    if !defined(MOZILLA_PRESUME_EDSP)
static bool
check_edsp(void)
{
  return (arm_cpu_flags & MOZILLA_HAS_EDSP_FLAG) != 0;
}
#    endif

#    if !defined(MOZILLA_PRESUME_ARMV6)
static bool
check_armv6(void)
{
  return (arm_cpu_flags & MOZILLA_HAS_ARMV6_FLAG) != 0;
}
#    endif

#    if !defined(MOZILLA_PRESUME_ARMV7)
static bool
check_armv7(void)
{
  return (arm_cpu_flags & MOZILLA_HAS_ARMV7_FLAG) != 0;
}
#    endif

#    if !defined(MOZILLA_PRESUME_NEON)
static bool
check_neon(void)
{
  return (arm_cpu_flags & MOZILLA_HAS_NEON_FLAG) != 0;
}
#    endif

#  endif 

namespace mozilla {
  namespace arm_private {
#  if !defined(MOZILLA_PRESUME_EDSP)
    bool edsp_enabled = check_edsp();
#  endif
#  if !defined(MOZILLA_PRESUME_ARMV6)
    bool armv6_enabled = check_armv6();
#  endif
#  if !defined(MOZILLA_PRESUME_ARMV7)
    bool armv7_enabled = check_armv7();
#  endif
#  if !defined(MOZILLA_PRESUME_NEON)
    bool neon_enabled = check_neon();
#  endif
  } 
} 

#endif 
