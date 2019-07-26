







#ifndef SANDBOX_LINUX_SERVICES_ANDROID_I386_UCONTEXT_H_
#define SANDBOX_LINUX_SERVICES_ANDROID_I386_UCONTEXT_H_






#if !defined(__BIONIC_HAVE_UCONTEXT_T)
#include <asm/sigcontext.h>


struct _libc_fpreg {
  unsigned short significand[4];
  unsigned short exponent;
};


struct _libc_fpstate {
  unsigned long cw;
  unsigned long sw;
  unsigned long tag;
  unsigned long ipoff;
  unsigned long cssel;
  unsigned long dataoff;
  unsigned long datasel;
  struct _libc_fpreg _st[8];
  unsigned long status;
};

typedef uint32_t greg_t;

typedef struct {
  uint32_t gregs[19];
  struct _libc_fpstate* fpregs;
  uint32_t oldmask;
  uint32_t cr2;
} mcontext_t;

enum {
  REG_GS = 0,
  REG_FS,
  REG_ES,
  REG_DS,
  REG_EDI,
  REG_ESI,
  REG_EBP,
  REG_ESP,
  REG_EBX,
  REG_EDX,
  REG_ECX,
  REG_EAX,
  REG_TRAPNO,
  REG_ERR,
  REG_EIP,
  REG_CS,
  REG_EFL,
  REG_UESP,
  REG_SS,
};

typedef struct ucontext {
  uint32_t uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  sigset_t uc_sigmask;
  struct _libc_fpstate __fpregs_mem;
} ucontext_t;

#else
#include <sys/ucontext.h>
#endif  

#endif  
