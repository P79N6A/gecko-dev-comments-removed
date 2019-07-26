







#ifndef SANDBOX_LINUX_SERVICES_ANDROID_ARM_UCONTEXT_H_
#define SANDBOX_LINUX_SERVICES_ANDROID_ARM_UCONTEXT_H_

#if !defined(__BIONIC_HAVE_UCONTEXT_T)
#include <asm/sigcontext.h>


typedef unsigned long greg_t;


typedef struct ucontext {
  unsigned long   uc_flags;
  struct ucontext  *uc_link;
  stack_t     uc_stack;
  struct sigcontext uc_mcontext;
  sigset_t    uc_sigmask;
  
  int     __not_used[32 - (sizeof (sigset_t) / sizeof (int))];
  

  unsigned long   uc_regspace[128] __attribute__((__aligned__(8)));
} ucontext_t;

#else
#include <sys/ucontext.h>
#endif  

#endif  
