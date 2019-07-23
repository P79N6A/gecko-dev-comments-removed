
































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_ARM_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_ARM_H__

#define MD_FLOATINGSAVEAREA_ARM_FPR_COUNT 32
#define MD_FLOATINGSAVEAREA_ARM_FPEXTRA_COUNT 8








typedef struct {
  u_int64_t	fpscr;      

  
  u_int64_t	regs[MD_FLOATINGSAVEAREA_ARM_FPR_COUNT];

  
  u_int32_t     extra[MD_FLOATINGSAVEAREA_ARM_FPEXTRA_COUNT];
} MDFloatingSaveAreaARM;

#define MD_CONTEXT_ARM_GPR_COUNT 16

typedef struct {
  


  u_int32_t	context_flags;

  





  u_int32_t     iregs[MD_CONTEXT_ARM_GPR_COUNT];

  






  u_int32_t    cpsr;

  
  MDFloatingSaveAreaARM float_save;

} MDRawContextARM;



#define MD_CONTEXT_ARM_INTEGER           (MD_CONTEXT_ARM | 0x00000002)
#define MD_CONTEXT_ARM_FLOATING_POINT    (MD_CONTEXT_ARM | 0x00000004)

#define MD_CONTEXT_ARM_FULL              (MD_CONTEXT_ARM_INTEGER | \
                                          MD_CONTEXT_ARM_FLOATING_POINT)

#define MD_CONTEXT_ARM_ALL               (MD_CONTEXT_ARM_INTEGER | \
                                          MD_CONTEXT_ARM_FLOATING_POINT)

#endif  
