











































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_SPARC_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_SPARC_H__

#define MD_FLOATINGSAVEAREA_SPARC_FPR_COUNT 32

typedef struct {

  
  u_int64_t	regs[MD_FLOATINGSAVEAREA_SPARC_FPR_COUNT];

  u_int64_t	filler;
  u_int64_t	fsr;        
} MDFloatingSaveAreaSPARC;  

#define MD_CONTEXT_SPARC_GPR_COUNT 32

typedef struct {
  


  u_int32_t	context_flags;
  u_int32_t	flag_pad;
  






  

  




  u_int64_t     g_r[MD_CONTEXT_SPARC_GPR_COUNT];

  

  


  u_int64_t     ccr;

  u_int64_t     pc;     
  u_int64_t     npc;    
  u_int64_t     y;      

  


  u_int64_t     asi;

  


  u_int64_t     fprs;

  
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
