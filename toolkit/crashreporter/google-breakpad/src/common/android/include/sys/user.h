




























#ifndef GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_SYS_USER_H
#define GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_SYS_USER_H

#ifdef __cplusplus
extern "C" {
#endif







#if defined(__arm__)

#define _ARM_USER_H  1  // Prevent <asm/user.h> conflicts


struct user_regs {
  
  
  
  unsigned long int uregs[18];
};


struct user_fpregs {
  struct fp_reg {
    unsigned int sign1:1;
    unsigned int unused:15;
    unsigned int sign2:1;
    unsigned int exponent:14;
    unsigned int j:1;
    unsigned int mantissa1:31;
    unsigned int mantissa0:32;
  } fpregs[8];
  unsigned int  fpsr:32;
  unsigned int  fpcr:32;
  unsigned char ftype[8];
  unsigned int  init_flag;
};


struct user_vfpregs {
  unsigned long long  fpregs[32];
  unsigned long       fpscr;
};

#elif defined(__i386__)

#define _I386_USER_H 1  // Prevent <asm/user.h> conflicts


struct user_regs_struct {
  long ebx, ecx, edx, esi, edi, ebp, eax;
  long xds, xes, xfs, xgs, orig_eax;
  long eip, xcs, eflags, esp, xss;
};

struct user_fpregs_struct {
  long cwd, swd, twd, fip, fcs, foo, fos;
  long st_space[20];
};

struct user_fpxregs_struct {
  unsigned short cwd, swd, twd, fop;
  long fip, fcs, foo, fos, mxcsr, reserved;
  long st_space[32];
  long xmm_space[32];
  long padding[56];
};

struct user {
  struct user_regs_struct    regs;
  int                        u_fpvalid;
  struct user_fpregs_struct  i387;
  unsigned long              u_tsize;
  unsigned long              u_dsize;
  unsigned long              u_ssize;
  unsigned long              start_code;
  unsigned long              start_stack;
  long                       signal;
  int                        reserved;
  struct user_regs_struct*   u_ar0;
  struct user_fpregs_struct* u_fpstate;
  unsigned long              magic;
  char                       u_comm [32];
  int                        u_debugreg [8];
};


#elif defined(__mips__)




#else
#  error "Unsupported Android CPU ABI"
#endif

#ifdef __cplusplus
}  
#endif  

#endif
