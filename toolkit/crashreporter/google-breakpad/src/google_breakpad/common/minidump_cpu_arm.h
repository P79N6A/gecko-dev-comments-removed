
































































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_ARM_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_CPU_ARM_H__

#define MD_FLOATINGSAVEAREA_ARM_FPR_COUNT 32
#define MD_FLOATINGSAVEAREA_ARM_FPEXTRA_COUNT 8








typedef struct {
  uint64_t	fpscr;      

  
  uint64_t	regs[MD_FLOATINGSAVEAREA_ARM_FPR_COUNT];

  
  uint32_t     extra[MD_FLOATINGSAVEAREA_ARM_FPEXTRA_COUNT];
} MDFloatingSaveAreaARM;

#define MD_CONTEXT_ARM_GPR_COUNT 16

typedef struct {
  


  uint32_t	context_flags;

  





  uint32_t     iregs[MD_CONTEXT_ARM_GPR_COUNT];

  






  uint32_t    cpsr;

  
  MDFloatingSaveAreaARM float_save;

} MDRawContextARM;




enum MDARMRegisterNumbers {
  MD_CONTEXT_ARM_REG_IOS_FP = 7,
  MD_CONTEXT_ARM_REG_FP     = 11,
  MD_CONTEXT_ARM_REG_SP     = 13,
  MD_CONTEXT_ARM_REG_LR     = 14,
  MD_CONTEXT_ARM_REG_PC     = 15
};









#define MD_CONTEXT_ARM_OLD               0x00000040


#define MD_CONTEXT_ARM                   0x40000000
#define MD_CONTEXT_ARM_INTEGER           (MD_CONTEXT_ARM | 0x00000002)
#define MD_CONTEXT_ARM_FLOATING_POINT    (MD_CONTEXT_ARM | 0x00000004)

#define MD_CONTEXT_ARM_FULL              (MD_CONTEXT_ARM_INTEGER | \
                                          MD_CONTEXT_ARM_FLOATING_POINT)

#define MD_CONTEXT_ARM_ALL               (MD_CONTEXT_ARM_INTEGER | \
                                          MD_CONTEXT_ARM_FLOATING_POINT)

#endif  
