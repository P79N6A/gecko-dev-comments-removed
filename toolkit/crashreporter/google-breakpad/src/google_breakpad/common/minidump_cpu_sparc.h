











































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_SPARC_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_SPARC_H__

#define MD_FLOATINGSAVEAREA_SPARC_FPR_COUNT 32

typedef struct {

  
  uint64_t      regs[MD_FLOATINGSAVEAREA_SPARC_FPR_COUNT];

  uint64_t      filler;
  uint64_t      fsr;        
} MDFloatingSaveAreaSPARC;  

#define MD_CONTEXT_SPARC_GPR_COUNT 32

typedef struct {
  


  uint32_t      context_flags;
  uint32_t      flag_pad;
  






  

  




  uint64_t     g_r[MD_CONTEXT_SPARC_GPR_COUNT];

  

  


  uint64_t     ccr;

  uint64_t     pc;     
  uint64_t     npc;    
  uint64_t     y;      

  


  uint64_t     asi;

  


  uint64_t     fprs;

  
  MDFloatingSaveAreaSPARC float_save;

} MDRawContextSPARC;  





#define MD_CONTEXT_SPARC                 0x10000000
#define MD_CONTEXT_SPARC_CONTROL         (MD_CONTEXT_SPARC | 0x00000001)
#define MD_CONTEXT_SPARC_INTEGER         (MD_CONTEXT_SPARC | 0x00000002)
#define MD_CONTEXT_SAPARC_FLOATING_POINT (MD_CONTEXT_SPARC | 0x00000004)
#define MD_CONTEXT_SAPARC_EXTRA          (MD_CONTEXT_SPARC | 0x00000008)

#define MD_CONTEXT_SPARC_FULL            (MD_CONTEXT_SPARC_CONTROL | \
                                          MD_CONTEXT_SPARC_INTEGER)

#define MD_CONTEXT_SPARC_ALL             (MD_CONTEXT_SPARC_FULL | \
                                          MD_CONTEXT_SAPARC_FLOATING_POINT | \
                                          MD_CONTEXT_SAPARC_EXTRA)

#endif 
