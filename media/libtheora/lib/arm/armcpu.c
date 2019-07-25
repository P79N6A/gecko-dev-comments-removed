


















#include "armcpu.h"

#if !defined(OC_ARM_ASM)|| \
 !defined(OC_ARM_ASM_EDSP)&&!defined(OC_ARM_ASM_ARMV6)&& \
 !defined(OC_ARM_ASM_NEON)
ogg_uint32_t oc_cpu_flags_get(void){
  return 0;
}

#elif defined(_MSC_VER)

# define WIN32_LEAN_AND_MEAN
# define WIN32_EXTRA_LEAN
# include <windows.h>

ogg_uint32_t oc_cpu_flags_get(void){
  ogg_uint32_t flags;
  flags=0;
  


# if defined(OC_ARM_ASM_EDSP)
  __try{
    
    __emit(0xF5DDF000);
    flags|=OC_CPU_ARM_EDSP;
  }
  __except(GetExceptionCode()==EXCEPTION_ILLEGAL_INSTRUCTION){
    
  }
#  if defined(OC_ARM_ASM_MEDIA)
  __try{
    
    __emit(0xE6333F93);
    flags|=OC_CPU_ARM_MEDIA;
  }
  __except(GetExceptionCode()==EXCEPTION_ILLEGAL_INSTRUCTION){
    
  }
#   if defined(OC_ARM_ASM_NEON)
  __try{
    
    __emit(0xF2200150);
    flags|=OC_CPU_ARM_NEON;
  }
  __except(GetExceptionCode()==EXCEPTION_ILLEGAL_INSTRUCTION){
    
  }
#   endif
#  endif
# endif
  return flags;
}

#elif defined(__linux__)
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

ogg_uint32_t oc_cpu_flags_get(void){
  ogg_uint32_t  flags;
  FILE         *fin;
  flags=0;
  


  fin=fopen("/proc/cpuinfo","r");
  if(fin!=NULL){
    

    char buf[512];
    while(fgets(buf,511,fin)!=NULL){
      if(memcmp(buf,"Features",8)==0){
        char *p;
        p=strstr(buf," edsp");
        if(p!=NULL&&(p[5]==' '||p[5]=='\n'))flags|=OC_CPU_ARM_EDSP;
        p=strstr(buf," neon");
        if(p!=NULL&&(p[5]==' '||p[5]=='\n'))flags|=OC_CPU_ARM_NEON;
      }
      if(memcmp(buf,"CPU architecture:",17)==0){
        int version;
        version=atoi(buf+17);
        if(version>=6)flags|=OC_CPU_ARM_MEDIA;
      }
    }
    fclose(fin);
  }
  return flags;
}

#else



# error "Configured to use ARM asm but no CPU detection method available for " \
 "your platform.  Reconfigure with --disable-asm (or send patches)."
#endif
