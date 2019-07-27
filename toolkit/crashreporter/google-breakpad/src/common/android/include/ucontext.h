




























#ifndef GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_UCONTEXT_H
#define GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_UCONTEXT_H

#include <sys/cdefs.h>
#include <signal.h>

#ifdef __BIONIC_HAVE_UCONTEXT_H
# include_next <ucontext.h>
#else
# include <sys/ucontext.h>
#endif  

#ifdef __cplusplus
extern "C" {
#endif


int breakpad_getcontext(ucontext_t* ucp);

#define getcontext(x)   breakpad_getcontext(x)

#ifdef __cplusplus
}  
#endif  

#endif
