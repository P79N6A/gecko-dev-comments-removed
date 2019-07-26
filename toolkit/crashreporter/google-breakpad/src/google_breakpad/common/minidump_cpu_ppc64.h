












































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_PPC64_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_PPC64_H__

#include "minidump_cpu_ppc.h"


typedef MDFloatingSaveAreaPPC MDFloatingSaveAreaPPC64;
typedef MDVectorSaveAreaPPC MDVectorSaveAreaPPC64;

#define MD_CONTEXT_PPC64_GPR_COUNT MD_CONTEXT_PPC_GPR_COUNT

typedef struct {
  


  uint64_t              context_flags;

  uint64_t              srr0;    

  uint64_t              srr1;    

  

  uint64_t              gpr[MD_CONTEXT_PPC64_GPR_COUNT];
  uint64_t              cr;      
  uint64_t              xer;     
  uint64_t              lr;      
  uint64_t              ctr;     
  uint64_t              vrsave;  

  


  MDFloatingSaveAreaPPC float_save;
  MDVectorSaveAreaPPC   vector_save;
} MDRawContextPPC64;  





#define MD_CONTEXT_PPC                0x20000000
#define MD_CONTEXT_PPC_BASE           (MD_CONTEXT_PPC | 0x00000001)
#define MD_CONTEXT_PPC_FLOATING_POINT (MD_CONTEXT_PPC | 0x00000008)
#define MD_CONTEXT_PPC_VECTOR         (MD_CONTEXT_PPC | 0x00000020)

#define MD_CONTEXT_PPC_FULL           MD_CONTEXT_PPC_BASE
#define MD_CONTEXT_PPC_ALL            (MD_CONTEXT_PPC_FULL | \
                                       MD_CONTEXT_PPC_FLOATING_POINT | \
                                       MD_CONTEXT_PPC_VECTOR)

#endif 
