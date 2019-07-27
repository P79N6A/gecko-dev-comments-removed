









#include <stdlib.h>
#include <string.h>
#include "arm.h"

static int arm_cpu_env_flags(int *flags) {
  char *env;
  env = getenv("VPX_SIMD_CAPS");
  if (env && *env) {
    *flags = (int)strtol(env, NULL, 0);
    return 0;
  }
  *flags = 0;
  return -1;
}

static int arm_cpu_env_mask(void) {
  char *env;
  env = getenv("VPX_SIMD_CAPS_MASK");
  return env && *env ? (int)strtol(env, NULL, 0) : ~0;
}

#if !CONFIG_RUNTIME_CPU_DETECT

int arm_cpu_caps(void) {
  


  int flags;
  int mask;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
#if HAVE_EDSP
  flags |= HAS_EDSP;
#endif 
#if HAVE_MEDIA
  flags |= HAS_MEDIA;
#endif 
#if HAVE_NEON
  flags |= HAS_NEON;
#endif 
  return flags & mask;
}

#elif defined(_MSC_VER) 

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

int arm_cpu_caps(void) {
  int flags;
  int mask;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
  



#if HAVE_EDSP
  if (mask & HAS_EDSP) {
    __try {
      
      __emit(0xF5DDF000);
      flags |= HAS_EDSP;
    } __except (GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
      
    }
  }
#if HAVE_MEDIA
  if (mask & HAS_MEDIA)
    __try {
      
      __emit(0xE6333F93);
      flags |= HAS_MEDIA;
    } __except (GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
    
  }
}
#if HAVE_NEON
if (mask &HAS_NEON) {
  __try {
    
    __emit(0xF2200150);
    flags |= HAS_NEON;
  } __except (GetExceptionCode() == EXCEPTION_ILLEGAL_INSTRUCTION) {
    
  }
}
#endif 
#endif 
#endif 
return flags & mask;
}

#elif defined(__ANDROID__) 
#include <cpu-features.h>

int arm_cpu_caps(void) {
  int flags;
  int mask;
  uint64_t features;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
  features = android_getCpuFeatures();

#if HAVE_EDSP
  flags |= HAS_EDSP;
#endif 
#if HAVE_MEDIA
  flags |= HAS_MEDIA;
#endif 
#if HAVE_NEON
  if (features & ANDROID_CPU_ARM_FEATURE_NEON)
    flags |= HAS_NEON;
#endif 
  return flags & mask;
}

#elif defined(__linux__) 

#include <stdio.h>

int arm_cpu_caps(void) {
  FILE *fin;
  int flags;
  int mask;
  if (!arm_cpu_env_flags(&flags)) {
    return flags;
  }
  mask = arm_cpu_env_mask();
  



  fin = fopen("/proc/cpuinfo", "r");
  if (fin != NULL) {
    


    char buf[512];
    while (fgets(buf, 511, fin) != NULL) {
#if HAVE_EDSP || HAVE_NEON
      if (memcmp(buf, "Features", 8) == 0) {
        char *p;
#if HAVE_EDSP
        p = strstr(buf, " edsp");
        if (p != NULL && (p[5] == ' ' || p[5] == '\n')) {
          flags |= HAS_EDSP;
        }
#if HAVE_NEON
        p = strstr(buf, " neon");
        if (p != NULL && (p[5] == ' ' || p[5] == '\n')) {
          flags |= HAS_NEON;
        }
#endif 
#endif 
      }
#endif 
#if HAVE_MEDIA
      if (memcmp(buf, "CPU architecture:", 17) == 0) {
        int version;
        version = atoi(buf + 17);
        if (version >= 6) {
          flags |= HAS_MEDIA;
        }
      }
#endif 
    }
    fclose(fin);
  }
  return flags & mask;
}
#else 
#error "--enable-runtime-cpu-detect selected, but no CPU detection method " \
"available for your platform. Reconfigure with --disable-runtime-cpu-detect."
#endif
