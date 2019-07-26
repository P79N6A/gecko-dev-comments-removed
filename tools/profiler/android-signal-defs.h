










#if defined(__arm__) || defined(__thumb__) || ANDROID_VERSION >= 9
#include <asm/sigcontext.h>
#else
#error use newer NDK or newer platform version (e.g. --with-android-version=9)
#endif

#ifndef __BIONIC_HAVE_UCONTEXT_T
typedef uint32_t __sigset_t;
typedef struct sigcontext mcontext_t;
typedef struct ucontext {
  uint32_t uc_flags;
  struct ucontext* uc_link;
  stack_t uc_stack;
  mcontext_t uc_mcontext;
  __sigset_t uc_sigmask;
} ucontext_t;
#endif

