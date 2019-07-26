




























#ifndef GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_UCONTEXT_H
#define GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_UCONTEXT_H

#include <sys/cdefs.h>

#ifdef __BIONIC_UCONTEXT_H
#include <ucontext.h>
#else

#include <sys/ucontext.h>

#ifdef __cplusplus
extern "C" {
#endif


int breakpad_getcontext(ucontext_t* ucp);

#define getcontext(x)   breakpad_getcontext(x)

#ifdef __cplusplus
}  
#endif  

#endif

#endif
